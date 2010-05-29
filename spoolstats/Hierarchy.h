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
#ifndef HIERARCHY_H
#define HIERARCHY_H

class Group;

class Hierarchy: public SenderCountingBucket {
public:
  // Hierarchy name
  const std::string name;

  Hierarchy(const std::string &name_);

  ~Hierarchy();

  void visit(const Article *a);

  // Generate logs
  void logs();

  // Generate a summary line for this hierarchy
  void summary(std::ostream &s);

  // Generate a report page for this hiearchy
  void page();

  // Locate a group (which must be in this hierarchy)
  Group *group(const std::string &name);

  // Map of group names to their objects
  std::map<std::string,Group *> groups;
};

#endif /* HIERARCHY_H */

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
