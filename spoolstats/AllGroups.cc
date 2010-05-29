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
  // TODO lots of scope to de-dupe with Hierarchy::page() here
  ofstream os((Config::output + "/index.html").c_str());
  os.exceptions(ofstream::badbit|ofstream::failbit);

  os << HTML::Header("Spool report");

  os << "<table class=sortable>\n";

  os << "<thead>\n";
  os << "<tr>\n";
  os << "<th>Hierarchy</th>\n";
  os << "<th>Articles/day</th>\n";
  os << "<th>Bytes/day</th>\n";
  os << "</tr>\n";
  os << "</thead>\n";

  for(map<string,Hierarchy *>::const_iterator it = Config::hierarchies.begin();
      it != Config::hierarchies.end();
      ++it) {
    Hierarchy *const h = it->second;
    h->summary(os);                     // Summary line
    h->page();                          // Details page
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
  os << "<td>" << Bytes(total_bytes_per_day) << "</td>\n";
  os << "</tr>";
  os << "</tfoot>\n";
  os << "</table>\n";
  os << flush;
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
