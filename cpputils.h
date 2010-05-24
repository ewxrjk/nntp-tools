#ifndef CPPUTILS
#define CPPUTILS

#include <list>
#include <string>
#include <ostream>

void split(std::list<std::string> &bits,
           char sep,
           const std::string &s);

std::ostream &html_quote(std::ostream &str, const std::string &s);

template<typename T>
std::ostream &format_bytes(std::ostream &str, T n) {
  if(n < 2048)
    str << n;
  else if(n < 2048 * 1024)
    str << n / 1024 << "K";
  else if(n < 2048LL * 1024 * 1024)
    str << n / (1024 * 1024) << "M";
  else
    str << n / (1024 * 1024 * 1024) << "G";
  return str;
}

#endif /* CPPUTILS */

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
