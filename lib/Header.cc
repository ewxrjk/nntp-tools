/*
 * This file is part of rjk-nntp-tools.
 * Copyright (C) 2010 Richard Kettlewell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */
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
