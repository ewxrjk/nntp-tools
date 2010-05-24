#include <config.h>
#include "cpputils.h"

using namespace std;

void split(list<string> &bits,
           char sep,
           const string &s) {
  string::size_type pos = 0;
  for(;;) {
    string::size_type n = s.find(sep, pos);
    if(n != string::npos) {
      bits.push_back(string(s, pos, n - pos));
      pos = n + 1;
    } else  {
      bits.push_back(string(s, pos, string::npos));
      break;
    }
  }
}

ostream &html_quote(ostream &str, const string &s) {
  for(string::size_type pos = 0; pos < s.size(); ++pos) {
    const unsigned char c = s[pos];
    switch(c) {
    default:
      if(c >= 32 && c <= 126)
        str << c;
      else {
      case '<':
      case '&':
        str << '<' << (int)c << ';';
      }
    }
  }
  return str;
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
