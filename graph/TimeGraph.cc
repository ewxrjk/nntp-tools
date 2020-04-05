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
#include "TimeGraph.h"

using namespace std;

TimeGraph::TimeGraph(int width_, int height_, decompose_time_type decompose_,
                     compose_time_type compose_):
    Graph(width_, height_),
    decompose_fn(decompose_), compose_fn(compose_) {}

void TimeGraph::define_x(const string &name, time_t start_, time_t end_) {
  if(hour(start_) == hour(end_)) {
    const long hs = hour(start_), he = hs + 1;
    const time_t ts = hourstart(hs), te = hourstart(he);
    Graph::define_x(name + display(ts, " (%x)"), ts, te);
    for(time_t t = ts; t < te; t += 600) {
      range_x(t, t + 600, "");
      marker_x(t, display(t, "%H:%M"));
    }
    marker_x(te, display(te, "%H:%M"));
  } else if(day(start_) == day(end_)) {
    const long ds = day(start_), de = ds + 1;
    const time_t ts = daystart(ds), te = daystart(de);
    Graph::define_x(name + display(ts, " (%F)"), ts, te);
    for(time_t t = ts; t < te; t = nexthour(t)) {
      range_x(t, nexthour(t), "");
      if(hour(t) % 32 % 3 == 0)
        marker_x(t, display(t, "%H:%M"));
    }
    marker_x(te, display(te, "%H:%M"));
  } else if(month(start_) == month(end_)) {
    const long ms = month(start_), me = ms + 1;
    const time_t ts = monthstart(ms), te = monthstart(me);
    Graph::define_x(name + display(ts, " (%b %Y)"), ts, te);
    for(time_t t = ts; t < te; t = nextday(t))
      range_x(t, nextday(t), display(t, "%d"));
  } else if(year(start_) == year(end_)) {
    const long ys = year(start_), ye = ys + 1;
    const time_t ts = yearstart(ys), te = yearstart(ye);
    Graph::define_x(name + display(ts, " (%Y)"), ts, te);
    for(time_t t = ts; t < te; t = nextmonth(t))
      range_x(t, nextmonth(t), display(t, "%b"));
  } else {
    const long ys = year(start_), ye = year(end_) + 1;
    const int ny = ye - ys;
    const time_t ts = yearstart(ys), te = yearstart(ye);
    Graph::define_x(name, ts, te);
    for(time_t t = ts; t < te; t = nextyear(t))
      range_x(t, nextyear(t), display(t, "%Y"));
    if(ny <= 3) {
      for(time_t t = ts; t <= te; t = nextmonth(t))
        marker_x(t, "");
    }
  }
}

long TimeGraph::hour(time_t t) {
  struct tm dc;

  decompose_fn(&t, &dc);
  return dc.tm_hour
         + 32 * (dc.tm_mday - 1 + 32 * (dc.tm_mon + 16 * dc.tm_year));
  // Good for ~100,000 years with a 32-bit long, or hundreds of trillions of
  // years with 64 bits.
}

time_t TimeGraph::hourstart(long h) {
  struct tm dc;
  dc.tm_hour = h % 32;
  h /= 32;
  dc.tm_mday = 1 + h % 32;
  h /= 32;
  dc.tm_mon = h % 16;
  h /= 16;
  dc.tm_year = h;
  return compose(dc);
}

long TimeGraph::day(time_t t) {
  struct tm dc;

  decompose_fn(&t, &dc);
  return dc.tm_mday - 1 + 32 * (dc.tm_mon + 16 * dc.tm_year);
}

time_t TimeGraph::daystart(long d) {
  struct tm dc;
  dc.tm_hour = 0;
  dc.tm_mday = 1 + d % 32;
  d /= 32;
  dc.tm_mon = d % 16;
  d /= 16;
  dc.tm_year = d;
  return compose(dc);
}

long TimeGraph::month(time_t t) {
  struct tm dc;

  decompose_fn(&t, &dc);
  return dc.tm_mon + 16 * dc.tm_year;
}

time_t TimeGraph::monthstart(long m) {
  struct tm dc;
  dc.tm_hour = 0;
  dc.tm_mday = 1;
  dc.tm_mon = m % 16;
  m /= 16;
  dc.tm_year = m;
  return compose(dc);
}

long TimeGraph::year(time_t t) {
  struct tm dc;

  decompose_fn(&t, &dc);
  return dc.tm_year;
}

time_t TimeGraph::yearstart(long y) {
  struct tm dc;
  dc.tm_hour = 0;
  dc.tm_mday = 1;
  dc.tm_mon = 0;
  dc.tm_year = y;
  return compose(dc);
}

time_t TimeGraph::compose(struct tm &dc) {
  dc.tm_sec = 0;
  dc.tm_min = 0;
  dc.tm_isdst = -1;
  return compose_fn(&dc);
}

string TimeGraph::display(time_t t, const char *format) {
  struct tm dc;
  char buffer[1024];
  decompose_fn(&t, &dc);
  strftime(buffer, sizeof buffer, format, &dc);
  return buffer;
}
