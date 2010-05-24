#include <config.h>
#include "Group.h"
#include "Article.h"
#include "cpputils.h"
#include <fnmatch.h>
#include <iostream>

using namespace std;

Group::Group(const string &name_): name(name_),
                                   articles(0),
                                   bytes(0) {
}

void Group::article(const Article *a) {
  list<string> groupnames;
  a->get_groups(groupnames);
  for(list<string>::const_iterator it = groupnames.begin();
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
    }
  }
}

void Group::set_patterns(const std::string &patternlist) {
  split(patterns, ',', patternlist);
}

bool Group::group_matches(const string &groupname) {
  if(patterns.size()) {
    for(list<string>::const_iterator it = patterns.begin();
        it != patterns.end();
        ++it)
      if(fnmatch(it->c_str(), groupname.c_str(), 0) == 0)
        return true;
    return false;
  } else
    return true;
}

void Group::report() {
  cout << "<tr>\n";
  cout << "<td>";
  html_quote(cout, name) << "</td>\n";
  cout << "<td sorttable_customkey=\"-" << articles << "\">" << articles << "</td>\n";
  cout << "<td sorttable_customkey=\"-" << bytes << "\">";
  format_bytes(cout, bytes) << "</td>\n";
  cout << "<td sorttable_customkey=\"-" << senders.size() << "\">" << senders.size() << "</td>\n";
  cout << "</tr>\n";
}

map<string,Group *> Group::groups;
list<string> Group::patterns;

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
