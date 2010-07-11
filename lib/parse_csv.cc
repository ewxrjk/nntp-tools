/*
 * This file is part of rjk-nntp-tools.
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
#include "cpputils.h"
#include "utils.h"
#include <cerrno>
#include <sstream>

using namespace std;

void read_csv(const string &path, list<vector<intmax_t> > &rows) {
  // istream (at least as actually implemented) doesn't give us a way
  // to distinguish error from eof, so we don't use it.
  FILE *fp;
  if(!(fp = fopen(path.c_str(), "r")))
    fatal(errno, "opening %s", path.c_str());
  char *l = 0;
  size_t n = 0;
  while(getline(&l, &n, fp) >= 0) {
    vector<string> bits;
    split(bits, ',', string(l));
    vector<intmax_t> v;
    for(size_t k = 0; k < bits.size(); ++k) {
      stringstream ss(bits[k]);
      intmax_t i;
      ss >> i;
      v.push_back(i);
    }
    rows.push_back(v);
  }
  if(ferror(fp))
    fatal(errno, "reading %s", path.c_str());
  fclose(fp);
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
