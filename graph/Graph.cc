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
#include "Graph.h"
#include <sstream>

using namespace std;

Graph::Graph(int width_, int height_):
  surface(Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, width_, height_)),
  context(Cairo::Context::create(surface)),
  width(width_), height(height_),
  mark_size(4.0), border(8.0), label_space(4.0),
  current_variable(-1) {
  // The background
  context->set_source_rgb(1.0, 1.0, 1.0);
  context->paint();
  // The default font
  context->select_font_face("serif", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
  context->set_font_size(12.0);
}

void Graph::define_x(const std::string &name, double start_, double end_) {
  xname = name;
  start = start_;
  end = end_;
}

void Graph::define_title(const std::string &title_) {
  title = title_;
}

int Graph::define_y(const std::string &name, double min, double max,
                    double r, double g, double b) {
  static const struct {
    double r, g, b;
  } default_colors[] = {
    { 0.0, 0.75, 0.0 },
    { 0.75, 0.0, 0.0 },
    { 0.0, 0.75, 0.75 },
    { 0.5, 0.5, 0.0 },
  };
  Variable v;
  v.name = name;
  v.min = min;
  v.max = max;
  if(r < 0) {
    const int n = variables.size() % (sizeof default_colors / sizeof default_colors[0]);
    r = default_colors[n].r;
    g = default_colors[n].g;
    b = default_colors[n].b;
  }
  v.r = r;
  v.g = g;
  v.b = b;
  variables.push_back(v);
  return variables.size() - 1;
}

void Graph::marker_x(double x, const string &value) {
  markers[x] = value;
}

void Graph::range_x(double xmin, double xmax, const string &value) {
  ranges[pair<double,double>(xmin, xmax)] = value;
}

void Graph::marker_y(int v, double y) {
  stringstream s;
  s << y;
  variables[v].markers[y] = s.str();
}

void Graph::marker_y(int v, double y, const string &value) {
  variables[v].markers[y] = value;
}

void Graph::axes() {
  compute_bounds();
  draw_axes();
}

void Graph::compute_bounds() {
  Cairo::TextExtents te;
  Cairo::FontExtents fe;

  context->get_font_extents(fe);

  // Title --------------------------------------------------------------------

  btop = border + fe.height + fe.descent + label_space;

  // X axis markers ------------------------------------------------------------

  bbottom = border + fe.descent + fe.height + label_space + fe.descent + fe.height + label_space + mark_size;

  // Left hand Y axis markers --------------------------------------------------

  double max = 0;
  for(map<double,string>::iterator it = variables[0].markers.begin();
      it != variables[0].markers.end();
      ++it) {
    context->get_text_extents(it->second, te);
    if(te.width > max)
      max = te.width;
  }
  context->get_text_extents(variables[0].name, te);
  bleft = max + mark_size + border + label_space + te.width + label_space;

  // Right hand Y axis markers -------------------------------------------------

  if(variables.size() > 1) {
    max = 0;
    for(map<double,string>::iterator it = variables[1].markers.begin();
        it != variables[1].markers.end();
        ++it) {
      context->get_text_extents(it->second, te);
      if(te.width > max)
        max = te.width;
    }
    context->get_text_extents(variables[1].name, te);
    bright = max + mark_size + border + label_space + te.width + label_space;
  } else
    bright = border;
}

void Graph::draw_axes() {
  Cairo::TextExtents te;
  Cairo::FontExtents fe;
  double x, y;

  // Title --------------------------------------------------------------------
  context->set_source_rgb(0, 0, 0);
  context->get_text_extents(title, te);
  x = width / 2 - te.width / 2 - te.x_bearing;
  y = border - te.y_bearing;
  context->move_to(x, y);
  context->show_text(title);
  // TODO use a different font for the title

  // The X axis ---------------------------------------------------------------
  // Markers
  context->get_font_extents(fe);
  for(map<double,string>::iterator it = markers.begin();
      it != markers.end();
      ++it) {
    context->get_text_extents(it->second, te);
    x = xc(it->first) - te.width / 2 - te.x_bearing;
    y = (height - bbottom) + fe.height + label_space;
    if(x < bleft)
      x = bleft;
    if(x + te.width > width - bright)
      x = width - bright - te.width;
    context->move_to(x, y);
    context->show_text(it->second);
  }
  // Ranges
  bool shaded = false;
  for(map<pair<double,double>,string>::iterator it = ranges.begin();
      it != ranges.end();
      ++it) {
    const double xmin = it->first.first;
    const double xmax = it->first.second;
    if(shaded) {
      context->set_source_rgb(0.925, 0.95, 1.0);
      context->rectangle(xc(xmin), btop,
                         xc(xmax) - xc(xmin), height - bbottom - btop);
      context->fill();
    }
    context->set_source_rgb(0.0, 0.0, 0.0);
    context->get_text_extents(it->second, te);
    x = xc((xmin + xmax) / 2) - te.width / 2 - te.x_bearing;
    y = (height - bbottom) + fe.height + label_space;
    if(x < bleft)
      x = bleft;
    if(x + te.width > width - bright)
      x = width - bright - te.width;
    context->move_to(x, y);
    context->show_text(it->second);
    shaded = !shaded;
  }
  // xname
  context->set_source_rgb(0.0, 0.0, 0.0);
  context->get_text_extents(xname, te);
  x = bleft + (width - bleft - bright) / 2 - te.width / 2 - te.x_bearing;
  y = height - border - fe.descent;
  context->move_to(x, y);
  context->show_text(xname);
  // Markers
  context->set_source_rgb(0, 0, 0);
  context->move_to(bleft, height - bbottom);
  context->line_to(width - bright, height - bbottom);
  for(map<double,string>::iterator it = markers.begin();
      it != markers.end();
      ++it) {
    context->move_to(xc(it->first), height - bbottom);
    context->rel_line_to(0, mark_size);
  }
  context->stroke();

  // Left hand Y axis ---------------------------------------------------------

  context->move_to(bleft, btop);
  context->line_to(bleft, height - bbottom);
  for(map<double,string>::iterator it = variables[0].markers.begin();
      it != variables[0].markers.end();
      ++it) {
    context->move_to(bleft + 1, yc(0, it->first));
    context->rel_line_to(-mark_size, 0.0);
  }
  context->set_source_rgb(variables[0].r, variables[0].g, variables[0].b);
  context->stroke();
  for(map<double,string>::iterator it = variables[0].markers.begin();
      it != variables[0].markers.end();
      ++it) {
    context->get_text_extents(it->second, te);
    x = bleft - mark_size - te.width - te.x_bearing - label_space;
    y = yc(0, it->first) - te.height / 2 - te.y_bearing;
    context->move_to(x, y);
    context->show_text(it->second);
  }
  context->get_text_extents(variables[0].name, te);
  x = border - te.x_bearing;
  y = btop + (height - btop - bbottom) / 2 - te.height / 2 - te.y_bearing;
  context->move_to(x, y);
  context->show_text(variables[0].name);

  // Right-hand Y axis --------------------------------------------------------

  if(variables.size() > 1) {
    context->move_to(width - bright, height - bbottom);
    context->line_to(width - bright, btop);
    for(map<double,string>::iterator it = variables[1].markers.begin();
        it != variables[1].markers.end();
        ++it) {
      context->move_to(width - bright - 1, yc(1, it->first));
      context->rel_line_to(mark_size, 0.0);
    }
    context->set_source_rgb(variables[1].r, variables[1].g, variables[1].b);
    context->stroke();
    for(map<double,string>::iterator it = variables[1].markers.begin();
        it != variables[1].markers.end();
        ++it) {
      context->get_text_extents(it->second, te);
      x = (width - bright) + mark_size - te.x_bearing + label_space;
      y = yc(1, it->first) - te.height / 2 - te.y_bearing;
      context->move_to(x, y);
      context->show_text(it->second);
    }
    context->get_text_extents(variables[1].name, te);
    x = width - border - te.width - te.x_bearing;
    y = btop + (height - btop - bbottom) / 2 - te.height / 2 - te.y_bearing;
    context->move_to(x, y);
    context->show_text(variables[1].name);
  }

  // TODO do something sane with additional variables
}

void Graph::plot(int v, double x, double y, bool link) {
  if(v != current_variable && current_variable >= 0) {
    context->set_source_rgb(variables[current_variable].r,
                            variables[current_variable].g,
                            variables[current_variable].b);
    context->stroke();
    current_variable = -1;
  }
  // If there was no previous point, we can't link to it
  if(current_variable < 0)
    link = false;
  if(link)
    // Link from the previous point
    context->line_to(xc(x), yc(v, y));
  // Make a spot
  context->move_to(xc(x), yc(v, y) - 1);
  context->line_to(xc(x), yc(v, y) + 1);
  // Make sure the link to the next point is in the right place
  context->move_to(xc(x), yc(v, y));
  // Remember which variable we're on
  current_variable = v;
}

void Graph::save(const std::string &path) {
  if(current_variable >= 0) {
    // TODO de-dupe
    context->set_source_rgb(variables[current_variable].r,
                            variables[current_variable].g,
                            variables[current_variable].b);
    context->stroke();
    current_variable = -1;
  }
  surface->write_to_png(path);
}
