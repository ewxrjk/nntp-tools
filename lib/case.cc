#include "cpputils.h"
#include <cctype>

using namespace std;

/* Convert S to lower case, in-place, and return a reference to it. */
string &lower(string &s) {
  for(string::size_type n = 0; n < s.size(); ++n)
    s[n] = tolower(s[n]);
  return s;
}

/* Convert S to upper case, in-place, and return a reference to it. */
string &upper(string &s) {
  for(string::size_type n = 0; n < s.size(); ++n)
    s[n] = toupper(s[n]);
  return s;
}
