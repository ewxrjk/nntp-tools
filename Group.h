#ifndef GROUP_H
#define GROUP_H

#include <map>
#include <string>
#include <list>

class Article;

class Group {
public:
  static std::map<std::string,Group *> groups;

  std::string name;
  int articles;                 // count of articles
  long long bytes;              // count of bytes

  std::map<std::string,int> senders;    // sender -> article count

  Group(const std::string &name_);
  static void article(const Article *a);
  void report();

  static void set_patterns(const std::string &pattern);
private:
  static std::list<std::string> patterns;
  static bool group_matches(const std::string &groupname);
};


#endif /* GROUP_H */

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
