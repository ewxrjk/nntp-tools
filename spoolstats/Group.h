#ifndef GROUP_H
#define GROUP_H

class Article;

class Group: public SenderCountingBucket {
public:
  std::string name;                     // name of group

  Group(const std::string &name_);

  // Visit one article
  void visit(const Article *a);

  // Generate table line
  void summary(std::ostream &os);
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
