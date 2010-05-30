#include "Graph.h"

using namespace std;

Graph::Graph(int width_, int height_,
             double start_, double end_):
  surface(Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, width_, height_)),
  context(Cairo::Context::create(surface)),
  width(width_), height(height_),
  start(start_), end(end_),
  current_variable(-1) {
  // The background
  context->set_source_rgb(1.0, 1.0, 1.0);
  context->paint();
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

void Graph::label(int v, double y, const string &value) {
  variables[v].labels[y] = value;
}

void Graph::axes() {
  // TODO calculate borders to fit labels
  bleft = bright = btop = bbottom = 16;

  // TODO title

  // The X axis ---------------------------------------------------------------
  context->move_to(bleft, height - bbottom);
  context->line_to(width - bright, height - bbottom);
  for(map<double,string>::iterator it = labels.begin();
      it != labels.end();
      ++it) {
    context->move_to(xc(it->first), height - bbottom);
    context->rel_line_to(0, 4.0);
    // TODO text
  }
  context->set_source_rgb(0, 0, 0);
  context->stroke();

  // Left hand Y axis ---------------------------------------------------------

  context->move_to(bleft, btop);
  context->line_to(bleft, height - bbottom);
  for(map<double,string>::iterator it = variables[0].labels.begin();
      it != variables[0].labels.end();
      ++it) {
    context->move_to(bleft, yc(0, it->first));
    context->rel_line_to(-4.0, 0.0);
    // TODO text
  }
  context->set_source_rgb(variables[0].r, variables[0].g, variables[0].b);
  context->stroke();

  // Right-hand Y axis --------------------------------------------------------

  if(variables.size() > 1) {
    context->move_to(width - bright, height - bbottom);
    context->line_to(width - bright, btop);
    for(map<double,string>::iterator it = variables[1].labels.begin();
        it != variables[1].labels.end();
        ++it) {
      context->move_to(width - bright, yc(1, it->first));
      context->rel_line_to(4.0, 0.0);
      // TODO text
    }
    context->set_source_rgb(variables[1].r, variables[1].g, variables[1].b);
    context->stroke();
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

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
