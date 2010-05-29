#ifndef HTML_H
#define HTML_H

#include <string>
#include <ostream>

namespace HTML {
  class Escape {
  public:
    inline Escape(const std::string &s): str(s) {}
    const std::string &str;
    static std::ostream &write(std::ostream &os, const std::string &str);
  };

  inline std::ostream &operator<<(std::ostream &os, Escape e) {
    return e.write(os, e.str);
  }

  class Quote {
  public:
    inline Quote(const std::string &s): str(s) {}
    const std::string &str;
    static std::ostream &write(std::ostream &os, const std::string &str);
  };

  inline std::ostream &operator<<(std::ostream &os, Quote e) {
    return e.write(os, e.str);
  }

  class Header {
  public:
    inline Header(const std::string &t): title(t) {}
    const std::string &title;
    static std::ostream &write(std::ostream &os, const std::string &title);
  };

  inline std::ostream &operator<<(std::ostream &os, Header h) {
    return h.write(os, h.title);
  }

};

#endif /* HTML_H */

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
