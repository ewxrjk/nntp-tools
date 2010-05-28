/*
 * spoolstats - news spool stats
 * Copyright (C) 2010 Richard Kettlewell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */
#include "spoolstats.h"

#include <algorithm>
#include <fstream>

#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <ctime>
#include <getopt.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

/* --- options ------------------------------------------------------------- */

const struct option options[] = {
  { "debug", no_argument, 0, 'D' },
  { "spool", required_argument, 0, 'S' },
  { "hierarchy", required_argument, 0, 'H' },
  { "big8", no_argument, 0, '8' },
  { "days", required_argument, 0, 'N' },
  { "latency", required_argument, 0, 'L' },
  { "output", required_argument, 0, 'O' },
  { "help", no_argument, 0, 'h' },
  { "quiet", no_argument, 0, 'Q' },
  { "version", no_argument, 0, 'V' },
 { 0, 0, 0, 0 }
};

static bool terminal;
static time_t start_time, end_time, start_mtime;
static int days;
static int end_latency = 86400;
static int start_latency = 60;
static string output = ".";

/* --- reporting ----------------------------------------------------------- */

static void html_header(const string &title, ostream &os) {
  os << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\">\n";
  os << "<html>\n";
  os << "<head><title>";
  html_quote(os, title) << "</title>\n";
  // TODO stylesheet
  os << "<script src=\"sorttable.js\"></script>\n";
  os << "<body>\n";
  os << "<h1>";
  html_quote(os, title) << "</h1>\n";
}

static void report(int days, ostream &os) {
  vector<string> grouplist;

  html_header("Spool report", os);

  os << "<h2>Hierarchies</h2>\n";

  os << "<table class=sortable>\n";
  os << "<thead>\n";
  os << "<tr>\n";
  os << "<th>Hierarchy</th>\n";
  os << "<th>Articles/day</th>\n";
  os << "<th>Bytes/day</th>\n";
  os << "<th>Posters</th>\n";
  os << "</tr>\n";
  os << "</thead>\n";

  for(set<string>::const_iterator it = Group::hierarchies.begin();
      it != Group::hierarchies.end();
      ++it) {
    // TODO more detail
    os << "<tr>\n";
    os << "<td><a href=\"" << *it << ".html\">" << *it << ".*</a></td>\n";
    os << "</tr>\n";
  }

  os << "</table>\n";

  os << "</body>\n";
  os << "</html>\n";
}

static void report_hierarchy(const string &hierarchy, int days, ostream &os) {
  vector<string> grouplist;

  html_header(hierarchy + ".*", os);

  os << "<table class=sortable>\n";
  os << "<thead>\n";
  os << "<tr>\n";
  os << "<th>Group</th>\n";
  os << "<th>Articles/day</th>\n";
  os << "<th>Bytes/day</th>\n";
  os << "<th>Posters</th>\n";
  os << "</tr>\n";
  os << "</thead>\n";

  for(map<string,Group *>::const_iterator it = Group::groups.begin();
      it != Group::groups.end();
      ++it) {
    string::size_type n = it->first.find('.');
    if(it->first.compare(0, n, hierarchy) == 0)
      grouplist.push_back(it->first);
  }
  sort(grouplist.begin(), grouplist.end());
  long articles = 0;
  long long bytes = 0;
  for(size_t n = 0; n < grouplist.size(); ++n) {
    Group *g = Group::groups[grouplist[n]];
    g->report(days, os);
    articles += g->articles;
    bytes += g->bytes;
  }

  os << "</table>\n";

  os << "</body>\n";
  os << "</html>\n";
}

/* --- spool processing ---------------------------------------------------- */

static int visit_article(const string &path,
                         time_t start_time,
                         time_t end_time) {
  int fd, n;
  string article;
  struct stat sb;
  char buffer[2048];

  if((fd = open(path.c_str(), O_RDONLY)) < 0)
    fatal(errno, "opening %s", path.c_str());
  while((n = read(fd, buffer, sizeof buffer)) > 0) {
    article.append(buffer, n);
    if(article.find("\n\n") != string::npos
       || article.find("\r\n\r\n") != string::npos)
      break;
  }
  if(n < 0)
    fatal(errno, "reading %s", path.c_str());
  close(fd);
  if(debug)
    cerr << "article " << path << endl;
  return Article::visit(article, start_time, end_time);
}

static void visit_spool(const string &root,
                        const string &path,
                        time_t start_time,
                        time_t end_time) {
        static int count, included;

  DIR *dp;
  struct dirent *de;
  string nodepath;

  if(!(dp = opendir(path.c_str())))
    fatal(errno, "opening %s", path.c_str());
  errno = 0;
  while((de = readdir(dp))) {
    if(de->d_name[0] != '.') {
      struct stat sb;

      nodepath = path;
      nodepath += "/";
      nodepath += de->d_name;
      if(stat(nodepath.c_str(), &sb) < 0)
        fatal(errno, "stat %s", nodepath.c_str());
      if(S_ISDIR(sb.st_mode))
        visit_spool(root, nodepath, start_time, end_time);
      else if(S_ISREG(sb.st_mode)) {
        if(sb.st_mtime >= start_mtime) {
          string group(path, root.size() + 1);
          string::size_type n;
          while((n = group.find('/')) != string::npos)
            group[n] = '.';
          included += visit_article(nodepath, start_time, end_time);
        }
        count += 1;
        if(terminal && count % 10 == 0)
          cerr << included << "/" << count << "\r";
      }
    }
    errno = 0;
  }
  if(errno)
    fatal(errno, "reading %s", path.c_str());
  closedir(dp);
}

/* --- main ---------------------------------------------------------------- */

int main(int argc, char **argv) {
  string spool = "/var/spool/news/articles";
  int n;
  time_t start_time, end_time;
  int days = 7;

  init_timezones();
  terminal = !!isatty(2);
  while((n = getopt_long(argc, argv, "DS:QhH:8VN:L:O:", options, 0)) >= 0) {
    switch(n) {
    case 'D':
      debug = 1;
      break;
    case 'S':
      spool = optarg;
      break;
    case 'Q':
      terminal = false;
      break;
    case 'H': {
      vector<string> bits;
      split(bits, ',', string(optarg));
      for(size_t i = 0; i < bits.size(); ++i)
        Group::hierarchies.insert(bits[i]);
      break;
    }
    case 'L': {
      vector<string> bits;
      split(bits, ',', string(optarg));
      if(bits.size() != 2)
        fatal(0, "invalid argument to --latency option");
      start_latency = atoi(bits[0].c_str());
      end_latency = atoi(bits[1].c_str());
      break;
    }
    case '8':
      Group::hierarchies.insert("news");
      Group::hierarchies.insert("humanities");
      Group::hierarchies.insert("sci");
      Group::hierarchies.insert("comp");
      Group::hierarchies.insert("misc");
      Group::hierarchies.insert("soc");
      Group::hierarchies.insert("rec");
      Group::hierarchies.insert("talk");
      break;
    case 'N':
      days = atoi(optarg);
      if(days <= 0)
        fatal(0, "--days must be positive");
      break;
    case 'O':
      output = optarg;
      break;
    case 'h':
      printf("Usage:\n\
  spoolstats [OPTIONS]\n\
\n\
Options:\n\
  -N, --days DAYS                 Number of days to analyse\n\
  -L, --latency BEFORE, AFTER     Set latencies in seconds\n\
  -S, --spool PATH                Path to spool\n\
  -H, --hierarchy NAME[,NAME...]  Hierachies to analyse\n\
  -8, --big8                      Analyse the Big 8\n\
  -O, --output DIRECTORY          Output directory\n\
  -Q, --quiet                     Quieter operation\n\
  -h, --help                      Display usage message\n\
  -V, --version                   Display version number\n");
      exit(0);
    case 'V':
      printf("spoolstats from rjk-nntp-tools version " VERSION "\n");
      exit(0);
    default:
      exit(1);
    }
  }
  // Compute sampling interval
  time(&end_time);
  end_time -= end_latency;
  start_time = end_time - 86400 * days;
  start_mtime = start_time - start_latency;
  // Scan each hierarchy.  Article/Group will cooperate to ensure that each
  // article is only processed once even if we see it multiple times due to
  // crossposting.
  for(set<string>::const_iterator it = Group::hierarchies.begin();
      it != Group::hierarchies.end();
      ++it)
    visit_spool(spool, spool + "/" + *it, start_time, end_time);
  if(terminal)
    cerr << "                    \r";
  // Generate overall report
  ofstream index((output + "/index.html").c_str());
  index.exceptions(ofstream::failbit|ofstream::badbit);
  report(days, index);
  // Generate hierarhcy reports
  for(set<string>::const_iterator it = Group::hierarchies.begin();
      it != Group::hierarchies.end();
      ++it) {
    ofstream os((output + "/" + *it + ".html").c_str());
    os.exceptions(ofstream::failbit|ofstream::badbit);
    report_hierarchy(*it, days, os);
  }
  return 0;
}

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
