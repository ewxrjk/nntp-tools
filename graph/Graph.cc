#include "Graph.h"

using namespace std;

Graph::Graph(int width_, int height_,
             double start_, double end_):
  surface(Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, width_, height_)),
  context(Cairo::Context::create(surface)),
  width(width_), height(height_),
  start(start_), end(end_),
  mark_size(4.0), border(8.0), label_space(2.0),
  current_variable(-1) {
  // The background
  context->set_source_rgb(1.0, 1.0, 1.0);
  context->paint();
  // The default font
  context->select_font_face("serif", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
  context->set_font_size(12.0);
}

void Graph::set_xname(const std::string &name) {
  xname = name;
}

void Graph::set_title(const std::string &title_) {
  title = title_;
}

int Graph::variable(const std::string &name, double min, double max,
                    double r, double g, double b) {
  static const struct {
    double r, g, b;
  } default_colors[] = {
    { 0.0, 1.0, 0.0 },
    { 1.0, 0.0, 0.0 },
    { 0.0, 1.0, 1.0 },
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

void Graph::label(double x, const string &value) {
  labels[x] = value;
}

void Graph::range(double xmin, double xmax, const string &value) {
  ranges[pair<double,double>(xmin, xmax)] = value;
}

void Graph::label(int v, double y, const string &value) {
  variables[v].labels[y] = value;
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

  // X axis labels ------------------------------------------------------------

  bbottom = border + fe.descent + fe.height + label_space + fe.descent + fe.height + label_space + mark_size;

  // Left hand Y axis labels --------------------------------------------------

  double max = 0;
  for(map<double,string>::iterator it = variables[0].labels.begin();
      it != variables[0].labels.end();
      ++it) {
    context->get_text_extents(it->second, te);
    if(te.width > max)
      max = te.width;
  }
  context->get_text_extents(variables[0].name, te);
  bleft = max + mark_size + border + label_space + te.width + label_space;

  // Right hand Y axis labels -------------------------------------------------

  if(variables.size() > 1) {
    max = 0;
    for(map<double,string>::iterator it = variables[1].labels.begin();
        it != variables[1].labels.end();
        ++it) {
    context->get_text_extents(it->second, te);
    if(te.width > max)
      max = te.width;
    }
    context->get_text_extents(variables[0].name, te);
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
  // Labels
  context->get_font_extents(fe);
  for(map<double,string>::iterator it = labels.begin();
      it != labels.end();
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
      context->set_source_rgb(0.9, 0.9, 0.9);
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
  for(map<double,string>::iterator it = labels.begin();
      it != labels.end();
      ++it) {
    context->move_to(xc(it->first), height - bbottom);
    context->rel_line_to(0, mark_size);
  }
  context->stroke();

  // Left hand Y axis ---------------------------------------------------------

  context->move_to(bleft, btop);
  context->line_to(bleft, height - bbottom);
  for(map<double,string>::iterator it = variables[0].labels.begin();
      it != variables[0].labels.end();
      ++it) {
    context->move_to(bleft + 1, yc(0, it->first));
    context->rel_line_to(-mark_size, 0.0);
  }
  context->set_source_rgb(variables[0].r, variables[0].g, variables[0].b);
  context->stroke();
  for(map<double,string>::iterator it = variables[0].labels.begin();
      it != variables[0].labels.end();
      ++it) {
    context->get_text_extents(it->second, te);
    x = bleft - mark_size - te.width - te.x_bearing - label_space;
    y = yc(0, it->first) - te.height / 2 - te.y_bearing;
    context->move_to(x, y);
    context->show_text(it->second);
  }
  context->get_text_extents(variables[0].name, te);
  x = border;
  y = btop + (height - btop - bbottom) / 2 - te.height / 2 - te.y_bearing;
  context->move_to(x, y);
  context->show_text(variables[0].name);

  // Right-hand Y axis --------------------------------------------------------

  if(variables.size() > 1) {
    context->move_to(width - bright, height - bbottom);
    context->line_to(width - bright, btop);
    for(map<double,string>::iterator it = variables[1].labels.begin();
        it != variables[1].labels.end();
        ++it) {
      context->move_to(width - bright - 1, yc(1, it->first));
      context->rel_line_to(mark_size, 0.0);
      // TODO text
    }
    context->set_source_rgb(variables[1].r, variables[1].g, variables[1].b);
    context->stroke();
  }
  for(map<double,string>::iterator it = variables[1].labels.begin();
      it != variables[1].labels.end();
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

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
