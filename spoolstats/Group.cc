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
     << Bytes(bytes_per_day) 
     << "</td>\n";
  os << "<td sorttable_customkey=-" << posters << ">" << posters << "</td>\n";
  // TODO can we find a better stream state restoration idiom?
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
