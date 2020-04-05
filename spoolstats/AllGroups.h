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
#ifndef ALL_H
#define ALL_H

class Hierarchy;

class AllGroups: public Bucket {
public:
  AllGroups(): skip_lwm(0), skip_mtime(0), dirs(0) {}

  ~AllGroups();

  // Visit one article
  void visit(const Article *a);

  // Scan the spool
  void scan();

  // Generate logs
  void logs();

  // Read logs
  void readLogs();

  // Generate graphs
  void graphs();

  // Generate all reports
  void report();

private:
  // Message IDs that have been seen
  std::set<std::string> seen;

  ArticleProperty useragents;
  ArticleProperty charsets;

  // Recurse into one directory
  void recurse(const std::string &dir);

  // Visit one article by name.  Returns 1 if article used, else 0.
  int visit(const std::string &path);

  // Generate the hierarchies report
  void report_hierarchies();

  // Generate the groups report
  void report_groups();

  // Generate the agents report
  void report_agents(const std::string &path, bool summarized);

  // Generate the charsets report
  void report_charsets();

  // Summarize a user-agent name
  static const std::string &summarize(const std::string &);

  long skip_lwm;
  long skip_mtime;
  long dirs;
};

#endif /* ALL_H */
