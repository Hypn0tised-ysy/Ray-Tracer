#ifndef COMMON_H
#define COMMON_H

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>

using std::make_shared;
using std::shared_ptr;

double constexpr Infinity = std::numeric_limits<double>::infinity();
double constexpr PI = 3.1415926535897932385;

double degrees_to_radians(double degrees) { return degrees * PI / 180.0; }

#include "color.h"
#include "ray.h"
#include "vec3.h"

#endif // COMMON_H