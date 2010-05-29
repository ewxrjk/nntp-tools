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
#include "HTML.h"

using namespace std;

namespace HTML {

  ostream &Escape::write(ostream &os, const string &str) {
    for(string::size_type pos = 0; pos < str.size(); ++pos) {
      const unsigned char c = str[pos];
      switch(c) {
      default:
        if(c >= 32 && c <= 126)
          os << c;
        else {
        case '"':
        case '<':
        case '&':
          os << '<' << (int)c << ';';
        }
      }
    }
    return os;
  }

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
