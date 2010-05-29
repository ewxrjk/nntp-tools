#ifndef CONFIG__H
#define CONFIG__H

struct Config {
  static bool terminal;
  static time_t start_time;
  static time_t end_time;
  static time_t start_mtime;
  static std::string output;
  static int end_latency;
  static int start_latency;
  static int days;
  static std::string spool;

  // Parse command line
  static void Options(int argc, char **argv);

  // Set of hierarchies to analyse
  static std::map<std::string, Hierarchy *> hierarchies;

private:
  // Add a hierarchy
  static void hierarchy(const std::string &h);

};

#endif /* CONFIG__H */

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
