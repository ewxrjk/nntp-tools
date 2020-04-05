/*
 * This file is part of rjk-nntp-tools.
 * Copyright (C) 2010-12 Richard Kettlewell
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
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>

using namespace std;

AllGroups::~AllGroups() {}

// Visit one article
void AllGroups::visit(const Article *a) {
  Bucket::visit(a);
  useragents.update(a, a->useragent());
  charsets.update(a, a->charset());
}

// Scan the spool
void AllGroups::scan() {
  for(map<string, Hierarchy *>::const_iterator it = Config::hierarchies.begin();
      it != Config::hierarchies.end(); ++it) {
    Hierarchy *const h = it->second;
    recurse(Config::spool + "/" + h->name);
    // TODO we could report and delete h here if we introduced an end_mtime.
  }
  // AllGroups::recurse() keeps a running count, erase it now we're done
  if(Config::terminal)
    cerr << "                                                                  "
            "      \r";
}

// Recurse into one directory
void AllGroups::recurse(const string &dir) {
  static int count, included;
  DIR *dp;
  struct dirent *de;
  string nodepath;
  struct stat sb;
  long low_water_mark = -1;

  ++dirs;
  if(!(dp = opendir(dir.c_str())))
    fatal(errno, "opening %s", dir.c_str());
  errno = 0;
  while((de = readdir(dp))) {
    if(de->d_name[0] != '.') {
      if(Config::terminal && count % 31 == 0)
        cerr << included << "/" << count << " skip-lwm: " << skip_lwm
             << " skip-mtime: " << skip_mtime << " dirs: " << dirs << "\r";
      // Convert filename to article number
      errno = 0;
      char *end;
      long article = strtol(de->d_name, &end, 10);
      if(errno || end == de->d_name || *end)
        article = -1;
      else if(article < low_water_mark) {
        ++count;
        ++skip_lwm;
        continue;
      }
      nodepath = dir;
      nodepath += "/";
      nodepath += de->d_name;
      if(stat(nodepath.c_str(), &sb) < 0)
        fatal(errno, "stat %s", nodepath.c_str());
      if(S_ISDIR(sb.st_mode))
        recurse(nodepath);
      else if(S_ISREG(sb.st_mode)) {
        // Skip articles that precede articles known to be too early by mtime
        if(article >= 0) {
          if(sb.st_mtime >= Config::start_mtime)
            included += visit(nodepath);
          else {
            low_water_mark = article;
            ++skip_mtime;
          }
        }
        count += 1;
      }
    }
    errno = 0; // stupid readdir() API
  }
  if(errno)
    fatal(errno, "reading %s", dir.c_str());
  closedir(dp);
}

// Visit one article by name
int AllGroups::visit(const string &path) {
  int fd, bytes_read;
  string article;
  struct stat sb;
  char buffer[2048];

  if((fd = open(path.c_str(), O_RDONLY)) < 0)
    fatal(errno, "opening %s", path.c_str());
  if(fstat(fd, &sb) < 0)
    fatal(errno, "stat %s", path.c_str());
  while((bytes_read = read(fd, buffer, sizeof buffer)) > 0) {
    article.append(buffer, bytes_read);
    if(article.find("\n\n") != string::npos
       || article.find("\r\n\r\n") != string::npos)
      break;
  }
  if(bytes_read < 0)
    fatal(errno, "reading %s", path.c_str());
  close(fd);
  if(debug)
    cerr << "article " << path << endl;
  // Parse article
  Article a(article, sb.st_size);
  // Reject articles outside the sampling range
  if(a.date() < Config::start_time || a.date() >= Config::end_time)
    return 0;
  // Only visit each article once
  if(seen.find(a.mid()) != seen.end())
    return 0;
  seen.insert(a.mid());
  // Supply article to global bucket (AllGroups)
  visit(&a);
  // Get list of groups
  vector<string> groups;
  a.get_groups(groups);
  // Order the list, so we can easily de-dupe
  sort(groups.begin(), groups.end());
  const Hierarchy *last_h = NULL;
  int visited = 0;
  for(size_t n = 0; n < groups.size(); ++n) {
    // De-dupe groups
    if(n > 0 && groups[n - 1] == groups[n])
      continue;
    // Identify hierarchy
    const string hname(groups[n], 0, groups[n].find('.'));
    // Eliminate unwanted hierarchies
    const map<string, Hierarchy *>::const_iterator it =
        Config::hierarchies.find(hname); // TODO encapsulate in Config
    if(it == Config::hierarchies.end())
      continue;
    Hierarchy *const h = it->second;
    // Add to group data
    h->group(groups[n])->visit(&a);
    visited = 1;
    // De-dupe hierarchy
    if(h == last_h)
      continue;
    // Add to hierarchy data
    h->visit(&a);
    // Remember hierarchy for next time
    last_h = h;
  }
  return visited;
}

// Generate all reports
void AllGroups::report() {
  report_hierarchies();
  report_groups();
  report_agents((Config::output + "/agents.html"), false);
  report_agents((Config::output + "/agents-summary.html"), true);
  report_charsets();
  for(map<string, Hierarchy *>::const_iterator it = Config::hierarchies.begin();
      it != Config::hierarchies.end(); ++it) {
    Hierarchy *const h = it->second;
    h->page();
  }
}

void AllGroups::report_hierarchies() {
  // TODO lots of scope to de-dupe with Hierarchy::page() here
  try {
    ofstream os((Config::output + "/index.html").c_str());
    os.exceptions(ofstream::badbit | ofstream::failbit);

    os << HTML::Header("Spool report", "spoolstats.css", "sorttable.js");

    os << "<h2>History</h2>\n";
    os << "<div>\n";
    os << "<p class=graph><a href=" << HTML::Quote("all.png") << ">"
       << "<img src=" << HTML::Quote("all.png") << ">"
       << "</a></p>\n";
    os << "</div>\n";

    os << "<h2>Last " << Config::days << " days</h2>\n";
    os << "<div>\n";
    os << "<table class=sortable>\n";

    HTML::thead(os, "Hierarchy", "Articles/day", "Bytes/day", "Posters",
                (const char *)NULL);

    for(map<string, Hierarchy *>::const_iterator it =
            Config::hierarchies.begin();
        it != Config::hierarchies.end(); ++it) {
      Hierarchy *const h = it->second;
      h->summary(os);
    }

    const intmax_t total_bytes_per_day = bytes / Config::days;
    const double total_arts_per_day = (double)articles / Config::days;

    os << "<tfoot>\n";
    os << "<tr>\n";
    os << "<td><a href=" << HTML::Quote("allgroups.html") << ">"
       << "All groups</a></td>\n";
    os << "<td>" << setprecision(total_arts_per_day >= 10 ? 0 : 1)
       << total_arts_per_day << setprecision(6) << "</td>\n";
    os << "<td>" << round_kilo(total_bytes_per_day) << "</td>\n";
    os << "</tr>\n";
    os << "</tfoot>\n";
    os << "</table>\n";
    os << "</div>\n";
    Config::footer(os);
    os << flush;
  } catch(ios::failure &) {
    fatal(errno, "writing to %s", (Config::output + "/index.html").c_str());
  }
}

void AllGroups::report_groups() {
  try {
    ofstream os((Config::output + "/allgroups.html").c_str());
    os.exceptions(ofstream::badbit | ofstream::failbit);

    os << HTML::Header("All groups", "spoolstats.css", "sorttable.js");

    os << "<h2>Last " << Config::days << " days</h2>\n";
    os << "<div>\n";
    os << "<table class=sortable>\n";

    HTML::thead(os, "Group", "Articles/day", "Bytes/day", "Posters",
                (const char *)NULL);

    for(map<string, Hierarchy *>::const_iterator jt =
            Config::hierarchies.begin();
        jt != Config::hierarchies.end(); ++jt) {
      Hierarchy *const h = jt->second;
      for(map<string, Group *>::const_iterator it = h->groups.begin();
          it != h->groups.end(); ++it) {
        Group *g = it->second; // Summary line
        g->summary(os);
      }
    }

    const intmax_t total_bytes_per_day = bytes / Config::days;
    const double total_arts_per_day = (double)articles / Config::days;

    os << "<tfoot>\n";
    os << "<tr>\n";
    os << "<td>Total</td>\n";
    os << "<td>" << setprecision(total_arts_per_day >= 10 ? 0 : 1)
       << total_arts_per_day << setprecision(6) << "</td>\n";
    os << "<td>" << round_kilo(total_bytes_per_day) << "</td>\n";
    os << "<td></td>\n"; // TODO?
    os << "</tr>\n";
    os << "</tfoot>\n";
    os << "</table>\n";
    os << "</div>\n";
    Config::footer(os);
    os << flush;
  } catch(ios::failure &) {
    fatal(errno, "writing to %s", (Config::output + "/allgroups.html").c_str());
  }
}

void AllGroups::logs() {
  for(map<string, Hierarchy *>::const_iterator it = Config::hierarchies.begin();
      it != Config::hierarchies.end(); ++it) {
    Hierarchy *const h = it->second;
    h->logs();
  }
  try {
    ofstream os((Config::output + "/all.csv").c_str(), ios::app);
    os.exceptions(ofstream::badbit | ofstream::failbit);
    os << Config::end_time << ',' << Config::days * 86400 << ',' << bytes << ','
       << articles << '\n'
       << flush;
  } catch(ios::failure &) {
    fatal(errno, "writing to %s", (Config::output + "/all.csv").c_str());
  }
  charsets.logs(Config::output + "/encodings.csv");
  useragents.logs(Config::output + "/useragents.csv");
}

void AllGroups::readLogs() {
  for(map<string, Hierarchy *>::const_iterator it = Config::hierarchies.begin();
      it != Config::hierarchies.end(); ++it) {
    Hierarchy *const h = it->second;
    h->readLogs();
  }
  vector<vector<Value>> rows;
  read_csv(Config::output + "/all.csv", rows);
  if(rows.size()) {
    const vector<Value> &last = rows.back();
    bytes = last[2];
    articles = last[3];
  }
  charsets.readLogs(Config::output + "/encodings.csv");
  useragents.readLogs(Config::output + "/useragents.csv");
}

void AllGroups::graphs() {
  for(map<string, Hierarchy *>::const_iterator it = Config::hierarchies.begin();
      it != Config::hierarchies.end(); ++it) {
    Hierarchy *const h = it->second;
    h->graphs();
  }
  graph("All groups", Config::output + "/all.csv", Config::output + "/all.png");
}

void AllGroups::report_agents(const std::string &path, bool summarized) {
  try {
    ofstream os(path.c_str());
    os.exceptions(ofstream::badbit | ofstream::failbit);

    os << HTML::Header("User agents", "spoolstats.css", "sorttable.js");

    os << "<table class=sortable>\n";
    HTML::thead(os, "User Agent", "Articles", "Posters", (const char *)NULL);

    ArticleProperty *uas = NULL;
    ArticleProperty summarized_uas;
    if(summarized) {
      useragents.summarize(summarized_uas, AllGroups::summarize);
      uas = &summarized_uas;
    } else
      uas = &useragents;
    vector<const ArticleProperty::PropertyValue *> agents;
    uas->order(agents);

    for(unsigned n = 0; n < agents.size(); ++n) {
      os << "<tr>\n";
      os << "<td>" << HTML::Escape(agents[n]->value) << "</td>\n";
      os << "<td sorttable_customkey=-" << fixed << agents[n]->articles << ">"
         << agents[n]->articles << "</td>\n";
      os << "<td sorttable_customkey=-" << fixed << agents[n]->senderCount
         << ">" << agents[n]->senderCount << "</td>\n";
      os << "</tr>\n";
    }

    os << "</table>\n";
    Config::footer(os);
    os << flush;
  } catch(ios::failure &) {
    fatal(errno, "writing to %s", path.c_str());
  }
}

void AllGroups::report_charsets() {
  const string path = Config::output + "/charsets.html";
  try {
    ofstream os(path.c_str());
    os.exceptions(ofstream::badbit | ofstream::failbit);

    os << HTML::Header("Character Encodings", "spoolstats.css", "sorttable.js");

    os << "<table class=sortable>\n";
    HTML::thead(os, "Character Encoding", "Articles", "Posters",
                (const char *)NULL);

    vector<const ArticleProperty::PropertyValue *> charsets_o;
    charsets.order(charsets_o);

    for(unsigned n = 0; n < charsets_o.size(); ++n) {
      os << "<tr>\n";
      os << "<td>" << HTML::Escape(charsets_o[n]->value) << "</td>\n";
      os << "<td sorttable_customkey=-" << fixed << charsets_o[n]->articles
         << ">" << charsets_o[n]->articles << "</td>\n";
      os << "<td sorttable_customkey=-" << fixed << charsets_o[n]->senderCount
         << ">" << charsets_o[n]->senderCount << "</td>\n";
      os << "</tr>\n";
    }

    os << "</table>\n";
    Config::footer(os);
    os << flush;
  } catch(ios::failure &) {
    fatal(errno, "writing to %s", path.c_str());
  }
}

const string &AllGroups::summarize(const string &ua) {
  struct agent {
    const char *substring;
    const std::string name;
  };
  static struct agent clients[] = {
      {"40tude_Dialog", "40tude_Dialog"},
      {"Alpine", "Alpine"},
      {"alpine", "Alpine"},
      {"Apple Mail", "Apple Mail"},
      {"Claws Mail", "Claws Mail"},
      {"Direct Read News", "Direct Read News"},
      {"Emas", "Emas"},
      {"Forte Agent", "Forte Agent"},
      {"ForteAgent", "Forte Agent"},
      {"Forte Free Agent", "Forte Agent"},
      {"^G2", "Google Groups"},
      {"Gnus", "Gnus"},
      {"^gnus", "Gnus"},
      {"Groundhog Newsreader for Android", "Groundhog Newsreader for Android"},
      {"Hamster", "Hamster"},
      {"Hogwasher", "Hogwasher"},
      {"JetBrains Omea Reader", "JetBrains Omea Reader"},
      {"knews", "knews"},
      {"KNode", "KNode"},
      {"Lotus Notes", "Lotus Notes"},
      {"MacSOUP", "MacSOUP"},
      {"MesNews", "MesNews"},
      {"Messenger-Pro", "Messenger-Pro"},
      {"Michi Buster", "Michi Buster"},
      {"MicroPlanet Gravity", "MicroPlanet Gravity"},
      {"MicroPlanet-Gravity", "MicroPlanet Gravity"},
      {"Microsoft Outlook Express", "Outlook Express"},
      {"Microsoft-Outlook-©Express", "Outlook Express"},
      {"Microsoft Windows Mail", "Outlook Express"},
      {"Microsoft Windows Live Mail", "Outlook Express"},
      {"Microsoft Internet News", "Microsoft Internet News"},
      {"Microsoft-Entourage", "Microsoft Entourage"},
      {"NewsWatcher", "NewsWatcher"},
      {"Mutt", "Mutt"},
      {"Netscape", "Netscape"},
      {"NewsHound", "NewsHound"},
      {"NewsLeecher", "NewsLeecher"},
      {"NewsPortal", "NewsPortal"},
      {"NewsTap", "NewsTap"},
      {"Noworyta News Reader", "Noworyta News Reader"},
      {"Opera", "Opera"},
      {"Pan", "Pan"},
      {"^pan ", "Pan"},
      {"Pegasus Mail", "Pegasus Mail"},
      {"Pluto", "Pluto"},
      {"PMINews", "PMINews"},
      {"ProNews", "ProNews"},
      {"SeaMonkey", "SeaMonkey"},
      {"Iceape", "SeaMonkey"}, // Debian
      {"slrn", "slrn"},
      {"Sylpheed", "Sylpheed"},
      {"Thoth", "Thoth"},
      {"Thunderbird", "Thunderbird"},
      {"Lanikai", "Thunderbird"},  // early 2010 preview
      {"Shredder", "Thunderbird"}, // mid 2008 preview
      {"Icedove", "Thunderbird"},  // Debian
      {"^tin", "tin"},
      {"^TIN", "tin"},
      {"TRAVEL.com", "TRAVEL.com"},
      {"trn", "trn"},
      {"Turnpike", "Turnpike"},
      {"^U <", "Turnpike"}, // wibble!
      {"Unison", "Unison"},
      {"XanaNews", "XanaNews"},
      {"Xnews", "Xnews"},
      {"XPN", "XPN"},
      {"^NN/", "NN"},
      {"^NN ", "NN"},
      {"^nn/", "NN"},
      {"^nn ", "NN"},
      {"WinVN", "WinVN"},
      {"News Xpress", "News Xpress"},
      {"NewsAgent", "NewsAgent"},
      {"PC Piggy News", "PC Piggy News"},
      {"MATLAB Central Newsreader", "MATLAB Central Newsreader"},
      {"Flrn", "Flrn"},
      {"Loom", "Loom"},
      {"iForth", "iForth"},
      {"SquirrelMail", "SquirrelMail"},
      {"NewsMan Pro", "NewsMan Pro"},
      {"News Rover", "News Rover"},
      {"AspNNTP", "AspNNTP"},
      {"Grepler", "Grepler"},
      {"^xrn", "xrn"},
      {"Web-News", "Web-News"},
      {"Marcel", "Marcel"},
      {"Gemini", "Gemini"},
      {"newsSync", "newsSync"},
      {"FUDforum", "FUDforum"},
      {"KMail", "KMail"},
      {"Pineapple News", "Pineapple News"},
      {"Evolution", "Evolution"},
      {"Internet Messaging Program (IMP)", "Internet Messaging Program (IMP)"},
      {"MR/2", "MR/2"},
      {"postfaq", "postfaq"},
      {"Virtual Access", "Virtual Access"},
      {"PenguinReader", "PenguinReader"},
      {"http://www.umailcampaign.com", "http://www.umailcampaign.com"},
      {"YahooMailWebService", "YahooMailWebService"},
      {"^Mozilla",
       "Mozilla-compatible browser"}, // everyone claims to be mozilla!
  };
  // By 'summarize' we mean we throw away version and platform information and
  // just identify the client.  Mostly we do substring match but for very short
  // substrings we anchor to the start of the string to avoid false +ves.
  for(unsigned n = 0; n < sizeof clients / sizeof *clients; ++n) {
    if(clients[n].substring[0] == '^') {
      if(ua.find(clients[n].substring + 1) == 0)
        return clients[n].name;
    } else {
      if(ua.find(clients[n].substring) != string::npos)
        return clients[n].name;
    }
  }
  return ua;
}
