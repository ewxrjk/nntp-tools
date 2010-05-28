#include "spoolstats.h"

using namespace std;

Group::Group(const string &name_): name(name_),
                                   articles(0),
                                   bytes(0) {
}

int Group::article(const Article *a) {
  int r = 0;
  vector<string> groupnames;
  a->get_groups(groupnames);
  for(vector<string>::const_iterator it = groupnames.begin();
      it != groupnames.end();
      ++it) {
    const string &groupname = *it;
    if(group_matches(groupname)) {
      if(groups.find(groupname) == groups.end())
        groups[groupname] = new Group(groupname);
      Group *g = groups[groupname];
      g->articles += 1;
      g->bytes += a->get_size();
      const string &sender = a->sender();
      if(g->senders.find(sender) == g->senders.end())
        g->senders[sender] = 1;
      else
        ++g->senders[sender];
      r = 1;
    }
  }
  return r;
}

bool Group::group_matches(const string &groupname) {
  string::size_type n = groupname.find('.');
  if(n == string::npos)
    return false;
  return hierarchies.find(string(groupname, 0, n)) != hierarchies.end();
}

void Group::report(int days) {
  const long bytes_per_day = bytes / days;
  const double arts_per_day = (double)articles / days;
  cout << "<tr>\n";
  cout << "<td>";
  html_quote(cout, name) << "</td>\n";
  cout << "<td sorttable_customkey=\"-" 
       << fixed << arts_per_day << "\">" 
       << fixed << setprecision(arts_per_day >= 10 ? 0 : 1) << arts_per_day
       << fixed << setprecision(6)
       << "</td>\n";
  cout << "<td sorttable_customkey=\"-" << bytes_per_day << "\">";
  format_bytes(cout, bytes_per_day) << "</td>\n";
  cout << "<td sorttable_customkey=\"-" << senders.size() << "\">" << senders.size() << "</td>\n";
  cout << "</tr>\n";
}

map<string,Group *> Group::groups;
set<string> Group::hierarchies;

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
