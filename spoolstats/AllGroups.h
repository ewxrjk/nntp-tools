#ifndef ALL_H
#define ALL_H

class Hierarchy;

class AllGroups: public Bucket {
public:
  ~AllGroups();

  // Visit one article
  void visit(const Article *a);

  // Scan the spool
  void scan();

  // Generate all reports
  void report();

private:
  // Message IDs that have been seen
  std::set<std::string> seen;

  // Recurse into one directory
  void recurse(const std::string &dir);

  // Visit one article by name.  Returns 1 if article used, else 0.
  int visit(const std::string &path);

  // Generate the hierarchies report
  void report_hierarchies();

  // Generate the groups report
  void report_groups();
};

#endif /* ALL_H */

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
