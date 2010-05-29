#include "cpputils.h"

std::ostream &Bytes::write(std::ostream &os, intmax_t bytes) {
  if(bytes < 2048)
    os << bytes;
  else if(bytes < 2048 * 1024)
    os << bytes / 1024 << "K";
  else if(bytes < 2048LL * 1024 * 1024)
    os << bytes / (1024 * 1024) << "M";
  else
    os << bytes / (1024 * 1024 * 1024) << "G";
  return os;
}

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
