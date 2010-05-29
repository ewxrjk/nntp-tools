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
  Bucket::visit(a);
}

void Hierarchy::summary(ostream &os) {
  const intmax_t bytes_per_day = bytes / Config::days;
  const double arts_per_day = (double)articles / Config::days;
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
  os << "</tr>\n";
  // TODO can we find a better stream state restoration idiom?
}

void Hierarchy::page() {
  ofstream os((Config::output + "/" + name + ".html").c_str());
  os.exceptions(ofstream::badbit|ofstream::failbit);

  os << HTML::Header(name + ".*", "spoolstats.css", "sorttable.js");

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

  os << "<tfoot>\n";
  os << "<tr>\n";
  os << "<td>Total</td>\n";
  os << "<td>"
     << setprecision(total_arts_per_day >= 10 ? 0 : 1) << total_arts_per_day
     << setprecision(6)
     << "</td>\n";
  os << "<td>" << Bytes(total_bytes_per_day) << "</td>\n";
  os << "<td></td>\n";                  // TODO?
  os << "</tr>\n";
  os << "</tfoot>\n";
  os << "</table>\n";
  os << "<p><a href=" << HTML::Quote("index.html") << ">Hierarchies</a>"
     << " | <a href=" << HTML::Quote("allgroups.html") << ">All groups</a>"
     << "</p>\n";
  os << flush;
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
