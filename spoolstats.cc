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
  { "debug", no_argument, 0, 'd' },
  { "spool", required_argument, 0, 'S' },
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'V' },
 { 0, 0, 0, 0 }
};

/* --- reporting ----------------------------------------------------------- */

static void report() {
  vector<string> grouplist;

  grouplist.reserve(Group::groups.size());
  for(map<string,Group *>::const_iterator it = Group::groups.begin();
      it != Group::groups.end();
      ++it)
    grouplist.push_back(it->first);
  sort(grouplist.begin(), grouplist.end());
  for(size_t n = 0; n < grouplist.size(); ++n) {
    Group *g = Group::groups[grouplist[n]];
    g->report();
  }
}

/* --- spool processing ---------------------------------------------------- */

static void visit_article(const std::string &path) {
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
  Article::visit(buffer);
}

static void visit_spool(const std::string &path) {
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
        visit_spool(nodepath);
      else if(S_ISREG(sb.st_mode))
        visit_article(nodepath);
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

  init_timezones();
  while((n = getopt_long(argc, argv, "dS:hV", options, 0)) >= 0) {
    switch(n) {
    case 'd':
      debug = 1;
      break;
    case 'S':
      spool = optarg;
      break;
    case 'h':
      printf("Usage:\n\
  spoolstats [OPTIONS]\n\
\n\
Options:\n\
  -S, --spool PATH      Path to spool\n\
  -h, -help             Display usage message\n\
  -V, --version         Display verison number\n");
      exit(0);
    case 'V':
      printf("spoolstats from rjk-nntp-tools version " VERSION "\n");
      exit(0);
    default:
      exit(1);
    }
  }
  visit_spool(spool);
  report();
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
