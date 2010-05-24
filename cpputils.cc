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

