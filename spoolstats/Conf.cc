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
#include <getopt.h>
#include <unistd.h>
#include <cstdlib>

using namespace std;

bool Config::terminal;
time_t Config::start_time;
time_t Config::end_time;
time_t Config::start_mtime;
int Config::end_latency = 86400;
int Config::start_latency = 60;
string Config::output = ".";
int Config::days = 7;
string Config::spool = "/var/spool/news/articles";

// Parse command line options
void Config::Options(int argc, char **argv) {
  int n;
  enum {
    opt_scan = 256,
    opt_no_scan,
    opt_graph,
    opt_no_graph
  };

  // The option table
  static const struct option options[] = {
    { "debug", no_argument, 0, 'D' },
    { "spool", required_argument, 0, 'S' },
    { "hierarchies", required_argument, 0, 'H' },
    { "big8", no_argument, 0, '8' },
    { "days", required_argument, 0, 'N' },
    { "latency", required_argument, 0, 'L' },
    { "output", required_argument, 0, 'O' },
    { "scan", no_argument, 0, opt_scan },
    { "no-scan", no_argument, 0, opt_no_scan },
    { "graph", no_argument, 0, opt_graph },
    { "no-graph", no_argument, 0, opt_no_graph },
    { "help", no_argument, 0, 'h' },
    { "quiet", no_argument, 0, 'Q' },
    { "version", no_argument, 0, 'V' },
    { 0, 0, 0, 0 }
  };

  terminal = !!isatty(2);
  while((n = getopt_long(argc, argv, "DS:QhH:8VN:L:O:", options, 0)) >= 0) {
    switch(n) {
    case 'D':
      debug = 1;
      break;
    case 'S':
      spool = optarg;
      break;
    case 'Q':
      terminal = false;
      break;
    case 'H': {
      vector<string> bits;
      split(bits, ',', string(optarg));
      for(size_t i = 0; i < bits.size(); ++i)
        hierarchy(bits[i]);
      break;
    }
    case 'L': {
      vector<string> bits;
      split(bits, ',', string(optarg));
      if(bits.size() != 2)
        fatal(0, "invalid argument to --latency option");
      start_latency = atoi(bits[0].c_str());
      end_latency = atoi(bits[1].c_str());
      break;
    }
    case '8':
      hierarchy("news");
      hierarchy("humanities");
      hierarchy("sci");
      hierarchy("comp");
      hierarchy("misc");
      hierarchy("soc");
      hierarchy("rec");
      hierarchy("talk");
      break;
    case 'N':
      days = atoi(optarg);
      if(days <= 0)
        fatal(0, "--days must be positive");
      break;
    case 'O':
      output = optarg;
      break;
    case opt_scan:
      scan = true;
      break;
    case opt_no_scan:
      scan = false;
      break;
    case opt_graph:
      graph = true;
      break;
    case opt_no_graph:
      graph = false;
      break;
    case 'h':
      printf("Usage:\n\
  spoolstats [OPTIONS]\n\
\n\
Options:\n\
  -N, --days DAYS                   Number of days to analyse\n\
  -L, --latency BEFORE, AFTER       Set latencies in seconds\n\
  -S, --spool PATH                  Path to spool\n\
  -H, --hierarchies NAME[,NAME...]  Hierachies to analyse\n\
  -8, --big8                        Analyse the Big 8\n\
  -O, --output DIRECTORY            Output directory\n\
  -Q, --quiet                       Quieter operation\n\
  --no-scan, --no-graph             Suppress phases\n\
  -h, --help                        Display usage message\n\
  -V, --version                     Display version number\n");
      exit(0);
    case 'V':
      printf("spoolstats from rjk-nntp-tools version " VERSION "\n");
      exit(0);
    default:
      exit(1);
    }
  }
  time(&end_time);
  end_time -= end_latency;
  start_time = end_time - 86400 * days;
  start_mtime = start_time - start_latency;
}

// Add a hierarchy
void Config::hierarchy(const string &h) {
  if(hierarchies.find(h) == hierarchies.end())
    hierarchies[h] = new Hierarchy(h);
}

void Config::footer(ostream &os) {
  os << "<p><a href=" << HTML::Quote(".") << ">Hierarchies</a>"
     << " | <a href=" << HTML::Quote("allgroups.html") << ">All groups</a>"
     << " | <a href=" << HTML::Quote("agents-summary.html") << ">User agents</a>"
     << " (<a href=" << HTML::Quote("agents.html") << ">full</a>)"
     << " | <a href=" << HTML::Quote("charsets.html") << ">Encodings</a>"
     << "</p>\n";
  os << "<p class=credits><a href="
     << HTML::Quote("http://www.greenend.org.uk/rjk/2006/newstools.html")
     << ">spoolstats "VERSION"</a></p>\n";
}

map<string, Hierarchy *> Config::hierarchies;

bool Config::scan = true;
bool Config::graph = true;
