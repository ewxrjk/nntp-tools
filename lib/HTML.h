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

  inline std::ostream &operator<<(std::ostream &os, const Quote &e) {
    return e.write(os, e.str);
  }

  class Header {
  public:
    inline Header(const std::string &t,
                  const char *c = NULL,
                  const char *j = NULL): title(t),
                                         css(c),
                                         js(j) {}
    const std::string &title;
    const char *css;
    const char *js;
    static std::ostream &write(std::ostream &os, 
                               const std::string &title,
                               const char *css,
                               const char *js);
  };

  inline std::ostream &operator<<(std::ostream &os, 
                                  const Header &h) {
    return h.write(os, h.title, h.css, h.js);
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
