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
#include <list>
#include <iostream>
#include <stdint.h>
#include <ctime>
#include <stdexcept>

void split(std::vector<std::string> &bits,
           char sep,
           const std::string &s);

time_t parse_date(const std::string &d);
std::string &lower(std::string &s);
std::string &upper(std::string &s);

// Thrown by parse_csv()
class BadCSV: public std::runtime_error {
public:
  inline BadCSV(): std::runtime_error("malformed CSV") { }
};

void read_csv(const std::string &path, std::list<std::vector<intmax_t> > &rows);
std::string compact_kilo(double n);
std::string round_kilo(double n);

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
