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
#ifndef ARTICLE_H
#define ARTICLE_H

#include <map>
#include <string>
#include <vector>
#include <cassert>

class Article {
public:
  Article(const std::string &text, size_t bytes_);

  void get_groups(std::vector<std::string> &groups) const;
  time_t date() const;

  inline bool valid() const {
    auto it = headers.find("message-id");
    if(it == headers.end())
      return false;
    it = headers.find("date");
    if(it == headers.end())
      return false;
    it = headers.find("from");
    if(it == headers.end())
      return false;
    return true;
  }

  inline const std::string &mid() const {
    auto it = headers.find("message-id");
    assert(it != headers.end());
    return it->second;
  }

  inline size_t get_size() const {
    return bytes;
  }

  inline const std::string &sender() const {
    auto it = headers.find("from");
    assert(it != headers.end());
    return it->second;
  }

  const std::string &useragent() const;
  const std::string charset() const;

private:
  void parse(const std::string &text);
  static int eol(const std::string &text, std::string::size_type pos);
  static int eoh(const std::string &text, std::string::size_type pos);

  size_t bytes;
  std::map<std::string, std::string> headers;
  mutable time_t cached_date;
};

#endif /* ARTICLE_H */
