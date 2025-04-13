#ifndef COLOR_H
#define COLOR_H

#include <cmath>
#include <iostream>
#include <ostream>

#include "interval.h"
#include "vec3.h"

using color3 = vec3;

double linear_to_gamma(double linear_value) {
  if (linear_value > 0)
    return std::sqrt(linear_value);
  return 0;
}

void write_color(std::ostream &out, color3 const &color) {
  static interval const intensity_range(0.0, 0.999);

  auto r = color.r;
  auto g = color.g;
  auto b = color.b;

  // remove NaN->NaN doesn't equal itself
  if (r != r)
    r = 0.0;
  if (g != g)
    g = 0.0;
  if (b != b)
    b = 0.0;

  r = linear_to_gamma(color.r);
  g = linear_to_gamma(color.g);
  b = linear_to_gamma(color.b);

  int rIntensity = int(256 * intensity_range.clamp(r)),
      gIntensity = int(256 * intensity_range.clamp(g)),
      bIntensity = int(256 * intensity_range.clamp(b));

  out << rIntensity << " " << gIntensity << " " << bIntensity << "\n";
}

#endif