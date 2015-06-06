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
#include "cpputils.h"
#include "error.h"

void read_file(const std::string &path, std::vector<std::string> &lines) {
  FILE *fp;
  std::string line;
  int ch;
  if(!(fp = fopen(path.c_str(), "r")))
    fatal(errno, "opening %s", path.c_str());
  do {
    line.clear();
    while((ch = getc(fp)) >= 0) {
      line += ch;
      if(ch == '\n')
        break;
    }
    if(line.size())
      lines.push_back(line);
  } while(line.size());
  if(ferror(fp))
    fatal(errno, "reading %s", path.c_str());
  fclose(fp);
}
