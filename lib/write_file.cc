/*
 * This file is part of rjk-nntp-tools.
 * Copyright © 2015 Richard Kettlewell
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
#include <cstdio>
#include <cerrno>
#include <algorithm>
#include "cpputils.h"
#include "error.h"

void write_file(const std::string &path, std::vector<std::string> &lines) {
  FILE *fp;
  const std::string tmp = path + ".new";
  if(!(fp = fopen(tmp.c_str(), "w")))
    fatal(errno, "opening %s", tmp.c_str());
  std::for_each(lines.begin(), lines.end(), [&](const std::string &line) {
    fwrite(line.data(), 1, line.size(), fp);
  });
  if(ferror(fp) || fclose(fp) < 0)
    fatal(errno, "writing %s", tmp.c_str());
  if(rename(tmp.c_str(), path.c_str()) < 0)
    fatal(errno, "renaming %s", tmp.c_str());
}
