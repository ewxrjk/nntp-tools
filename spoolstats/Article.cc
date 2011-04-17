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
#include "spoolstats.h"

using namespace std;

Article::Article(const string &text, size_t bytes_): bytes(bytes_),
                                                     cached_date(-1) {
  parse(text);
}

void Article::get_groups(vector<string> &groups) const {
  split(groups, ',', headers.find("newsgroups")->second);
}

time_t Article::date() const {
  if(cached_date == -1) {
    string datestr = headers.find("date")->second;
    cached_date = parse_date(datestr);
  }
  return cached_date;
}

void Article::parse(const string &text) {
  string::size_type pos = 0;

  while(pos < text.size()) {
    if(eol(text, pos))
      break;                  // end of headers
    string::size_type header_end = pos;
    while(header_end < text.size() && !eoh(text, header_end))
      ++header_end;
    if(header_end >= text.size())      // truncated, skip
      break;
    // trim trailing whitespace
    string header(text, pos, header_end - pos); // full header
    pos = header_end + eol(text, header_end); // after header+CRLF
    string::size_type colon = header.find(':');
    if(colon == string::npos)
      continue;               // bad header, skip
    string name(header, 0, colon);
    string::size_type s = header.find_first_not_of(" \t", colon + 1);
    string::size_type e = header.size();
    while(e < colon && (header[e-1] == ' ' || header[e-1] == '\t'))
      --e;
    if(s > e)
      s = e;
    string value(header, s, e - s);
    headers[lower(name)] = value;
    if(debug)
      cerr << "  header " << name << endl
           << "        '" << value << "'" << endl;
  }
}

/// end of line?
int Article::eol(const string &text, string::size_type pos) {
  // both LF and CRLF will do (so we can support wire-format and
  // native-format spools)
  if(pos < text.size() && text[pos] == '\n')
    return 1;
  if(pos + 1 < text.size() && text[pos] == '\r' && text[pos + 1] == '\n')
    return 2;
  return 0;
}

// end of header?
int Article::eoh(const string &text, string::size_type pos) {
  string::size_type n = eol(text, pos);
  if(!n)
    return 0;
  pos += n;
  if(pos < text.size()
     && (text[pos] == ' ' || text[pos] == '\t'))
    return 0;
  return 1;
}

const string &Article::useragent() const {
  map<string, string>::const_iterator it;
  static const string unknown = "(unknown)";

  it = headers.find("user-agent");
  if(it == headers.end())
    it = headers.find("x-newsreader");
  if(it == headers.end())
    it = headers.find("x-mailer");
  if(it == headers.end())
    return unknown;
  else
    return it->second;
}

const string Article::charset() const {
  // RFC2045 s5.1.  This is a rather goofy parse, but we're just totting up the
  // results, not filtering out evil.
  static const char tokenchars[] = "!#$%&'*+-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ^_`abcdefghijklmnopqrstuvwxyz{|}~";
  map<string, string>::const_iterator it;
  it = headers.find("content-type");
  if(it == headers.end())
    return "unknown";
  const string &ct = it->second;
  string::size_type pos = ct.find("charset"), end;
  if(pos == string::npos)
    return "unknown";
  pos += 7;
  if((pos = ct.find_first_not_of(" \t\r\n", pos)) == string::npos)
    return "unknown";
  if(ct[pos++] != '=')
    return "unknown";
  if((pos = ct.find_first_not_of(" \t\r\n", pos)) == string::npos)
    return "unknown";
  if(pos == '"') {
    ++pos;
    if((end = ct.find('"', pos)) == string::npos)
      return "unknown";
  } else {
    if((end = ct.find_first_not_of(tokenchars, pos)) == string::npos)
      end = ct.size();
    if(end == pos)
      return "unknown";
  }
  string r(ct, pos, end - pos);
  for(pos = 0; pos < r.size(); ++pos)
    r[pos] = tolower(r[pos]);
  return r;
}
