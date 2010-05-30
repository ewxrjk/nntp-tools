#include "Graph.h"

int main() {
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
  g.save("t.png");
  return 0;
}
