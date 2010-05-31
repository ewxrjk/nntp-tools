/*
 * This file is part of spoolstats.
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
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <algorithm>

using namespace std;

AllGroups::~AllGroups() {
}

// Visit one article
void AllGroups::visit(const Article *a) {
  Bucket::visit(a);
}

// Scan the spool
void AllGroups::scan() {
  for(map<string,Hierarchy *>::const_iterator it = Config::hierarchies.begin();
      it != Config::hierarchies.end();
      ++it) {
    Hierarchy *const h = it->second;
    recurse(Config::spool + "/" + h->name);
    // TODO we could report and delete h here if we introduced an end_mtime.
  }
  // AllGroups::recurse() keeps a running count, erase it now we're done
  if(Config::terminal)
    cerr << "                    \r";
}

// Recurse into one directory
void AllGroups::recurse(const string &dir) {
  static int count, included;
  DIR *dp;
  struct dirent *de;
  string nodepath;
  struct stat sb;

  if(!(dp = opendir(dir.c_str())))
    fatal(errno, "opening %s", dir.c_str());
  errno = 0;
  while((de = readdir(dp))) {
    if(de->d_name[0] != '.') {
      // TODO we have a further possible optimization; if the article name is
      // numerically lower than one known to be too early by mtime, we can skip
      // it before we even stat it.
      nodepath = dir;
      nodepath += "/";
      nodepath += de->d_name;
      if(stat(nodepath.c_str(), &sb) < 0)
        fatal(errno, "stat %s", nodepath.c_str());
      if(S_ISDIR(sb.st_mode))
        recurse(nodepath);
      else if(S_ISREG(sb.st_mode)) {
        if(sb.st_mtime >= Config::start_mtime)
          included += visit(nodepath);
        count += 1;
        if(Config::terminal && count % 11 == 0)
          cerr << included << "/" << count << "\r";
      }
    }
    errno = 0;                          // stupid readdir() API
  }
  if(errno)
    fatal(errno, "reading %s", dir.c_str());
  closedir(dp);
}

// Visit one article by name
int AllGroups::visit(const string &path) {
  int fd, bytes_read;
  string article;
  struct stat sb;
  char buffer[2048];

  if((fd = open(path.c_str(), O_RDONLY)) < 0)
    fatal(errno, "opening %s", path.c_str());
  if(fstat(fd, &sb) < 0)
    fatal(errno, "stat %s", path.c_str());
  while((bytes_read = read(fd, buffer, sizeof buffer)) > 0) {
    article.append(buffer, bytes_read);
    if(article.find("\n\n") != string::npos
       || article.find("\r\n\r\n") != string::npos)
      break;
  }
  if(bytes_read < 0)
    fatal(errno, "reading %s", path.c_str());
  close(fd);
  if(debug)
    cerr << "article " << path << endl;
  // Parse article
  Article a(article, sb.st_size);
  // Reject articles outside the sampling range
  if(a.date() < Config::start_time || a.date() >= Config::end_time)
    return 0;
  // Only visit each article once
  if(seen.find(a.mid()) != seen.end())
    return 0;
  seen.insert(a.mid());
  // Supply article to global bucket (AllGroups)
  visit(&a);
  // Get list of groups
  vector<string> groups;
  a.get_groups(groups);
  // Order the list, so we can easily de-dupe
  sort(groups.begin(), groups.end());
  const Hierarchy *last_h = NULL;
  int visited = 0;
  for(size_t n = 0; n < groups.size(); ++n) {
    // De-dupe groups
    if(n > 0 && groups[n-1] == groups[n])
      continue;
    // Identify hierarchy
    const string hname(groups[n], 0, groups[n].find('.'));
    // Eliminate unwanted hierarchies
    const map<string,Hierarchy *>::const_iterator it
      = Config::hierarchies.find(hname); // TODO encapsulate in Config
    if(it == Config::hierarchies.end())
      continue;
    Hierarchy *const h = it->second;
    // Add to group data
    h->group(groups[n])->visit(&a);
    visited = 1;
    // De-dupe hierarchy
    if(h == last_h)
      continue;
    // Add to hierarchy data
    h->visit(&a);
    // Remember hierarchy for next time
    last_h = h;
  }
  return visited;
}

// Generate all reports
void AllGroups::report() {
  report_hierarchies();
  report_groups();
  for(map<string,Hierarchy *>::const_iterator it = Config::hierarchies.begin();
      it != Config::hierarchies.end();
      ++it) {
    Hierarchy *const h = it->second;
    h->page();
  }
}

void AllGroups::report_hierarchies() {
  // TODO lots of scope to de-dupe with Hierarchy::page() here
  try {
    ofstream os((Config::output + "/index.html").c_str());
    os.exceptions(ofstream::badbit|ofstream::failbit);

    os << HTML::Header("Spool report", "spoolstats.css", "sorttable.js");

    os << "<h2>History</h2>\n";
    os << "<div>\n";
    os << "<p class=graph><a href=" << HTML::Quote("all.png") << ">"
       << "<img src=" << HTML::Quote("all.png") << ">"
       << "</a></p>\n";
    os << "</div>\n";

    os << "<h2>Last " << Config::days << " days</h2>\n";
    os << "<div>\n";
    os << "<table class=sortable>\n";

    os << "<thead>\n";
    os << "<tr>\n";
    os << "<th>Hierarchy</th>\n";
    os << "<th>Articles/day</th>\n";
    os << "<th>Bytes/day</th>\n";
    os << "<th>Posters</td>\n";
    os << "</tr>\n";
    os << "</thead>\n";

    for(map<string,Hierarchy *>::const_iterator it = Config::hierarchies.begin();
        it != Config::hierarchies.end();
        ++it) {
      Hierarchy *const h = it->second;
      h->summary(os);
    }

    const intmax_t total_bytes_per_day = bytes / Config::days;
    const double total_arts_per_day = (double)articles / Config::days;

    os << "<tfoot>\n";
    os << "<tr>\n";
    os << "<td><a href=" << HTML::Quote("allgroups.html") << ">"
       << "All groups</a></td>\n";
    os << "<td>"
       << setprecision(total_arts_per_day >= 10 ? 0 : 1) << total_arts_per_day
       << setprecision(6)
       << "</td>\n";
    os << "<td>" << Bytes(total_bytes_per_day) << "</td>\n";
    os << "</tr>\n";
    os << "</tfoot>\n";
    os << "</table>\n";
    os << "</div>\n";
    Config::footer(os);
    os << flush;
  } catch(ios::failure) {
    fatal(errno, "writing to %s", (Config::output + "/index.html").c_str());
  }
}

void AllGroups::report_groups() {
  try {
    ofstream os((Config::output + "/allgroups.html").c_str());
    os.exceptions(ofstream::badbit|ofstream::failbit);

    os << HTML::Header("All groups", "spoolstats.css", "sorttable.js");

    os << "<h2>Last " << Config::days << " days</h2>\n";
    os << "<div>\n";
    os << "<table class=sortable>\n";

    os << "<thead>\n";
    os << "<tr>\n";
    os << "<th>Group</th>\n";
    os << "<th>Articles/day</th>\n";
    os << "<th>Bytes/day</th>\n";
    os << "<th>Posters</td>\n";
    os << "</tr>\n";
    os << "</thead>\n";

    for(map<string,Hierarchy *>::const_iterator jt = Config::hierarchies.begin();
        jt != Config::hierarchies.end();
        ++jt) {
      Hierarchy *const h = jt->second;
      for(map<string,Group *>::const_iterator it = h->groups.begin();
          it != h->groups.end();
          ++it) {
        Group *g = it->second;              // Summary line
        g->summary(os);
      }
    }

    const intmax_t total_bytes_per_day = bytes / Config::days;
    const double total_arts_per_day = (double)articles / Config::days;

    os << "<tfoot>\n";
    os << "<tr>\n";
    os << "<td>Total</td>\n";
    os << "<td>"
       << setprecision(total_arts_per_day >= 10 ? 0 : 1) << total_arts_per_day
       << setprecision(6)
       << "</td>\n";
    os << "<td>" << Bytes(total_bytes_per_day) << "</td>\n";
    os << "<td></td>\n";                  // TODO?
    os << "</tr>\n";
    os << "</tfoot>\n";
    os << "</table>\n";
    os << "</div>\n";
    Config::footer(os);
    os << flush;
  } catch(ios::failure) {
    fatal(errno, "writing to %s", (Config::output + "/allgroups.html").c_str());
  }
}

void AllGroups::logs() {
  for(map<string,Hierarchy *>::const_iterator it = Config::hierarchies.begin();
      it != Config::hierarchies.end();
      ++it) {
    Hierarchy *const h = it->second;
    h->logs();
  }
  try {
    ofstream os((Config::output + "/all.csv").c_str(), ios::app);
    os.exceptions(ofstream::badbit|ofstream::failbit);
    os << Config::end_time
       << ',' << Config::days * 86400
       << ',' << bytes
       << ',' << articles
       << '\n'
       << flush;
  } catch(ios::failure) {
    fatal(errno, "writing to %s", (Config::output + "/all.csv").c_str());
  }
}

void AllGroups::graphs() {
  for(map<string,Hierarchy *>::const_iterator it = Config::hierarchies.begin();
      it != Config::hierarchies.end();
      ++it) {
    Hierarchy *const h = it->second;
    h->graphs();
  }
  graph("All groups",
        Config::output + "/all.csv", 
        Config::output + "/all.png");
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
