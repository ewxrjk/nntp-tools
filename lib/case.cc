#include "cpputils.h"
#include <cctype>

using namespace std;

string &lower(string &s) {
  for(string::size_type n = 0; n < s.size(); ++n)
    s[n] = tolower(s[n]);
  return s;
}

string &upper(string &s) {
  for(string::size_type n = 0; n < s.size(); ++n)
    s[n] = toupper(s[n]);
  return s;
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
