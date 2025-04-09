#ifndef RAY_H
#define RAY_H

#include "vec3.h"

class Ray {
public:
  point3 const &getOrigin() const { return getorigin; }
  vec3 const &getDirection() const { return direction; }
  double const &getTime() const { return time; }

  Ray(){};
  Ray(point3 const &_origin, vec3 const &_direction, double time)
      : getorigin(_origin), direction(_direction), time(time){};

  point3 at(double factorOfDirection) const {
    return getorigin + factorOfDirection * direction;
  }

private:
  point3 getorigin;
  vec3 direction;
  double time;
};

#endif