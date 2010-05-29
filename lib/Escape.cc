#include "HTML.h"

using namespace std;

namespace HTML {

  ostream &Escape::write(ostream &os, const string &str) {
    for(string::size_type pos = 0; pos < str.size(); ++pos) {
      const unsigned char c = str[pos];
      switch(c) {
      default:
        if(c >= 32 && c <= 126)
          os << c;
        else {
        case '"':
        case '<':
        case '&':
          os << '<' << (int)c << ';';
        }
      }
    }
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
