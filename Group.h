#ifndef GROUP_H
#define GROUP_H

#include <map>
#include <string>

class Article;

class Group {
public:
  static std::map<std::string,Group *> groups;

  std::string name;
  int articles;                 // count of articles
  long long bytes;              // count of bytes

  Group(const std::string &name_);
  static void article(const Article *a);
  void report();
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
