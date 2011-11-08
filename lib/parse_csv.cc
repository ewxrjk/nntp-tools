/*
 * This file is part of rjk-nntp-tools.
 * Copyright (C) 2010-11 Richard Kettlewell
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
#include <iomanip>

using namespace std;

// Parse a CSV file with numeric contents
void read_csv(const string &path, list<vector<Value> > &rows) {
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
    vector<Value> v;
    for(size_t k = 0; k < bits.size(); ++k) {
      const std::string bit = bits[k];
      if(isdigit(bit.at(0))) {
        stringstream ss(bit);
        intmax_t i;
        ss >> i;
        v.push_back(i);
      } else if(bit.at(0) == '"') {
        string s;
        size_t i = 1;
        while(i < bit.size() - 1) {
          if(bit.at(i) == '\\') {
            ++i;
            char ch = 0;
            int count = 0;
            while(count < 3 && bit.at(i) >= '0' && bit.at(i) <= '7') {
              ch = 8 * ch + bit.at(i) - '0';
              ++i;
              ++count;
            }
            s += ch;
          } else
            s += bit.at(i);
        }
        v.push_back(s);
      } else {
        v.push_back(bit);
      }
    }
    rows.push_back(v);
  }
  if(ferror(fp))
    fatal(errno, "reading %s", path.c_str());
  fclose(fp);
}

string csv_quote(const string &s) {
  stringstream quoted;
  quoted << '"';
  for(size_t n = 0; n < s.size(); ++n) {
    switch(s.at(n)) {
    case '"':
    case '\\':
      quoted << '\\' << s.at(n);
      break;
    default:
      if(s.at(n) < 0x20 || s.at(n) > 0x7E)
        quoted << '\\' << oct << setw(3) << setfill('0')
               << static_cast<int>(s.at(n));
      else
        quoted << s.at(n);
      break;
    }
  }
  quoted << '"';
  return quoted.str();
}
