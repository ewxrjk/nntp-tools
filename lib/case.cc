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
