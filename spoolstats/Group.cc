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
#include "spoolstats.h"

using namespace std;

Group::Group(const string &name_): name(name_) {
}

// Visit one article
void Group::visit(const Article *a) {
  Bucket::visit(a);
  const string &sender = a->sender();
  map<string,int>::iterator it = senders.find(sender);
  if(it == senders.end())
    senders[sender] = 1;
  else
    ++it->second;
}

// Generate table line
void Group::summary(ostream &os) {
  const intmax_t bytes_per_day = bytes / Config::days;
  const double arts_per_day = (double)articles / Config::days;
  const long posters = senders.size();
  os << "<tr>\n";
  os << "<td>" << HTML::Escape(name) << "</td>\n";
  os << "<td sorttable_customkey=-" << fixed << arts_per_day << ">"
     << setprecision(arts_per_day >= 10 ? 0 : 1) << arts_per_day
     << setprecision(6)
     << "</td>\n";
  os << "<td sorttable_customkey=-" << bytes_per_day << ">"
     << round_kilo(bytes_per_day) 
     << "</td>\n";
  os << "<td sorttable_customkey=-" << posters << ">" << posters << "</td>\n";
  // TODO can we find a better stream state restoration idiom?
}
