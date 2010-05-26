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
#include <config.h>

#include "utils.h"
#include "Group.h"
#include "Article.h"
#include "timezones.h"

#include <iostream>
#include <algorithm>
#include <vector>

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
  { "groups", required_argument, 0, 'G' },
  { "big8", no_argument, 0, '8' },
  { "days", required_argument, 0, 'N' },
  { "help", no_argument, 0, 'h' },
  { "quiet", no_argument, 0, 'Q' },
  { "version", no_argument, 0, 'V' },
 { 0, 0, 0, 0 }
};

static bool terminal;
static time_t start_time, end_time;
static int days;

/* --- reporting ----------------------------------------------------------- */

static void report(int days) {
  vector<string> grouplist;

  cout << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\">\n";
  cout << "<html>\n";
  cout << "<head><title>Spool Report</title>\n";
  // TODO stylesheet
  cout << "<script src=\"sorttable.js\"></script>\n";
  cout << "<body>\n";
  cout << "<h1>Spool Report</h1>\n";

  cout << "<h2>Group Report</h2>\n";
  cout << "<table class=sortable>\n";
  cout << "<thead>\n";
  cout << "<tr>\n";
  cout << "<th>Group</th>\n";
  cout << "<th>Articles/day</th>\n";
  cout << "<th>Bytes/day</th>\n";
  cout << "<th>Posters</th>\n";
  cout << "</tr>\n";
  cout << "</thead>\n";

  grouplist.reserve(Group::groups.size());
  for(map<string,Group *>::const_iterator it = Group::groups.begin();
      it != Group::groups.end();
      ++it)
    grouplist.push_back(it->first);
  sort(grouplist.begin(), grouplist.end());
  for(size_t n = 0; n < grouplist.size(); ++n) {
    Group *g = Group::groups[grouplist[n]];
    g->report(days);
  }

  cout << "</table>\n";

  cout << "</body>\n";
  cout << "</html>\n";
}

/* --- spool processing ---------------------------------------------------- */

static void visit_article(const std::string &path,
                        time_t start_time,
                        time_t end_time) {
  int fd;
  string buffer;
  struct stat sb;

  if((fd = open(path.c_str(), O_RDONLY)) < 0)
    fatal(errno, "opening %s", path.c_str());
  if(fstat(fd, &sb) < 0)
    fatal(errno, "stat %s", path.c_str());
  buffer.resize(sb.st_size);
  int n = read(fd, (void *)buffer.data(), sb.st_size);
  if(n < 0)
    fatal(errno, "reading %s", path.c_str());
  else if(n != sb.st_size)
    fatal(0, "reading %s: wrong byte count", path.c_str());
  close(fd);
  if(debug)
    cerr << "article " << path << endl;
  int r = Article::visit(buffer, start_time, end_time);
  static int count, included;
  included += r;
  count += 1;
  if(terminal && count % 10 == 0)
    cerr << included << "/" << count << "\r";
}

static void visit_spool(const std::string &path,
                        time_t start_time,
                        time_t end_time) {
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
      // TODO if possible limit directory traversal to ones that can match some pattern
      // failing that limit article analysis!
      if(S_ISDIR(sb.st_mode))
        visit_spool(nodepath, start_time, end_time);
      else if(S_ISREG(sb.st_mode))
        visit_article(nodepath, start_time, end_time);
    }
    errno = 0;
  }
  if(errno)
    fatal(errno, "reading %s", path.c_str());
  closedir(dp);
}

/* --- main ---------------------------------------------------------------- */

int main(int argc, char **argv) {
  const char *spool = "/var/spool/news/articles";
  int n;
  time_t start_time, end_time;
  int days = 7;

  init_timezones();
  terminal = !!isatty(2);
  while((n = getopt_long(argc, argv, "DS:QhG:8VN:", options, 0)) >= 0) {
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
    case 'G':
      Group::set_patterns(optarg);
      break;
    case '8':
      Group::set_patterns("comp.*,talk.*,misc.*,news.*,soc.*,sci.*,humanities.*,rec.*");
      break;
    case 'N':
      days = atoi(optarg);
      if(days <= 0)
        fatal(0, "--days must be positive");
      break;
    case 'h':
      printf("Usage:\n\
  spoolstats [OPTIONS]\n\
\n\
Options:\n\
  -N, --days DAYS        Number of days to analyse\n\
  -S, --spool PATH       Path to spool\n\
  -G, --groups PATTERNS  Groups to analyse\n\
  -8, --big8             Analyse the Big 8\n\
  -Q, --quiet            Quieter operation\n\
  -h, --help             Display usage message\n\
  -V, --version          Display version number\n");
      exit(0);
    case 'V':
      printf("spoolstats from rjk-nntp-tools version " VERSION "\n");
      exit(0);
    default:
      exit(1);
    }
  }
  time(&end_time);
  start_time = end_time - 86400 * days;
  visit_spool(spool, start_time, end_time);
  if(terminal)
    cerr << "                    \r";
  report(days);
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
