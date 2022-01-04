//-*-C++-*-
/*
 * This file is part of rjk-nntp-tools.
 * Copyright (C) 2010-11, 2014, 2015 Richard Kettlewell
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
#ifndef CPPUTILS
#define CPPUTILS

#include <vector>
#include <string>
#include <list>
#include <iostream>
#include <stdint.h>
#include <ctime>
#include <stdexcept>

void split(std::vector<std::string> &bits, char sep, const std::string &s);

time_t parse_date(const std::string &d, bool warn = false);
std::string &lower(std::string &s);
std::string &upper(std::string &s);

// Thrown by parse_csv()
class BadCSV: public std::runtime_error {
public:
  inline BadCSV(): std::runtime_error("malformed CSV") {}
};

// Value returned from a CSV file
class Value {
  enum Type {
    t_string,
    t_integer,
  };
  Type type = t_string;
  std::string v_string;
  intmax_t v_integer = 0;

public:
  Value(const std::string &s): type(t_string), v_string(s) {}
  Value(intmax_t n): type(t_integer), v_integer(n) {}
  operator std::string() const {
    if(type != t_string)
      throw std::runtime_error("type mismatch");
    return v_string;
  }
  operator intmax_t() const {
    if(type != t_integer)
      throw std::runtime_error("type mismatch");
    return v_integer;
  }
};

void read_csv(const std::string &path, std::vector<std::vector<Value>> &rows);
std::string csv_quote(const std::string &s);
std::string compact_kilo(double n);
std::string round_kilo(double n);
void read_file(const std::string &path, std::vector<std::string> &lines);
void write_file(const std::string &path, std::vector<std::string> &lines);

#endif /* CPPUTILS */
