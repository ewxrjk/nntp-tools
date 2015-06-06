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
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <getopt.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "error.h"

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

#define MAP_SIZE (60 * 24 * sizeof (map_entry))

static struct tm start_time;
static std::string state = ".";
static std::string current_path;
static struct map_entry *current_map;
static struct timeval latest_time;
static uint64_t time_counter;

class details;

static inline bool operator<(const struct timeval &a,
                             const struct timeval &b) {
  if(a.tv_sec < b.tv_sec)
    return true;
  else if(a.tv_sec == b.tv_sec)
    return a.tv_usec < b.tv_usec;
  else return false;
}

static bool process_file(FILE *fp);
static void process_line(const details &d);
static void process_accepted(const details &d);
static void read_timestamp();
static void update_timestamp();

int main(int argc, char **argv) {
  static const struct option options[] = {
    { "input", required_argument, 0, 'i' },
    { "help", no_argument, 0, 'h' },
    { "version", no_argument, 0, 'V' },
    { 0, 0, 0, 0 }
  };

  int n;
  const char *input = 0;

  while((n = getopt_long(argc, argv, "hVi:s:", options, 0)) >= 0) {
    switch(n) {
    case 'h':
      printf("Usage:\n\
  news-sources [OPTIONS]\n\
\n\
Options:\n\
  -i, --input PATH                  Input file (default: stdin)\n\
  -s, --state DIR                   State directory (default: .)\n\
  -h, --help                        Display usage message\n\
  -V, --version                     Display version number\n");
      return 0;
    case 'V':
      printf("news-sources from rjk-nntp-tools version " VERSION "\n");
      return 0;
    case 'i':
      input = optarg;
      break;
    case 's':
      state = optarg;
      break;
    default:
      return 1;
    }
  }

  struct timeval now;
  if(gettimeofday(&now, NULL) == -1)
    fatal(errno, "gettimeofday");
  if(!localtime_r(&now.tv_sec, &start_time))
    fatal(errno, "localtime_r");

  read_timestamp();

  if(input) {
    FILE *fp = fopen(input, "r");
    if(!fp)
      fatal(errno, "opening %s", input);
    if(!process_file(fp))
      fatal(errno, "reading %s", input);
    fclose(fp);
  } else {
    if(!process_file(stdin))
      fatal(errno, "reading stdin");
  }

  update_timestamp();

  return 0;
}

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
    } else
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

static void process_accepted(const details &d) {
  char buffer[32];
  snprintf(buffer, sizeof buffer, "%04d-%02d-%02d-",
           d.when.tm_year + 1900, d.when.tm_mon + 1, d.when.tm_mday);
  std::string path = state + "/" + buffer;
  for(size_t pos = 0; pos < d.peer.size(); ++pos) {
    if(d.peer.at(pos) > ' ' && d.peer.at(pos) < 0x7F)
      path += d.peer.at(pos);
    else
      path += 'X';
  }
  if(path != current_path) {
    if(current_map) {
      if(munmap(current_map, MAP_SIZE) < 0)
        fatal(errno, "munmap %s", current_path.c_str());
    }
    current_path.clear();
    current_map = 0;
    int fd;
    if((fd = open(path.c_str(), O_RDWR|O_CREAT, 0666)) < 0)
      fatal(errno, "open %s", path.c_str());
    if(ftruncate(fd, MAP_SIZE) < 0)
      fatal(errno, "ftruncate %s", path.c_str());
    void *new_map = mmap(0, MAP_SIZE, PROT_READ|PROT_WRITE,
                         MAP_SHARED, fd, 0);
    if(new_map == (void *)-1)
      fatal(errno, "mmap %s", path.c_str());
    current_map = (map_entry *)new_map;
    if(close(fd) < 0)
      fatal(errno, "close %s", path.c_str());
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
