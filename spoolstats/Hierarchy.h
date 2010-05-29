#ifndef HIERARCHY_H
#define HIERARCHY_H

class Group;

class Hierarchy: public Bucket {
public:
  // Hierarchy name
  const std::string name;

  Hierarchy(const std::string &name_);

  ~Hierarchy();

  void visit(const Article *a);

  // Generate a summary line for this hierarchy
  void summary(std::ostream &s);

  // Generate a report page for this hiearchy
  void page();

  // Locate a group (which must be in this hierarchy)
  Group *group(const std::string &name);

  // Map of group names to their objects
  std::map<std::string,Group *> groups;
};

#endif /* HIERARCHY_H */

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
