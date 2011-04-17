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
#include <cstdarg>

using namespace std;

namespace HTML {

  // Write out a table heading row.  The arguments are a
  // null-pointer-terminated list of char *s.
  void thead(ostream &os, const char *heading, ...) {
    va_list ap;
    os << "<thead>\n";
    os << "<tr>\n";
    va_start(ap, heading);
    do {
      os << "<th>" << Escape(heading) << "</th>\n";
      heading = va_arg(ap, const char *);
    } while(heading);
    va_end(ap);
    os << "</tr>\n";
    os << "</thead>\n";
  }

}
