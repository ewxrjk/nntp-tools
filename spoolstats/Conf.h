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
#ifndef CONF_H
#define CONF_H

struct Config {
  static bool terminal;
  static time_t start_time;
  static time_t end_time;
  static time_t start_mtime;
  static std::string output;
  static int end_latency;
  static int start_latency;
  static int days;
  static std::string spool;

  // Parse command line
  static void Options(int argc, char **argv);

  // Set of hierarchies to analyse
  static std::map<std::string, Hierarchy *> hierarchies;

  // Generate standard footer
  static void footer(std::ostream &os);

private:
  // Add a hierarchy
  static void hierarchy(const std::string &h);

};

#endif /* CONF_H */

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
