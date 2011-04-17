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
#include <cassert>
#include <iostream>

using namespace std;

static void generic() {
  cerr << "generic" << endl;
  Graph g(640, 480);
  g.define_title("Title");
  // X axis
  g.define_x("X", 0.0, 100.0);
  g.marker_x(0.0, "0");
  g.marker_x(50.0, "50");
  g.marker_x(100.0, "100");
  g.range_x(0.0, 25.0, "first");
  g.range_x(25.0, 50.0, "second");
  g.range_x(50.0, 75.0, "third");
  g.range_x(75.0, 100.0, "fourth");
  // Y axes
  g.define_y("Y0", 0.0, 10.0);
  g.marker_y(0, 0.0, "0");
  g.marker_y(0, 1.0, "1");
  g.marker_y(0, 2.0, "2");
  g.marker_y(0, 3.0, "3");
  g.marker_y(0, 4.0, "4");
  g.marker_y(0, 5.0, "5");
  g.marker_y(0, 6.0, "6");
  g.marker_y(0, 7.0, "7");
  g.marker_y(0, 8.0, "8");
  g.marker_y(0, 9.0, "9");
  g.marker_y(0, 10.0, "10");

  g.define_y("Y1", 0, 5.0);
  g.marker_y(1, 0.0, "0");
  g.marker_y(1, 1.0, "1");
  g.marker_y(1, 2.0, "2");
  g.marker_y(1, 3.0, "3");
  g.marker_y(1, 4.0, "4");
  g.marker_y(1, 5.0, "5");
  g.axes();

  for(double n = 0.0; n <= 100.0; ++n)
    g.plot(0, n, 5.0 + 5.0 * sin(n * 2 * M_PI / 100.0), true);
  srand(1234);
  double y = 2.5;
  for(double n = 0.0; n <= 100.0; ++n) {
    g.plot(1, n, y);
    y += (double)rand() / RAND_MAX - 0.5;
    if(y < 0) y = 0.0;
    if(y > 5) y = 5.0;
  }
  g.save("generic.png");
}

static void timegraph(time_t s, time_t e, const char *name) {
  cerr << name << endl;
  assert(s < e);
  TimeGraph g(640, 480);
  g.define_title(name);
  g.define_x("time", s, e);
  g.define_y("Y", 0.0, 10.0);
  g.axes();
  for(time_t n = s; n <= e; n += (e - s) / 480)
    g.plot(0, n, 10.0 * (n - s) / (double)(e - s));
  g.save(name);
}

static void hourly() {
  timegraph(1275300270, 1275300270 + 2000, "hourly.png");
}

static void daily() {
  timegraph(1275300270, 1275300270 + 7200, "daily.png");
}

static void monthly() {
  timegraph(1274263523, 1275300270, "monthly.png");
}

static void yearly() {
  timegraph(1274263523, 1280484346, "yearly.png");
}

static void huge() {
  timegraph(1274263523, 1344420380, "huge.png");
}

int main() {
  generic();
  hourly();
  daily();
  monthly();
  yearly();
  huge();
  return 0;
}
