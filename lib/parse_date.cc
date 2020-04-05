/*
 * This file is part of rjk-nntp-tools.
 * Copyright (C) 2010-11, 2014 Richard Kettlewell
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
#include "cpputils.h"
#include "utils.h"
#include <iostream>
#include <cstring>
#include "Timezones.h"

using namespace std;

static bool parse_date_std(const string &d, struct tm &bdt, int &zone);
static void skip_ws(const string &s, string::size_type &pos);
static bool parse_word(const string &s, string::size_type &pos, string &word);
static bool parse_int(const string &s, string::size_type &pos, int &n);

// Parse a date string and return it as a time_t
time_t parse_date(const string &d, bool warn) {
  struct tm bdt;
  int zone;

  if(!parse_date_std(d, bdt, zone)) {
    if(warn)
      cerr << "Cannot parse date: '" << d << "'" << endl;
    return 0;
  }
  time_t when = timegm(&bdt);
  when -= zone; // timezone adjustment
  // cerr << d << " ---> " << ctime(&when);
  return when;
}

// Parse a date string and return it as a struct tm and a timezone offset
static bool parse_date_std(const string &d, struct tm &bdt, int &zone) {
  static const string months[] = {
      "jan", "feb", "mar", "apr", "may", "jun",
      "jul", "aug", "sep", "oct", "nov", "dec",
  };

  // Possible date formats:
  //    ???, DD MMM YYYY HH:MM:SS +ZZZZ [(ZZZ)]
  //    ???, DD MMM YYYY HH:MM:SS ZZZ
  //    DD MMM YYYY HH:MM:SS +ZZZZ [(ZZZ)]
  //    DD MMM YYYY HH:MM:SS ZZZ

  // TODO: Cannot parse date: 'Saturday, 29 May 2010 00:29:01 +000'

  memset(&bdt, 0, sizeof bdt);
  zone = 0;
  // Skip "day,"
  string::size_type pos = 0;
  string word;
  if(parse_word(d, pos, word) && pos < d.size() && d[pos] == ',')
    ++pos;
  else
    pos = 0;
  skip_ws(d, pos);
  if(!parse_int(d, pos, bdt.tm_mday) || bdt.tm_mday <= 0 || bdt.tm_mday > 31) {
    if(debug)
      cerr << "bad mday" << endl;
    return false;
  }
  skip_ws(d, pos);
  if(!parse_word(d, pos, word)) {
    if(debug)
      cerr << "bad month" << endl;
    return false;
  }
  lower(word);
  bdt.tm_mon = -1;
  for(int n = 0; n < 12; ++n)
    if(word == months[n])
      bdt.tm_mon = n; // NB 0-based months
  if(bdt.tm_mon == -1) {
    if(debug)
      cerr << "bad month (lookup failed)" << endl;
    return false;
  }
  skip_ws(d, pos);
  if(!parse_int(d, pos, bdt.tm_year)) {
    if(debug)
      cerr << "bad year" << endl;
    return false;
  }
  if(bdt.tm_year >= 1900)
    bdt.tm_year -= 1900;
  skip_ws(d, pos);
  if(!parse_int(d, pos, bdt.tm_hour)) {
    if(debug)
      cerr << "bad hour" << endl;
    return false;
  }
  if(pos >= d.size() || d[pos] != ':') {
    if(debug)
      cerr << "missing colon" << endl;
    return false;
  }
  ++pos;
  if(!parse_int(d, pos, bdt.tm_min)) {
    if(debug)
      cerr << "bad minute" << endl;
    return false;
  }
  if(pos < d.size() && d[pos] == ':') {
    ++pos;
    if(!parse_int(d, pos, bdt.tm_sec)) {
      if(debug)
        cerr << "bad minute" << endl;
      return false;
    }
  }
  skip_ws(d, pos);
  if(pos < d.size()) {
    if(d[pos] == '+' || d[pos] == '-') {
      if(pos + 4 >= d.size()) {
        if(debug)
          cerr << "short zone" << endl;
        return false;
      }
      if(!isdigit(d[pos + 1]) || !isdigit(d[pos + 2]) || !isdigit(d[pos + 3])
         || !isdigit(d[pos + 4])) {
        if(debug)
          cerr << "nonnumeric zone" << endl;
        return false;
      }
      int zh = d[pos + 1] * 10 + d[pos + 2] - 11 * '0';
      int zm = d[pos + 3] * 10 + d[pos + 4] - 11 * '0';
      zone = zh * 3600 + zm * 60;
      if(d[pos] == '-')
        zone = -zone;
    } else {
      string zonename;
      if(!parse_word(d, pos, zonename)) {
        if(debug)
          cerr << "bad zone name" << endl;
        return false;
      }
      upper(zonename);
      if(!Timezones::zones.exists(zonename)) {
        if(debug)
          cerr << "unknown zone '" << zonename << "'" << endl;
        return false;
      }
      zone = Timezones::zones[zonename];
    }
  } else
    zone = 0; // assume UTC
  return true;
}

// Move POS forward past whitespace in S
static void skip_ws(const string &s, string::size_type &pos) {
  pos = s.find_first_not_of(" \t", pos);
}

// Move POS forward past a word in S, returning it via WORD
static bool parse_word(const string &s, string::size_type &pos, string &word) {
  bool good = false;
  word.clear();
  while(pos < s.size() && isalpha(s[pos])) {
    word += s[pos++];
    good = true;
  }
  return good;
}

// Move POS forward past a decimal integer in S, returning it via N
static bool parse_int(const string &s, string::size_type &pos, int &n) {
  bool good = false;
  n = 0;
  while(pos < s.size() && isdigit(s[pos])) {
    n = 10 * n + (s[pos++] - '0');
    good = true;
  }
  return good;
}
