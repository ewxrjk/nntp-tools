#include "HTML.h"

using namespace std;

namespace HTML {

  ostream &Quote::write(ostream &os, const string &str) {
    os << '"' << Escape(str) << '"';
    return os;
  }

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
