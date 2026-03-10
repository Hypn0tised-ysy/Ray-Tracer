#ifndef MOVING_CENTER_H
#define MOVING_CENTER_H

#include "vec3.h"

/*
given time between 0-1
get center position from point A to B linearly
*/
class moving_center {
public:
  double time;
  point3 origin;
  point3 destination;
  moving_center(){};
  moving_center(point3 const &_origin, vec3 const &destination)
      : origin(_origin), destination(destination) {
    displacement = destination - _origin;
  }

  point3 at(double time) const { return origin + time * displacement; }

private:
  vec3 displacement;
};

#endif // MOVING_CENTER_H