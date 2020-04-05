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
#ifndef TIMEGRAPH_H
#define TIMEGRAPH_H

#include "Graph.h"
#include <ctime>

class TimeGraph: public Graph {
public:
  typedef struct tm *decompose_time_type(const time_t *, struct tm *);
  typedef time_t compose_time_type(struct tm *);

  // Define graph dimensions
  TimeGraph(int width_, int height_,
            decompose_time_type *decompose_ = localtime_r,
            compose_time_type *compose_ = mktime);

  // Intelligently choose X range
  void define_x(const std::string &name, time_t start_, time_t end_);

  inline void plot(int v, time_t x, double y, bool link = false) {
    Graph::plot(v, x, y, link);
  }

private:
  decompose_time_type *decompose_fn;
  compose_time_type *compose_fn;

  long hour(time_t t);
  long day(time_t t);
  long month(time_t t);
  long year(time_t t);

  time_t hourstart(long h);
  time_t daystart(long d);
  time_t monthstart(long m);
  time_t yearstart(long y);

  inline time_t nexthour(time_t t) {
    return hourstart(hour(t) + 1);
  }
  inline time_t nextday(time_t t) {
    return daystart(day(t) + 1);
  }
  inline time_t nextmonth(time_t t) {
    return monthstart(month(t) + 1);
  }
  inline time_t nextyear(time_t t) {
    return yearstart(year(t) + 1);
  }

  time_t compose(struct tm &dc);

  std::string display(time_t t, const char *format);
};

#endif /* TIMEGRAPH_H */
