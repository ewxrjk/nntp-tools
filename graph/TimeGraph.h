#ifndef TIMEGRAPH_H
#define TIMEGRAPH_H

#include "Graph.h"

class TimeGraph: public Graph {
public:
  // Define graph dimensions
  inline TimeGraph(int width_, int height_): Graph(width_, height_) {
  }

  // Intelligently choose X range
  void define_x(const std::string &name, time_t start_, time_t end_);

  inline void plot(int v, time_t x, double y, bool link = false) {
    Graph::plot(v, x, y, link);
  }

private:
  static long hour(time_t t);
  static long day(time_t t);
  static long week(time_t t);
  static long month(time_t t);
  static long year(time_t t);
};

#endif /* TIMEGRAPH_H */

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
