#include "HTML.h"

using namespace std;

namespace HTML {

  ostream &Header::write(ostream &os,
                         const string &title,
                         const char *css,
                         const char *js) {
    os << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">\n";
    os << "<html>\n";
    os << "<head><title>" << Escape(title) << "</title>\n";
    if(css)
      os << "<link rel=StyleSheet type=" << Quote("text/css")
         << " href=" << Quote(css) << ">\n";
    if(js)
      os << "<script src=" << Quote(js) << "></script>\n";
    os << "<body>\n";
    os << "<h1>" << Escape(title) << "</h1>\n";
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
