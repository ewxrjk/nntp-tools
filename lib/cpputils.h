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
#ifndef CPPUTILS
#define CPPUTILS

#include <vector>
#include <string>
#include <ostream>
#include <stdint.h>
#include <ctime>

void split(std::vector<std::string> &bits,
           char sep,
           const std::string &s);

class Bytes {
public:
  inline Bytes(intmax_t n): bytes(n) {}
  intmax_t bytes;
  static std::ostream &write(std::ostream &os, intmax_t bytes);
};

inline std::ostream &operator<<(std::ostream &os, const Bytes &b) {
  return b.write(os, b.bytes);
}

time_t parse_date(const std::string &d);
std::string &lower(std::string &s);
std::string &upper(std::string &s);

#endif /* CPPUTILS */

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
