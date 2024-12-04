#ifndef COLOR_H
#define COLOR_H

#include <iostream>
#include <ostream>

#include "vec3.h"

using color3 = vec3;

void write_color(std::ostream &out, color3 const &color) {
  int rIntensity = int(255.999 * color.r), gIntensity = int(255.999 * color.g),
      bIntenisty = int(255.999 * color.b);
  out << rIntensity << " " << gIntensity << " " << bIntenisty << "\n";
}

#endif