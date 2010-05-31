/*
 * This file is part of spoolstats.
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

Hierarchy::Hierarchy(const string &name_): name(name_) {
}

Hierarchy::~Hierarchy() {
  for(map<string,Group *>::const_iterator it = groups.begin();
      it != groups.end();
      ++it)
    delete it->second;
}

Group *Hierarchy::group(const string &groupname) {
  map<string,Group *>::iterator it = groups.find(groupname);
  if(it != groups.end())
    return it->second;
  Group *g = new Group(groupname);
  groups.insert(pair<string,Group *>(groupname, g));
  return g;
}

void Hierarchy::visit(const Article *a) {
  SenderCountingBucket::visit(a);
}

void Hierarchy::summary(ostream &os) {
  const intmax_t bytes_per_day = bytes / Config::days;
  const double arts_per_day = (double)articles / Config::days;
  const long posters = senders.size();
  os << "<tr>\n";
  os << "<td><a href=" << HTML::Quote(name + ".html") << ">"
     << HTML::Escape(name) << ".*</a></td>\n";
  os << "<td sorttable_customkey=-" << fixed << arts_per_day << ">"
     << setprecision(arts_per_day >= 10 ? 0 : 1) << arts_per_day
     << setprecision(6)
     << "</td>\n";
  os << "<td sorttable_customkey=-" << bytes_per_day << ">"
     << Bytes(bytes_per_day) 
     << "</td>\n";
  os << "<td sorttable_customkey=-" << posters << ">" << posters << "</td>\n";
  os << "</tr>\n";
  // TODO can we find a better stream state restoration idiom?
}

void Hierarchy::page() {
  try {
    ofstream os((Config::output + "/" + name + ".html").c_str());
    os.exceptions(ofstream::badbit|ofstream::failbit);

    os << HTML::Header(name + ".*", "spoolstats.css", "sorttable.js");

    os << "<h2>History</h2>\n";
    os << "<div>\n";
    os << "<p class=graph><a href=" << HTML::Quote(name + ".png") << ">"
       << "<img src=" << HTML::Quote(name + ".png") << ">"
       << "</a></p>\n";
    os << "</div>\n";

    os << "<h2>Last " << Config::days << " days</h2>\n";
    os << "<div>\n";
    os << "<table class=sortable>\n";

    os << "<thead>\n";
    os << "<tr>\n";
    os << "<th>Group</th>\n";
    os << "<th>Articles/day</th>\n";
    os << "<th>Bytes/day</th>\n";
    os << "<th>Posters</td>\n";
    os << "</tr>\n";
    os << "</thead>\n";

    for(map<string,Group *>::const_iterator it = groups.begin();
        it != groups.end();
        ++it) {
      Group *g = it->second;              // Summary line
      g->summary(os);
    }

    const intmax_t total_bytes_per_day = bytes / Config::days;
    const double total_arts_per_day = (double)articles / Config::days;
    const long total_posters = senders.size();

    os << "<tfoot>\n";
    os << "<tr>\n";
    os << "<td>Total</td>\n";
    os << "<td>"
       << setprecision(total_arts_per_day >= 10 ? 0 : 1) << total_arts_per_day
       << setprecision(6)
       << "</td>\n";
    os << "<td>" << Bytes(total_bytes_per_day) << "</td>\n";
    os << "<td>" << total_posters << "</td>\n";
    os << "</tr>\n";
    os << "</tfoot>\n";
    os << "</table>\n";
    os << "</div>\n";
    Config::footer(os);
    os << flush;
  } catch(ios::failure) {
    fatal(errno, "writing to %s",
          (Config::output + "/" + name + ".html").c_str());
  }
}

void Hierarchy::logs() {
  try {
    ofstream os((Config::output + "/" + name + ".csv").c_str(), ios::app);
    os.exceptions(ofstream::badbit|ofstream::failbit);
    os << Config::end_time
       << ',' << Config::days * 86400
       << ',' << bytes
       << ',' << articles
       << ',' << senders.size()
       << '\n'
       << flush;
  } catch(ios::failure) {
    fatal(errno, "writing to %s", (Config::output + "/" + name +".csv").c_str());
  }
}

void Hierarchy::graphs() {
  graph(name + ".*",
        Config::output + "/" + name + ".csv", 
        Config::output + "/" + name + ".png");
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
