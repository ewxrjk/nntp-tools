#include "Group.h"
#include "Article.h"
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
    if(groups.find(groupname) == groups.end())
      groups[groupname] = new Group(groupname);
    Group *g = groups[groupname];
    g->articles += 1;
    g->bytes += a->get_size();
  }
  static int count;
  cerr << count++ << "\r";
}

void Group::report() {
  cout << name << endl
       << "  articles: " << articles << endl
       << "  bytes:    " << bytes << endl;
}

map<string,Group *> Group::groups;

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
