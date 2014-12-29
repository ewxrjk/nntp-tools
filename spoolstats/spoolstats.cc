/*
 * spoolstats - news spool stats
 * Copyright (C) 2010, 14 Richard Kettlewell
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

extern unsigned char sorttable_js[], spoolstats_css[];
extern unsigned sorttable_js_len, spoolstats_css_len;

using namespace std;

static void extrafile(const char *name, unsigned char *contents, unsigned len) {
  try {
    ofstream os((Config::output + "/" + name).c_str());
    os.exceptions(ofstream::badbit|ofstream::failbit);
    os.write((char *)contents, len);
    os << flush;
  } catch(ios::failure) {
    fatal(errno, "writing to %s", (Config::output + "/" + name).c_str());
  }
}

int main(int argc, char **argv) {
  Config::Options(argc, argv);
  // Become the right user
  if(Config::user.size())
    become(Config::user.c_str());
  // Scan everything
  AllGroups all;
  if(Config::scan) {
    all.scan();
    all.logs();
  } else
    all.readLogs();
  if(Config::graph) {
    // Generate  report
    all.graphs();
    all.report();
    // Auxiliary files
    extrafile("sorttable.js", sorttable_js, sorttable_js_len);
    extrafile("spoolstats.css", spoolstats_css, spoolstats_css_len);
  }
  return 0;
}
