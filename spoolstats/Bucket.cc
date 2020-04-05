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
#include "spoolstats.h"

using namespace std;

Bucket::~Bucket() {}

void Bucket::verticalScale(double max, double &limit, double &chunk,
                           double &count) {
  for(chunk = 1; 10 * chunk < max; chunk *= 10)
    ;
  for(count = 1; count * chunk < max; ++count)
    ;
  limit = chunk * count;
}

void Bucket::graph(const string &title, const string &csv, const string &png) {
  vector<vector<Value>> rows;
  read_csv(csv, rows);
  TimeGraph g(720, 480, gmtime_r, timegm);
  g.define_title(title);
  g.define_x("Date", rows.front()[0], rows.back()[0]);
  double maxbyterate = 0, maxarticlerate = 0;
  for(vector<vector<Value>>::iterator it = rows.begin(); it != rows.end();
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
  double limbyterate, chunkbyterate, countbyterate;
  verticalScale(maxbyterate, limbyterate, chunkbyterate, countbyterate);
  double limarticlerate, chunkarticlerate, countarticlerate;
  verticalScale(maxarticlerate, limarticlerate, chunkarticlerate,
                countarticlerate);
  g.define_y("byte/d", 0, limbyterate);
  g.define_y("arts/d", 0, limarticlerate);
  for(double n = 0; n <= countbyterate; n += (countbyterate > 4 ? 1 : 0.5)) {
    double y = n * chunkbyterate;
    g.marker_y(0, y, compact_kilo(y));
  }
  for(double n = 0; n <= countarticlerate;
      n += (countarticlerate > 4 ? 1 : 0.5)) {
    double y = n * chunkarticlerate;
    g.marker_y(1, y, compact_kilo(y));
  }
  g.axes();
  for(vector<vector<Value>>::iterator it = rows.begin(); it != rows.end();
      ++it) {
    double x = (*it)[0];
    intmax_t seconds = (*it)[1];
    intmax_t bytecount = (*it)[2];
    double byterate = bytecount / (seconds / 86400);
    g.plot(0, x, byterate, true);
  }
  for(vector<vector<Value>>::iterator it = rows.begin(); it != rows.end();
      ++it) {
    double x = (*it)[0];
    intmax_t seconds = (*it)[1];
    intmax_t articlecount = (*it)[3];
    double articlerate = articlecount / (seconds / 86400);
    g.plot(1, x, articlerate, true);
  }
  g.save(png);
}
