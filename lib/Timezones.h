#ifndef TIMEZONES_H
#define TIMEZONES_H

#include <string>
#include <map>

class Timezones {
public:
  inline int operator[](const std::string &s) const {
    return timezones.find(s)->second;
  }
  inline bool exists(const std::string &s) const {
    return timezones.find(s) != timezones.end();
  }
  static Timezones zones;
private:
  Timezones();
  std::map<std::string,int> timezones;
};

#endif /* TIMEZONES_H */

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
