#include "TimeGraph.h"
#include <sstream>
#include <iomanip>

using namespace std;

void TimeGraph::define_x(const string &name, time_t start_, time_t end_) {
  if(hour(start_) == hour(end_)) {
    int h = hour(start_);
    time_t t = h * 3600;
    Graph::define_x(name, t, t + 3600);
    for(int n = 0; n < 60; n += 10) {
      stringstream s;
      s << setw(2) << setfill('0') << h % 24 << ":" << setw(2) << n;
      range_x(t + n * 60, t + (n + 10) * 60, "");
      marker_x(t + n * 60, s.str());
    }
    stringstream s;
    s << setw(2) << setfill('0') << (h + 1) % 24 << ":" << setw(2) << 0;
    marker_x(t + 60 * 60, s.str());
  } else if(day(start_) == day(end_)) {
    int d = day(start_);
    time_t t = d * 86400;
    Graph::define_x(name, t, t + 86400);
    for(int n = 0; n < 24; ++n) {
      stringstream s;
      if(n % 3 == 0)
	s << n << ":00";
      range_x(t + n * 3600, t + (n + 1) * 3600, "");
      marker_x(t + n * 3600, s.str());
    }
    marker_x(t + 24 * 3600, "0:00");
  } else if(week(start_) == week(end_)) {
    // TODO bizarre choice of start of week (due to time_t(0) being a Thursday)
    static const char *const dayname[] =
      { "Thu", "Fri", "Sat", "Sun", "Mon", "Tue", "Wed" };
    int w = week(start_);
    time_t t = w * 86400 * 7;
    Graph::define_x(name, t, t + 86400 * 7);
    for(int n = 0; n < 7; ++n)
      range_x(t + n * 86400, t + (n + 1) * 86400, dayname[n]);
  } else if(month(start_) == month(end_)) {
    // TODO
  } else if(year(start_) == year(end_)) {
    // TODO
  } else {
    // TODO
  }
}

long TimeGraph::hour(time_t t) {
  return t / 3600;
}

long TimeGraph::day(time_t t) {
  return t / 86400;
}

long TimeGraph::week(time_t t) {
  return t / (86400 * 7);
}

long TimeGraph::month(time_t t) {
  struct tm r;
  localtime_r(&t, &r);
  return (r.tm_year + 1900) * 12 + r.tm_mon;
}

long TimeGraph::year(time_t t) {
  struct tm r;
  localtime_r(&t, &r);
  return r.tm_year + 1900;
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
