#ifndef COMMON_H
#define COMMON_H

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>

using std::make_shared;
using std::shared_ptr;

double constexpr Infinity_double = std::numeric_limits<double>::infinity();
double constexpr PI = 3.1415926535897932385;

double degrees_to_radians(double degrees) { return degrees * PI / 180.0; }

double random_double() {
  // return a double in [0, 1)
  return std::rand() / (RAND_MAX + 1.0);
}

double random_double(double min, double max) {
  // return a double in [min, max)
  return min + (max - min) * random_double();
}

int random_int(int min, int max) {
  // generate a random int in [min,max]
  double random_number = random_double(min, max + 1);
  return int(random_number);
}

#endif // COMMON_H