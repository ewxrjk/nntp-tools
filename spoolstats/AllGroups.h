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
#ifndef ALL_H
#define ALL_H

class Hierarchy;

class AllGroups: public Bucket {
public:
  ~AllGroups();

  // Visit one article
  void visit(const Article *a);

  // Scan the spool
  void scan();

  // Generate logs
  void logs();

  // Generate graphs
  void graphs();

  // Generate all reports
  void report();

private:
  // Message IDs that have been seen
  std::set<std::string> seen;

  // User agents
  std::map<std::string,long> useragents;

  // Recurse into one directory
  void recurse(const std::string &dir);

  // Visit one article by name.  Returns 1 if article used, else 0.
  int visit(const std::string &path);

  // Generate the hierarchies report
  void report_hierarchies();

  // Generate the groups report
  void report_groups();

  // Generate the agents report
  void report_agents();
  void report_agents_summarized();

  // Summarize a user-agent name
  static const std::string &summarize(const std::string &);
};

#endif /* ALL_H */

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
