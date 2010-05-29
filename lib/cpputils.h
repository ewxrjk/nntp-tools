#ifndef CPPUTILS
#define CPPUTILS

#include <vector>
#include <string>
#include <ostream>
#include <stdint.h>

void split(std::vector<std::string> &bits,
           char sep,
           const std::string &s);

class Bytes {
public:
  inline Bytes(intmax_t n): bytes(n) {}
  intmax_t bytes;
  static std::ostream &write(std::ostream &os, intmax_t bytes);
};

inline std::ostream &operator<<(std::ostream &os, const Bytes &b) {
  return b.write(os, b.bytes);
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
