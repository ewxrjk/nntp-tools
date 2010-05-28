#include "spoolstats.h"

using namespace std;

Article::Article(const string &text): cached_date(-1) {
  parse(text);
}

int Article::visit(const string &text,
                    time_t start_time,
                    time_t end_time) {
  Article *a = new Article(text);
  if(seen.find(a->mid()) == seen.end()
     && a->date() >= start_time
     && a->date() < end_time) {
    seen[a->mid()] = true;
    if(Group::article(a))
      return 1;
  }
  delete a;
  return 0;
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
  bytes = text.size();
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

string &Article::lower(string &s) {
  for(string::size_type n = 0; n < s.size(); ++n)
    s[n] = tolower(s[n]);
  return s;
}

string &Article::upper(string &s) {
  for(string::size_type n = 0; n < s.size(); ++n)
    s[n] = toupper(s[n]);
  return s;
}

time_t Article::parse_date(const string &d) {
  struct tm bdt;
  int zone;

  if(!parse_date_std(d, bdt, zone)) {
    cerr << "Cannot parse date: '" << d << "'" << endl;
    return 0;
  }
  time_t when = timegm(&bdt);
  when -= zone;               // timezone adjustment
  //cerr << d << " ---> " << ctime(&when);
  return when;
}

bool Article::parse_date_std(const string &d, struct tm &bdt, int &zone) {
  static const string months[] = {
    "jan", "feb", "mar", "apr",
    "may", "jun", "jul", "aug",
    "sep", "oct", "nov", "dec",
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
      bdt.tm_mon = n;              // NB 0-based months
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
      if(!isdigit(d[pos + 1])
         || !isdigit(d[pos + 2])
         || !isdigit(d[pos + 3])
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
      if(timezones.find(zonename) == timezones.end()) {
        if(debug)
          cerr << "unknown zone '" << zonename << "'" << endl;
        return false;
      }
      zone = timezones[zonename];
    }
  } else
    zone = 0;                 // assume UTC
  return true;
}

void Article::skip_ws(const string &s, string::size_type &pos) {
  pos = s.find_first_not_of(" \t", pos);
}

bool Article::parse_word(const string &s, string::size_type &pos, string &word) {
  bool good = false;
  word.clear();
  while(pos < s.size() && isalpha(s[pos])) {
    word += s[pos++];
    good = true;
  }
  return good;
}

bool Article::parse_int(const string &s, string::size_type &pos, int &n) {
  bool good = false;
  n = 0;
  while(pos < s.size() && isdigit(s[pos])) {
    n = 10 * n + (s[pos++] - '0');
    good = true;
  }
  return good;
}

//map<string,Article *> Article::articles;
map<string,bool> Article::seen;

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
