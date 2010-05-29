#include "HTML.h"

using namespace std;

namespace HTML {

  ostream &Header::write(ostream &os,
                         const string &title) {
    os << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">\n";
    os << "<html>\n";
    os << "<head><title>" << Escape(title) << "</title>\n";
    // TODO stylesheet
    os << "<script src=" << Quote("sorttable.js") << "></script>\n"; // TODO parameterize
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
