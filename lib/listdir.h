/*-*-C++-*-
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
#ifndef LISTDIR_H
#define LISTDIR_H

#include <dirent.h>

template<typename T, class P>
void list_directory(const std::string &path,
                           T &names,
                           P pred) {
  DIR *dp;
  struct dirent *de;
  if(!(dp = opendir(path.c_str())))
    fatal(errno, "opendir %s", path.c_str());
  while((de = readdir(dp)))
    if(pred(de->d_name))
      names.push_back(de->d_name);
  closedir(dp);
}

template<typename T>
void list_directory(const std::string &path,
                           T &names) {
  list_directory(path, names, [](const std::string &) { return true; });
}

#endif /* LISTDIR_H */
