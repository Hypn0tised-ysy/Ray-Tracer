#ifndef INTERVAL_H
#define INTERVAL_H

class interval {
public:
  double min, max;

  interval() : min(+Infinity_double), max(-Infinity_double) {}
  interval(double min, double max) : min(min), max(max) {}

  double size() const { return max - min; }

  bool contain(double value) const { return value >= min && value <= max; }

  bool surroud(double value) const { return value > min && value < max; }

  static interval const Empty, Universe;
};

interval const interval::Empty(+Infinity_double, -Infinity_double);
interval const interval::Universe(-Infinity_double, +Infinity_double);

#endif // INTERVAL_H