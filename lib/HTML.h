//-*-C++-*-
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
#ifndef HTML_H
#define HTML_H

#include <string>
#include <ostream>

namespace HTML {
/* os << Escape(s) writes S to os with escaping */
class Escape {
public:
  inline Escape(const std::string &s): str(s) {}
  const std::string &str;
  static std::ostream &write(std::ostream &os, const std::string &str);
};

inline std::ostream &operator<<(std::ostream &os, Escape e) {
  return e.write(os, e.str);
}

/* os << Quote(s) writes S to os with quoting and escaping (i.e. suitable for
 * use as an attribute value) */
class Quote {
public:
  inline Quote(const std::string &s): str(s) {}
  const std::string &str;
  static std::ostream &write(std::ostream &os, const std::string &str);
};

inline std::ostream &operator<<(std::ostream &os, const Quote &e) {
  return e.write(os, e.str);
}

/* os << HEader(title, css, js) writes an HTML header to OS.  TITLE is the
 * title and also the contents of an initial H1, CSS an optional style sheet
 * to embed and JS an optional fragment of Javascript. */
class Header {
public:
  inline Header(const std::string &t, const char *c = NULL,
                const char *j = NULL):
      title(t),
      css(c), js(j) {}
  const std::string &title;
  const char *css;
  const char *js;
  static std::ostream &write(std::ostream &os, const std::string &title,
                             const char *css, const char *js);
};

inline std::ostream &operator<<(std::ostream &os, const Header &h) {
  return h.write(os, h.title, h.css, h.js);
}

void thead(std::ostream &os, const char *heading, ...);

}; // namespace HTML

#endif /* HTML_H */
