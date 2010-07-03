/*
 * This file is part of spoolstats.
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
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <algorithm>

using namespace std;

AllGroups::~AllGroups() {
}

// Visit one article
void AllGroups::visit(const Article *a) {
  Bucket::visit(a);
  const string &ua = a->useragent();
  const string &sender = a->sender();
  map<string,uadata>::iterator it = useragents.find(ua);
  if(it == useragents.end())
    it = useragents.insert(pair<string,uadata>(ua,uadata(ua))).first;
  ++it->second.articles;
  it->second.senders.insert(sender);
}

// Scan the spool
void AllGroups::scan() {
  for(map<string,Hierarchy *>::const_iterator it = Config::hierarchies.begin();
      it != Config::hierarchies.end();
      ++it) {
    Hierarchy *const h = it->second;
    recurse(Config::spool + "/" + h->name);
    // TODO we could report and delete h here if we introduced an end_mtime.
  }
  // AllGroups::recurse() keeps a running count, erase it now we're done
  if(Config::terminal)
    cerr << "                    \r";
}

// Recurse into one directory
void AllGroups::recurse(const string &dir) {
  static int count, included;
  DIR *dp;
  struct dirent *de;
  string nodepath;
  struct stat sb;

  if(!(dp = opendir(dir.c_str())))
    fatal(errno, "opening %s", dir.c_str());
  errno = 0;
  while((de = readdir(dp))) {
    if(de->d_name[0] != '.') {
      // TODO we have a further possible optimization; if the article name is
      // numerically lower than one known to be too early by mtime, we can skip
      // it before we even stat it.
      nodepath = dir;
      nodepath += "/";
      nodepath += de->d_name;
      if(stat(nodepath.c_str(), &sb) < 0)
        fatal(errno, "stat %s", nodepath.c_str());
      if(S_ISDIR(sb.st_mode))
        recurse(nodepath);
      else if(S_ISREG(sb.st_mode)) {
        if(sb.st_mtime >= Config::start_mtime)
          included += visit(nodepath);
        count += 1;
        if(Config::terminal && count % 11 == 0)
          cerr << included << "/" << count << "\r";
      }
    }
    errno = 0;                          // stupid readdir() API
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
    if(n > 0 && groups[n-1] == groups[n])
      continue;
    // Identify hierarchy
    const string hname(groups[n], 0, groups[n].find('.'));
    // Eliminate unwanted hierarchies
    const map<string,Hierarchy *>::const_iterator it
      = Config::hierarchies.find(hname); // TODO encapsulate in Config
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
  for(map<string,Hierarchy *>::const_iterator it = Config::hierarchies.begin();
      it != Config::hierarchies.end();
      ++it) {
    Hierarchy *const h = it->second;
    h->page();
  }
}

void AllGroups::report_hierarchies() {
  // TODO lots of scope to de-dupe with Hierarchy::page() here
  try {
    ofstream os((Config::output + "/index.html").c_str());
    os.exceptions(ofstream::badbit|ofstream::failbit);

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

    for(map<string,Hierarchy *>::const_iterator it = Config::hierarchies.begin();
        it != Config::hierarchies.end();
        ++it) {
      Hierarchy *const h = it->second;
      h->summary(os);
    }

    const intmax_t total_bytes_per_day = bytes / Config::days;
    const double total_arts_per_day = (double)articles / Config::days;

    os << "<tfoot>\n";
    os << "<tr>\n";
    os << "<td><a href=" << HTML::Quote("allgroups.html") << ">"
       << "All groups</a></td>\n";
    os << "<td>"
       << setprecision(total_arts_per_day >= 10 ? 0 : 1) << total_arts_per_day
       << setprecision(6)
       << "</td>\n";
    os << "<td>" << round_kilo(total_bytes_per_day) << "</td>\n";
    os << "</tr>\n";
    os << "</tfoot>\n";
    os << "</table>\n";
    os << "</div>\n";
    Config::footer(os);
    os << flush;
  } catch(ios::failure) {
    fatal(errno, "writing to %s", (Config::output + "/index.html").c_str());
  }
}

void AllGroups::report_groups() {
  try {
    ofstream os((Config::output + "/allgroups.html").c_str());
    os.exceptions(ofstream::badbit|ofstream::failbit);

    os << HTML::Header("All groups", "spoolstats.css", "sorttable.js");

    os << "<h2>Last " << Config::days << " days</h2>\n";
    os << "<div>\n";
    os << "<table class=sortable>\n";

    HTML::thead(os, "Group", "Articles/day", "Bytes/day", "Posters",
                (const char *)NULL);

    for(map<string,Hierarchy *>::const_iterator jt = Config::hierarchies.begin();
        jt != Config::hierarchies.end();
        ++jt) {
      Hierarchy *const h = jt->second;
      for(map<string,Group *>::const_iterator it = h->groups.begin();
          it != h->groups.end();
          ++it) {
        Group *g = it->second;              // Summary line
        g->summary(os);
      }
    }

    const intmax_t total_bytes_per_day = bytes / Config::days;
    const double total_arts_per_day = (double)articles / Config::days;

    os << "<tfoot>\n";
    os << "<tr>\n";
    os << "<td>Total</td>\n";
    os << "<td>"
       << setprecision(total_arts_per_day >= 10 ? 0 : 1) << total_arts_per_day
       << setprecision(6)
       << "</td>\n";
    os << "<td>" << round_kilo(total_bytes_per_day) << "</td>\n";
    os << "<td></td>\n";                  // TODO?
    os << "</tr>\n";
    os << "</tfoot>\n";
    os << "</table>\n";
    os << "</div>\n";
    Config::footer(os);
    os << flush;
  } catch(ios::failure) {
    fatal(errno, "writing to %s", (Config::output + "/allgroups.html").c_str());
  }
}

void AllGroups::logs() {
  for(map<string,Hierarchy *>::const_iterator it = Config::hierarchies.begin();
      it != Config::hierarchies.end();
      ++it) {
    Hierarchy *const h = it->second;
    h->logs();
  }
  try {
    ofstream os((Config::output + "/all.csv").c_str(), ios::app);
    os.exceptions(ofstream::badbit|ofstream::failbit);
    os << Config::end_time
       << ',' << Config::days * 86400
       << ',' << bytes
       << ',' << articles
       << '\n'
       << flush;
  } catch(ios::failure) {
    fatal(errno, "writing to %s", (Config::output + "/all.csv").c_str());
  }
}

void AllGroups::graphs() {
  for(map<string,Hierarchy *>::const_iterator it = Config::hierarchies.begin();
      it != Config::hierarchies.end();
      ++it) {
    Hierarchy *const h = it->second;
    h->graphs();
  }
  graph("All groups",
        Config::output + "/all.csv",
        Config::output + "/all.png");
}

void AllGroups::report_agents(const std::string &path,
                              bool summarized) {
  try {
    ofstream os(path.c_str());
    os.exceptions(ofstream::badbit|ofstream::failbit);

    os << HTML::Header("User agents", "spoolstats.css", "sorttable.js");

    os << "<table class=sortable>\n";
    HTML::thead(os, "User Agent", "Articles", "Posters", (const char *)NULL);

    map<string,uadata> *uas = NULL;
    map<string,uadata> summarized_uas;
    if(summarized) {
      // Summarize
      for(map<string,uadata>::const_iterator it = useragents.begin();
          it != useragents.end();
          ++it) {
        const string summarized_name = summarize(it->first);
        map<string,uadata>::iterator jt = summarized_uas.find(summarized_name);
        if(jt == summarized_uas.end())
          jt = summarized_uas.insert(pair<string,uadata>(summarized_name,uadata(summarized_name))).first;
        jt->second += it->second;
      }
      uas = &summarized_uas;
    } else
      uas = &useragents;

    // Linearize
    vector<const uadata *> agents;
    for(map<string,uadata>::const_iterator it = uas->begin();
        it != uas->end();
        ++it)
      agents.push_back(&it->second);

    // Sort by count
    sort(agents.begin(), agents.end(), uadata::ptr_art_compare());

    for(unsigned n = 0; n < agents.size(); ++n) {
      os << "<tr>\n";
      os << "<td>" << HTML::Escape(agents[n]->name) << "</td>\n";
      os << "<td sorttable_customkey=-" << fixed << agents[n]->articles
         << ">" << agents[n]->articles << "</td>\n";
      os << "<td sorttable_customkey=-" << fixed << agents[n]->senders.size()
         << ">" << agents[n]->senders.size() << "</td>\n";
      os << "</tr>\n";
    }

    os << "</table>\n";
    Config::footer(os);
    os << flush;
  } catch(ios::failure) {
    fatal(errno, "writing to %s", path.c_str());
  }
}

const string &AllGroups::summarize(const string &ua) {
  struct agent {
    const char *substring;
    const std::string name;
  };
  static struct agent clients[] = {
    { "40tude_Dialog", "40tude_Dialog" },
    { "Alpine", "Alpine" },
    { "alpine", "Alpine" },
    { "Claws Mail", "Claws Mail" },
    { "Direct Read News", "Direct Read News" },
    { "Emas", "Emas" },
    { "Forte Agent", "Forte Agent" },
    { "Forte Free Agent", "Forte Agent" },
    { "G2", "G2" },
    { "Gnus", "Gnus" },
    { "Groundhog Newsreader for Android", "Groundhog Newsreader for Android" },
    { "Hamster", "Hamster" },
    { "Hogwasher", "Hogwasher" },
    { "JetBrains Omea Reader", "JetBrains Omea Reader" },
    { "knews", "knews" },
    { "KNode", "KNode" },
    { "MacSOUP", "MacSOUP" },
    { "MesNews", "MesNews" },
    { "Messenger-Pro", "Messenger-Pro" },
    { "MicroPlanet Gravity", "MicroPlanet Gravity" },
    { "MicroPlanet-Gravity", "MicroPlanet Gravity" },
    { "Microsoft Outlook Express", "Outlook Express" },
    { "Microsoft Windows Mail", "Outlook Express" },
    { "Microsoft Windows Live Mail", "Outlook Express" },
    { "Microsoft Internet News", "Microsoft Internet News" },
    { "Microsoft-Entourage", "Microsoft Entourage" },
    { "MT-NewsWatcher", "MT-NewsWatcher" },
    { "Mutt", "Mutt" },
    { "Netscape", "Netscape" },
    { "NewsHound", "NewsHound" },
    { "NewsLeecher", "NewsLeecher" },
    { "NewsPortal", "NewsPortal" },
    { "NewsTap", "NewsTap" },
    { "Noworyta News Reader", "Noworyta News Reader" },
    { "Opera Mail", "Opera Mail" },
    { "Pan", "Pan" },
    { "^pan ", "Pan" },
    { "Pluto", "Pluto" },
    { "PMINews", "PMINews" },
    { "ProNews", "ProNews" },
    { "SeaMonkey", "SeaMonkey" },
    { "Iceape", "SeaMonkey" },          // Debian
    { "slrn", "slrn" },
    { "Sylpheed", "Sylpheed" },
    { "Thoth", "Thoth" },
    { "Thunderbird", "Thunderbird" },
    { "Lanikai", "Thunderbird" },       // early 2010 preview
    { "Shredder", "Thunderbird" },      // mid 2008 preview
    { "Icedove", "Thunderbird" },       // Debian
    { "^tin", "tin" },
    { "^TIN", "tin" },
    { "TRAVEL.com", "TRAVEL.com" },
    { "trn", "trn" },
    { "Turnpike", "Turnpike" },
    { "Unison", "Unison" },
    { "XanaNews", "XanaNews" },
    { "Xnews", "Xnews" },
    { "XPN", "XPN" },
    { "^NN/", "NN" },
    { "^NN version", "NN" },
    { "^nn/", "NN" },
    { "WinVN", "WinVN" },
    { "News Xpress", "News Xpress" },
    { "NewsAgent", "NewsAgent" },
    { "PC Piggy News", "PC Piggy News" },
    { "MATLAB Central Newsreader", "MATLAB Central Newsreader" },
    { "Flrn", "Flrn" },
    { "Loom", "Loom" },
    { "iForth", "iForth" },
    { "SquirrelMail", "SquirrelMail" },
    { "NewsMan Pro", "NewsMan Pro" },
    { "News Rover", "News Rover" },
    { "AspNNTP", "AspNNTP" },
    { "Grepler", "Grepler" },
    { "^xrn", "xrn" },
    { "Web-News", "Web-News" },
    { "Marcel", "Marcel" },
    { "Gemini", "Gemini" },
    { "newsSync", "newsSync" },
    { "FUDforum", "FUDforum" },
    { "KMail", "KMail" },
    { "Pineapple News", "Pineapple News" },
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

AllGroups::uadata &AllGroups::uadata::operator+=(const uadata &that) {
  articles += that.articles;
  for(set<string>::const_iterator it = that.senders.begin();
      it != that.senders.end();
      ++it)
    senders.insert(*it);
  return *this;
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
