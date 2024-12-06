#ifndef INTERVAL_H
#define INTERVAL_H

#include <limits>

double constexpr INFINITY_DOUBLE = std::numeric_limits<double>::infinity();
class interval {
public:
  double min, max;

  interval() : min(+INFINITY_DOUBLE), max(-INFINITY_DOUBLE) {}
  interval(double min, double max) : min(min), max(max) {}

  double size() const { return max - min; }

  bool contain(double value) const { return value >= min && value <= max; }

  bool surroud(double value) const { return value > min && value < max; }

  double clamp(double value) const {
    if (value < min)
      return min;
    if (value > max)
      return max;
    return value;
  }

  static interval const Empty, Universe;
};

interval const interval::Empty(+INFINITY_DOUBLE, -INFINITY_DOUBLE);
interval const interval::Universe(-INFINITY_DOUBLE, +INFINITY_DOUBLE);

#endif // INTERVAL_H