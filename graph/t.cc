#include "Graph.h"

int main() {
  Graph g(640, 480,
          0.0, 100.0);
  g.set_title("Title");
  // X labels
  g.set_xname("X");
  g.label(0.0, "0");
  g.label(50.0, "50");
  g.label(100.0, "100");
  // Y axes
  g.variable("Y0", 0.0, 10.0);
  g.label(0, 0.0, "0");
  g.label(0, 1.0, "1");
  g.label(0, 2.0, "2");
  g.label(0, 3.0, "3");
  g.label(0, 4.0, "4");
  g.label(0, 5.0, "5");
  g.label(0, 6.0, "6");
  g.label(0, 7.0, "7");
  g.label(0, 8.0, "8");
  g.label(0, 9.0, "9");
  g.label(0, 10.0, "10");
  g.variable("Y1", 0, 5.0);
  g.label(1, 0.0, "0");
  g.label(1, 1.0, "1");
  g.label(1, 2.0, "2");
  g.label(1, 3.0, "3");
  g.label(1, 4.0, "4");
  g.label(1, 5.0, "5");
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
