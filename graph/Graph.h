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
#ifndef GRAPH_H
#define GRAPH_H

#include <cairomm/cairomm.h>
#include <string>
#include <vector>
#include <map>

// Graph-drawing class
class Graph {
public:
  // Define graph dimensions
  Graph(int width_, int height_);

  // TODO more flexible interfaces, e.g. exposing the underlying Cairo objects.

  // Call the following in order:-

  // Name the X axis.  Call exactly once.
  void define_x(const std::string &name, double start_, double end_);

  // Name the graph.  Call at most once.
  void define_title(const std::string &title_);

  // Define a Y axis.  Call once or twice.  The return value increases from 0
  // (and is the 'v' parameter below).
  int define_y(const std::string &name, double min, double max,
               double r = -1, double g = -1, double b = -1);

  // Add a marker to the X axis.  Call any number of times.
  void marker_x(double x, const std::string &value);

  // Add a range to the X axis.  Call any number of times.  Ranges should not
  // overlap.
  void range_x(double xmin, double xmax, const std::string &value);

  // Add a marker to the Y axis.  Call any number of times.
  void marker_y(int v, double y);

  // Add a marker to the Y axis.  Call any number of times.
  void marker_y(int v, double y, const std::string &value);

  // Draw axes.  Call exactly once.
  void axes();

  // Plot one sample.  Call any number of times.  If 'link' is true then a line
  // will be drawn from the previous plot(), provided it was for the same Y
  // axis (same 'v' value).
  void plot(int v, double x, double y, bool link = false);

  // Save as a PNG
  void save(const std::string &path);
private:
  struct Variable {
    std::string name;
    double min;
    double max;
    std::map<double,std::string> markers;
    double r, g, b;
  };

  Cairo::RefPtr<Cairo::Surface> surface;
  Cairo::RefPtr<Cairo::Context> context;
  int width, height;                    // dimensions
  double start, end;                    // X axis range
  std::vector<Variable> variables;      // Y axes
  double mark_size;
  double border;
  double label_space;
  std::string xname;
  std::string title;

  double bleft, bright, btop, bbottom;  // border sizes

  std::map<double,std::string> markers;
  std::map<std::pair<double,double>,std::string> ranges;
  int current_variable;

  inline double xc(double x) const {
    return bleft + (x - start) * (width - bleft - bright) / (end - start);
  }

  inline double yc(int v, double y) const {
    return height - bbottom - (y - variables[v].min) * (height - btop - bbottom) /
      (variables[v].max - variables[v].min);
  }

  void compute_bounds();
  void draw_axes();

};

#endif /* GRAPH_H */
