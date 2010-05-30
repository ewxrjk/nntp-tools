#ifndef GRAPH_H
#define GRAPH_H

#include <cairomm/cairomm.h>
#include <string>
#include <vector>
#include <map>

// Graph-drawing class
class Graph {
public:
  // Define graph dimensions and X scale
  Graph(int width_, int height_,
        double start_, double end_);

  // Define a variable
  int variable(const std::string &name, double min, double max,
               double r = -1, double g = -1, double b = -1);

  // Add an X label
  void label(double x, const std::string &value);

  // Add a Y label
  void label(int v, double y, const std::string &value);

  // Draw axes
  //
  // This should be after all calls to label() and before all calls to plot().
  void axes();

  // Plot one sample
  void plot(int v, double x, double y, bool link = false);

  // Save as a PNG
  void save(const std::string &path);
private:
  struct Variable {
    std::string name;
    double min;
    double max;
    std::map<double,std::string> labels;
    double r, g, b;
  };

  Cairo::RefPtr<Cairo::Surface> surface;
  Cairo::RefPtr<Cairo::Context> context;
  int width, height;                    // dimensions
  double start, end;                    // X axis range
  std::vector<Variable> variables;      // Y axes

  double bleft, bright, btop, bbottom;  // border sizes

  std::map<double,std::string> labels;
  int current_variable;

  inline double xc(double x) const {
    return bleft + (x - start) * (width - bleft - bright) / (end - start);
  }

  inline double yc(int v, double y) const {
    return bleft + (y - variables[v].min) * (height - btop - bbottom) /
      (variables[v].max - variables[v].min);
  }
};

#endif /* GRAPH_H */

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
