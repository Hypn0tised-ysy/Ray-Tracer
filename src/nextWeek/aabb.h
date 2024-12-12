#ifndef AABB_H
#define AABB_H

#include "interval.h"
#include "ray.h"
#include "vec3.h"
#include <algorithm>
class aabb {
public:
  interval x_interval, y_interval, z_interval;

  aabb() {}
  aabb(interval const &x_interval, interval const &y_interval,
       interval const &z_interval)
      : x_interval(x_interval), y_interval(y_interval), z_interval(z_interval) {
  }
  aabb(point3 const &p1, point3 const &p2) {
    x_interval = interval(std::fmin(p1.x, p2.x), std::fmax(p1.x, p2.x));
    y_interval = interval(std::fmin(p1.y, p2.y), std::fmax(p1.y, p2.y));
    z_interval = interval(std::fmin(p1.z, p2.z), std::fmax(p1.z, p2.z));
  }
  aabb(aabb const &a, aabb const &b) {
    x_interval = interval(a.x_interval, b.x_interval);
    y_interval = interval(a.y_interval, b.y_interval);
    z_interval = interval(a.z_interval, b.z_interval);
  }

  const interval &get_axis_interval(int ith_axis_interval) const {
    switch (ith_axis_interval) {
    case 0:
      return x_interval;
    case 1:
      return y_interval;
    case 2:
      return z_interval;
    default:
      return interval(0, 0);
    }
  }

  int longest_axis() const {
    auto lx = x_interval.length();
    auto ly = y_interval.length();
    auto lz = z_interval.length();
    return lx > ly ? (lx > lz ? 0 : 2) : (ly > lz ? 1 : 2);
  }

  bool hit(Ray const &ray, interval hit_range) const {
    double t0, t1;
    double root1, root2;
    point3 const &ray_origin = ray.getOrigin();
    vec3 const &ray_direction = ray.getDirection();

    for (int ith_axis = 0; ith_axis < 3; ith_axis++) {
      interval const &axis_interval = get_axis_interval(ith_axis);
      double const direction_component_reciprocal =
          1.0 / ray_direction[ith_axis];

      root1 = (axis_interval.min - ray_origin[ith_axis]) *
              direction_component_reciprocal;
      root2 = (axis_interval.max - ray_origin[ith_axis]) *
              direction_component_reciprocal;

      t0 = std::fmin(root1, root2);
      t1 = std::fmax(root1, root2);

      hit_range.min = t0 > hit_range.min ? t0 : hit_range.min;
      hit_range.max = t1 < hit_range.max ? t1 : hit_range.max;

      if (hit_range.min > hit_range.max)
        return false;
    }
    return true;
  }

  static const aabb Empty_bbox;
  static const aabb Universe_bbox;
};

const aabb aabb::Empty_bbox(interval::Empty, interval::Empty, interval::Empty);
const aabb aabb::Universe_bbox(interval::Universe, interval::Universe,
                               interval::Universe);

#endif // AABB_H