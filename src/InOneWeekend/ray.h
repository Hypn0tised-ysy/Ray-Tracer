#ifndef RAY_H
#define RAY_H

#include "vec3.h"

class Ray {
public:
  point3 const &getOrigin() const { return origin; }
  vec3 const &getDirection() const { return direction; }

  Ray(){};
  Ray(point3 const &_origin, vec3 const &_direction)
      : origin(_origin), direction(_direction){};

  point3 at(double factorOfDirection) const {
    return origin + factorOfDirection * direction;
  }

private:
  point3 origin;
  vec3 direction;
};

#endif