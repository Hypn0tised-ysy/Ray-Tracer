#ifndef COLOR_H
#define COLOR_H

#include <iostream>
#include <ostream>

#include "interval.h"
#include "vec3.h"

using color3 = vec3;

void write_color(std::ostream &out, color3 const &color) {
  static interval const intensity_range(0.0, 0.999);

  int rIntensity = int(256 * intensity_range.clamp(color.r)),
      gIntensity = int(256 * intensity_range.clamp(color.g)),
      bIntensity = int(256 * intensity_range.clamp(color.b));

  out << rIntensity << " " << gIntensity << " " << bIntensity << "\n";
}

#endif