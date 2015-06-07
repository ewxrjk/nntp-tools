/*
 * This file is part of rjk-nntp-tools.
 * Copyright (C) 2015 Richard Kettlewell
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
#include <config.h>
#include <algorithm>
#include <cairomm/cairomm.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <getopt.h>
#include <map>
#include <set>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "utils.h"
#include "cpputils.h"
#include "error.h"
#include "listdir.h"

// from art.c
#define ART_ACCEPT '+'
#define ART_CANC 'c'
#define ART_STRSTR '?'
#define ART_JUNK 'j'
#define ART_REJECT '-'

struct map_entry {
  uint64_t articles;
  uint64_t bytes;
};

#define MAP_BUCKETS (60 * 24)
#define MAP_SIZE (MAP_BUCKETS * sizeof (map_entry))
#define MAP_EXTENSION ".nsdat"

static struct tm start_time;
static std::string state = ".";
static std::string output = ".";
static std::string current_path;
static struct map_entry *current_map;
static struct timeval latest_time;
static uint64_t time_counter;
static std::set<std::string> days_changed;
static double width = MAP_BUCKETS / 2;
static double height = 256;
static double margin = 64;

static const double colors[][3]  = {
  { 1.0, 0.0, 0.0 },
  { 0.0, 0.0, 1.0 },
  { 0.0, 1.0, 0.0 },
  { 1.0, 0.0, 1.0 },
  { 1.0, 1.0, 0.0 },
  { 0.0, 1.0, 1.0 },
  // TODO more; or choose intelligently for the number of peers
};

class details;

static inline bool operator<(const struct timeval &a,
                             const struct timeval &b) {
  if(a.tv_sec < b.tv_sec)
    return true;
  else if(a.tv_sec == b.tv_sec)
    return a.tv_usec < b.tv_usec;
  else return false;
}

static inline bool starts_with(const std::string &a,
                               const std::string &b) {
  return a.compare(0, b.size(), b) == 0;
}

static inline bool ends_with(const std::string &a,
                             const std::string &b) {
  return (a.size() >= b.size()
          && a.compare(a.size() - b.size(), b.size(), b) == 0);
}

static bool process_file(FILE *fp);
static void process_line(const details &d);
static void process_accepted(const details &d);
static void read_timestamp();
static void update_timestamp();
static void process_day(const std::string &day);
static void draw_graph(const std::string &day,
                       const std::map<std::string,map_entry *> &info);
static void draw_axes(Cairo::RefPtr<Cairo::Context> context,
                      double max, double base,
                      const std::string &title);
static void fixup_html();

int main(int argc, char **argv) {
  static const struct option options[] = {
    { "state", required_argument, 0, 's' },
    { "output", required_argument, 0, 'o' },
    { "help", no_argument, 0, 'h' },
    { "version", no_argument, 0, 'V' },
    { 0, 0, 0, 0 }
  };

  int n;

  while((n = getopt_long(argc, argv, "hVs:o:", options, 0)) >= 0) {
    switch(n) {
    case 'h':
      printf("Usage:\n\
  news-sources [OPTIONS] [INPUT...]\n\
\n\
Options:\n\
  -s, --state DIR                   State directory (default: .)\n\
  -o, --output DIR                  Ouptut directory (default: .)\n\
  -h, --help                        Display usage message\n\
  -V, --version                     Display version number\n");
      return 0;
    case 'V':
      printf("news-sources from rjk-nntp-tools version " VERSION "\n");
      return 0;
    case 's':
      state = optarg;
      break;
    case 'o':
      output = optarg;
      break;
    default:
      return 1;
    }
  }

  // Find the current time, for later guessing what year a timestamp refers to
  struct timeval now;
  if(gettimeofday(&now, NULL) == -1)
    fatal(errno, "gettimeofday");
  if(!localtime_r(&now.tv_sec, &start_time))
    fatal(errno, "localtime_r");

  // Read the last-processed timestamp.  Any older records are ignored.
  read_timestamp();

  // Read the input.
  if(optind < argc) {
    while(optind < argc) {
      if(!strcmp(argv[optind], "-")) {
        if(!process_file(stdin))
          fatal(errno, "reading stdin");
      } else if(ends_with(argv[optind], ".gz")
                || ends_with(argv[optind], ".Z")) {
        std::vector<const char *> command;
        command.push_back("gzip");
        command.push_back("-cd");
        command.push_back(argv[optind]);
        command.push_back(NULL);
        pid_t pid;
        FILE *fp = popenvp("r", &pid, command[0], (char *const *)&command[0]);
        if(!fp)
          fatal(errno, "opening %s", argv[optind]);
        if(!process_file(fp))
          fatal(errno, "reading %s", argv[optind]);
        fclose(fp);
        int w;
        if(waitpid(pid, &w, 0) < 0)
          fatal(errno, "waitpid");
        if(w)
          fatal(0, "%s: wait status %#x", command[0], w);
      } else {
        FILE *fp = fopen(argv[optind], "r");
        if(!fp)
          fatal(errno, "opening %s", argv[optind]);
        if(!process_file(fp))
          fatal(errno, "reading %s", argv[optind]);
        fclose(fp);
      }
      ++optind;
    }
  } else {
    if(!process_file(stdin))
      fatal(errno, "reading stdin");
  }

  // Commit the last-processed timestamp.
  update_timestamp();

  // Render graphs for every day for which new data was gathered.
  std::for_each(days_changed.begin(), days_changed.end(), process_day);

  // Fix up links & index
  fixup_html();

  return 0;
}

// General-purpose line parser
class parser {
public:
  std::string line;
  size_t pos;
  size_t limit;

  bool input(FILE *fp) {
    int ch;
    line.clear();
    while((ch = getc(fp)) >= 0 && ch != '\n')
      line += ch;
    pos = 0;
    limit = line.size();
    return ch != EOF;
  }

  bool is(char c) {
    if(pos < limit && line.at(pos) == c) {
      ++pos;
      return true;
    } else
      return false;
  }

  bool is(const char *s) {
    size_t len_s = strlen(s);
    if(line.compare(pos, len_s, s, len_s) == 0) {
      pos += len_s;
      return true;
    } else
      return false;
  }

  void skip_spaces() {
    while(pos < limit && isspace(line.at(pos)))
      ++pos;
  }

  bool get_char(int &ch) {
    skip_spaces();
    if(pos < limit) {
      ch = line.at(pos++);
      return true;
    } else
      return false;
  }

  bool get_int(int &n) {
    skip_spaces();
    n = 0;
    if(pos < limit && isdigit(line.at(pos))) {
      while(pos < limit && isdigit(line.at(pos)))
        n += 10 * n + line.at(pos++) - '0';
      return true;
    } else
      return false;
  }

  bool get_string(std::string &s) {
    skip_spaces();
    s.clear();
    if(pos >= limit)
      return false;
    while(pos < limit && !isspace(line.at(pos)))
      s += line.at(pos++);
    return true;
  }
};

// Represent and parse a line from the news log
class details {
public:
  struct tm when;
  struct timeval when_time;
  int code;
  std::string peer;
  std::string message;
  int size;
  std::string extra;

  bool parse(parser &p) {
    static const char *const months[] = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    int n;
    for(n = 0; n < 12; ++n)
      if(p.is(months[n]))
        break;
    if(n >= 12)
      return false;
    when.tm_mon = n;
    if(!p.get_int(when.tm_mday))
      return false;
    if(!p.get_int(when.tm_hour))
      return false;
    if(!p.is(':'))
      return false;
    if(!p.get_int(when.tm_min))
      return false;
    if(!p.is(':'))
      return false;
    if(!p.get_int(when.tm_sec))
      return false;
    if(!p.is('.'))
      return false;
    int msec;
    if(!p.get_int(msec))
      return false;
    if(!p.get_char(code))
      return false;
    if(!p.get_string(peer))
      return false;
    if(!p.get_string(message))
      return false;
    if(code == ART_ACCEPT || code == ART_JUNK) {
      if(!p.get_int(size))
        return false;
    }
    p.skip_spaces();
    extra.assign(p.line, p.pos, std::string::npos);
    when.tm_year = start_time.tm_year;
    if(when.tm_mon > start_time.tm_mon)
      --when.tm_year;
    when.tm_isdst = -1;
    when_time.tv_sec = mktime(&when);
    when_time.tv_usec = 1000 * msec;
    return true;
  }
};

static bool process_file(FILE *fp) {
  details d;
  parser p;
  while(p.input(fp)) {
    if(d.parse(p))
      process_line(d);
  }
  return !ferror(fp);
}

static void process_line(const details &d) {
  if(d.when_time < latest_time)
    return;
  switch(d.code) {
  case ART_ACCEPT:
  case ART_JUNK:
    process_accepted(d);
    break;
  }
}

static map_entry *open_map(const std::string &path) {
  int fd;
  if((fd = open(path.c_str(), O_RDWR|O_CREAT, 0666)) < 0)
    fatal(errno, "open %s", path.c_str());
  if(ftruncate(fd, MAP_SIZE) < 0)
    fatal(errno, "ftruncate %s", path.c_str());
  void *new_map = mmap(0, MAP_SIZE, PROT_READ|PROT_WRITE,
                       MAP_SHARED, fd, 0);
  if(new_map == (void *)-1)
    fatal(errno, "mmap %s", path.c_str());
  if(close(fd) < 0)
    fatal(errno, "close %s", path.c_str());
  return (struct map_entry *)new_map;
}

static void close_map(struct map_entry *map) {
  if(map) {
    if(munmap(current_map, MAP_SIZE) < 0)
      fatal(errno, "munmap");
  }
}

static void process_accepted(const details &d) {
  char day[32];
  snprintf(day, sizeof day, "%04d-%02d-%02d",
           d.when.tm_year + 1900, d.when.tm_mon + 1, d.when.tm_mday);
  std::string path = state + "/" + day + "-";
  for(size_t pos = 0; pos < d.peer.size(); ++pos) {
    if(d.peer.at(pos) > ' ' && d.peer.at(pos) < 0x7F)
      path += d.peer.at(pos);
    else
      path += 'X';
  }
  path += MAP_EXTENSION;
  if(path != current_path) {
    close_map(current_map);
    current_map = open_map(path);
    current_path = path;
  }
  current_map[60 * d.when.tm_hour + d.when.tm_min].articles += 1;
  current_map[60 * d.when.tm_hour + d.when.tm_min].bytes += d.size;
  if(d.when_time.tv_sec >= 0) {
    if(latest_time < d.when_time)
      latest_time = d.when_time;
  }
  ++time_counter;
  if(time_counter >= 32)
    update_timestamp();
  days_changed.insert(day);
}

static void read_timestamp() {
  FILE *fp;
  time_counter = 0;
  if(!(fp = fopen("timestamp", "r"))) {
    if(errno == ENOENT)
      return;
    fatal(errno, "opening timestamp");
  }
  long long sec;
  long usec;
  if(fscanf(fp, "%lld.%ld", &sec, &usec) != 2)
    fatal(errno, "reading timestamp");
  latest_time.tv_sec = sec;
  latest_time.tv_usec = usec;
}

static void update_timestamp() {
  FILE *fp;
  if(!(fp = fopen("timestamp", "w")))
    fatal(errno, "opening timestamp");
  if(fprintf(fp, "%lld.%06ld\n", (long long)latest_time.tv_sec,
             (long)latest_time.tv_usec) < 0
     || fclose(fp) < 0)
    fatal(errno, "writing timestamp");
  time_counter = 0;
}

static void process_day(const std::string &day) {
  std::map<std::string,map_entry *> info;
  std::vector<std::string> names;
  list_directory(state, names);
  std::for_each(names.begin(), names.end(),
                [&] (std::string &name) {
                  if(starts_with(name, day)
                     && name[day.size()] == '-'
                     && ends_with(name, MAP_EXTENSION)) {
                    std::string path = state + "/" + name;
                    std::string peer(name, day.size() + 1,
                                     name.size() - (1 + day.size()
                                                    + strlen(MAP_EXTENSION)));
                    info[peer] = open_map(path);
                  }
               });
  draw_graph(day, info);
  std::for_each(info.begin(), info.end(),
                [] (const std::pair<std::string,map_entry *> &m) {
                  close_map(m.second);
                });
}

static double round_scale(double n, double &base) {
  double limit;
  base = exp10(floor(log10(n)));
  limit = base;
  while(limit < n)
    limit += base;
  return limit;
}

static void draw_graph(const std::string &day,
                       const std::map<std::string,map_entry *> &info) {
  double range_articles[MAP_BUCKETS], range_bytes[MAP_BUCKETS];
  memset(range_articles, 0, sizeof range_articles);
  memset(range_bytes, 0, sizeof range_bytes);
  for(int n = 0; n < MAP_BUCKETS; ++n) {
    double articles = 0, bytes = 0;
    std::for_each(info.begin(), info.end(),
                  [&] (const std::pair<std::string,map_entry *> &m) {
                    range_articles[n] += m.second[n].articles;
                    range_bytes[n] += m.second[n].bytes;
                    articles += m.second[n].articles;
                    bytes += m.second[n].bytes;
                  });
  }
  std::sort(&range_articles[0], &range_articles[MAP_BUCKETS]);
  std::sort(&range_bytes[0], &range_bytes[MAP_BUCKETS]);
  // Find the maximum size; the top 1% is excluded to avoid spikes dominating
  // the graph too much.
  double base_articles, base_bytes, max_articles, max_bytes;
  max_articles = round_scale(range_articles[MAP_BUCKETS * 99 / 100],
                             base_articles);
  max_bytes = round_scale(range_bytes[MAP_BUCKETS * 99 / 100],
                          base_bytes);

  Cairo::RefPtr<Cairo::Surface> surface_articles
    = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32,
                                  width + 2 * margin, height + 2 * margin);
  Cairo::RefPtr<Cairo::Context> context_articles
    = Cairo::Context::create(surface_articles);
  Cairo::RefPtr<Cairo::Surface> surface_bytes
    = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32,
                                  width + 2 * margin, height + 2 * margin);
  Cairo::RefPtr<Cairo::Context> context_bytes
    = Cairo::Context::create(surface_bytes);
  // White background
  context_articles->set_source_rgb(1.0, 1.0, 1.0);
  context_articles->paint();
  context_bytes->set_source_rgb(1.0, 1.0, 1.0);
  context_bytes->paint();
  // Draw the data
  for(int n = 0; n < MAP_BUCKETS; ++n) {
    uint64_t articles_below = 0;
    uint64_t bytes_below = 0;
    int npeer = 0;
    std::for_each(info.begin(), info.end(),
                  [&] (const std::pair<std::string,map_entry *> &m) {
                    uint64_t articles = m.second[n].articles;
                    uint64_t bytes = m.second[n].bytes;
                    if(bytes) {
                      context_bytes->set_source_rgb(colors[npeer][0],
                                                       colors[npeer][1],
                                                       colors[npeer][2]);
                      double y0 = bytes_below * height / max_bytes;
                      double y1 = (bytes_below + bytes) * height / max_bytes;
                      double x0 = n * width / MAP_BUCKETS;
                      double x1 = (n + 1) * width / MAP_BUCKETS;
                      context_bytes->rectangle(margin + x0, margin + height - y1,
                                               x1 - x0, y1 - y0);
                      context_bytes->fill();
                      bytes_below += bytes;
                    }
                    if(articles) {
                      context_articles->set_source_rgb(colors[npeer][0],
                                                       colors[npeer][1],
                                                       colors[npeer][2]);
                      double y0 = articles_below * height / max_articles;
                      double y1 = (articles_below + articles) * height / max_articles;
                      double x0 = n * width / MAP_BUCKETS;
                      double x1 = (n + 1) * width / MAP_BUCKETS;
                      context_articles->rectangle(margin + x0, margin + height - y1,
                                                  x1 - x0, y1 - y0);
                      context_articles->fill();
                      articles_below += articles;
                    }
                    ++npeer;
                  });
  }
  // Axes
  draw_axes(context_bytes, max_bytes, base_bytes, "Bytes/minute");
  draw_axes(context_articles, max_articles, base_articles, "Articles/minute");
  // Save to PNG
  surface_articles->write_to_png(output + "/" + day + "-articles.png");
  surface_bytes->write_to_png(output + "/" + day + "-bytes.png");
  // Generate an HTML wrapper
  const std::string &html = output + "/" + day + "-peers.html";
  FILE *fp;
  if(!(fp = fopen(html.c_str(), "w")))
    fatal(errno, "creating %s", html.c_str());
  fprintf(fp, "<head><title>Peering data for %s</title>\n", day.c_str());
  fprintf(fp, "<style type=\"text/css\">\n");
  fprintf(fp, ".blob {float:left;margin-right:1em;height:20px;width:32px;}\n");
  fprintf(fp, "img { border: 1px solid black; }\n");
  fprintf(fp, "</style>\n");
  fprintf(fp, "<body><h1>Peering data for %s</h1>\n", day.c_str());
  fprintf(fp, "<p><img src=\"%s-articles.png\"></p>\n", day.c_str());
  fprintf(fp, "<p><img src=\"%s-bytes.png\"></p>\n", day.c_str());
  int npeer = info.size();
  std::for_each(info.rbegin(), info.rend(),
                  [&] (const std::pair<std::string,map_entry *> &m) {
                  --npeer;
                  fprintf(fp, "<p><span class=blob style=\"background-color: #%02x%02x%02x\"></span> %s</p>\n",
                          (int)(255 * colors[npeer][0]),
                          (int)(255 * colors[npeer][1]),
                          (int)(255 * colors[npeer][2]),
                          m.first.c_str());
                });
  fprintf(fp, "<p>\n");
  fprintf(fp, "<span class=prev></span>\n");
  fprintf(fp, "<span class=next></span>\n");
  fprintf(fp, "</p>\n");
  if(ferror(fp) || fclose(fp) < 0)
    fatal(errno, "writing %s", html.c_str());
}

static void draw_axes(Cairo::RefPtr<Cairo::Context> context,
                      double max, double base,
                      const std::string &title) {
  Cairo::TextExtents te;
  Cairo::FontExtents fe;

  context->select_font_face("serif",
                            Cairo::FONT_SLANT_NORMAL,
                            Cairo::FONT_WEIGHT_NORMAL);
  context->set_font_size(12.0);
  context->get_font_extents(fe);

  // Time
  context->set_source_rgb(0.0, 0.0, 0.0);
  context->rectangle(margin, margin + height, width, 1);
  context->fill();
  for(int h = 0; h < 24; ++h) {
    char hour[32];
    snprintf(hour, sizeof hour, "%02d", h);
    double x = margin + h * width / 24;
    double y = margin + height + 1;
    context->rectangle(x, y, 1, 1);
    context->fill();
    context->move_to(x + 1, y + 1 + fe.height);
    context->show_text(hour);
  }

  // Quantity
  context->rectangle(margin - 1, margin, 1, height + 1);
  context->fill();
  double q;
  for(int n = 0; (q = n * base) <= max; ++n) {
    std::string quantity = compact_kilo(q);
    double x = margin - 2;
    double y = margin + height - height * q / max;
    context->rectangle(x, y, 1, 1);
    context->fill();
    context->get_text_extents(quantity, te);
    context->move_to(x - te.width - 2, y + te.height / 2);
    context->show_text(quantity);
  }

  // Title
  context->select_font_face("serif",
                            Cairo::FONT_SLANT_NORMAL,
                            Cairo::FONT_WEIGHT_BOLD);
  context->set_font_size(14.0);
  context->get_text_extents(title, te);
  context->move_to((2 * margin + width - te.width) / 2,
                   (margin - te.height) / 2);
  context->show_text(title);

}

static void fixup_html() {
  std::vector<std::string> names;
  list_directory(output, names, [] (const std::string &name) {
      return ends_with(name, "-peers.html");
    });
  std::sort(names.begin(), names.end());
  for(size_t n = 0; n < names.size(); ++n) {
    const std::string path = output + "/" + names[n];
    std::vector<std::string> lines;
    read_file(path, lines);
    bool changes = false;
    std::for_each(lines.begin(), lines.end(),
                  [&] (std::string &line) {
                    const std::string oldline = line;
                    if(line.find("class=prev") != std::string::npos) {
                      if(n > 0)
                        line = "<span class=prev><a href=\""+names[n-1]+"\">next</a></span>\n";
                      else
                        line = "<span class=prev></span>\n";
                    }
                    if(line.find("class=next") != std::string::npos) {
                      if(n < names.size() - 1)
                        line = "<span class=next><a href=\""+names[n+1]+"\">next</a></span>\n";
                      else
                        line = "<span class=next></span>\n";
                    }
                    if(line != oldline)
                      changes = true;
                  });
    if(changes)
      write_file(path, lines);
  }
}
