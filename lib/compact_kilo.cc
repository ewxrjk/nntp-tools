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
#include "cpputils.h"
#include <sstream>

using namespace std;

/* Return a compact representation of N, using SI suffixes. */
string compact_kilo(double n) {
  stringstream ss;
  if(n < 1000)
    ss << n;
  else if(n < 1E6)
    ss << n / 1E3 << "K";
  else if(n < 1E9)
    ss << n / 1E6 << "M";
  else if(n < 1E12)
    ss << n / 1E9 << "G";
  return ss.str();
}

