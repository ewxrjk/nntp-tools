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
#include "spoolstats.h"

using namespace std;

Bucket::~Bucket() {
}

void Bucket::graph(const string &title,
                   const string &csv, const string &png) {
  list<vector<intmax_t> > rows;
  read_csv(csv, rows);
  TimeGraph g(720, 480, gmtime_r, timegm);
  g.define_title(title);
  g.define_x("Date", rows.front()[0], rows.back()[0]);
  double maxbyterate = 0, maxarticlerate = 0;
  for(list<vector<intmax_t> >::iterator it = rows.begin();
      it != rows.end();
      ++it) {
    intmax_t seconds = (*it)[1];
    intmax_t bytecount = (*it)[2];
    intmax_t articlecount = (*it)[3];
    double byterate = bytecount / (seconds / 86400);
    double articlerate = articlecount / (seconds / 86400);
    if(byterate > maxbyterate)
      maxbyterate = byterate;
    if(articlerate > maxarticlerate)
      maxarticlerate = articlerate;
  }
  double limbyterate;
  for(limbyterate = 10.0; limbyterate < maxbyterate; limbyterate *= 10.0)
    ;
  double limarticlerate;
  for(limarticlerate = 10.0; limarticlerate < maxarticlerate; limarticlerate *= 10.0)
    ;
  g.define_y("byte/d", 0, limbyterate);
  g.define_y("arts/d", 0, limarticlerate);
  for(double y = 0.0; y <= limbyterate; y += limbyterate / 10.0)
    g.marker_y(0, y, compact_kilo(y));
  for(double y = 0.0; y <= limarticlerate; y += limarticlerate / 10.0)
    g.marker_y(1, y, compact_kilo(y));
  g.axes();
  for(list<vector<intmax_t> >::iterator it = rows.begin();
      it != rows.end();
      ++it) {
    double x = (*it)[0];
    intmax_t seconds = (*it)[1];
    intmax_t bytecount = (*it)[2];
    double byterate = bytecount / (seconds / 86400);
    g.plot(0, x, byterate, true);
  }
  for(list<vector<intmax_t> >::iterator it = rows.begin();
      it != rows.end();
      ++it) {
    double x = (*it)[0];
    intmax_t seconds = (*it)[1];
    intmax_t articlecount = (*it)[3];
    double articlerate = articlecount / (seconds / 86400);
    g.plot(1, x, articlerate, true);
  }
  g.save(png);
}
