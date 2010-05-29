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

using namespace std;

static void extrafile(const char *name, const char *contents) {
  ofstream os((Config::output + "/" + name).c_str());
  os.exceptions(ofstream::badbit|ofstream::failbit);
  os << contents << flush;
}

int main(int argc, char **argv) {
  init_timezones();
  Config::Options(argc, argv);
  // Scan everything
  AllGroups all;
  all.scan();
  // Generate the logs & report
  all.logs();
  all.report();
  // Auxilary files
  extrafile("sorttable.js", sorttable);
  extrafile("spoolstats.css", css);
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
