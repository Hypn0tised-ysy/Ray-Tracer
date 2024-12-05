#ifndef HITTABLE_H
#define HITTABLE_H

#include "ray.h"
#include "vec3.h"

class hit_record {
public:
  double factorOfDirection;
  point3 hitPoint;
  vec3 normalAgainstRay;
  bool frontFace;
  void set_surface_normal(const Ray &ray, const vec3 &unitOutwardNormal) {
    frontFace = ray.getDirection() * unitOutwardNormal < 0;
    normalAgainstRay = frontFace ? unitOutwardNormal : -unitOutwardNormal;
  }
};

class hittable {
public:
  virtual bool hit(const Ray &ray, double min_factorOfDirection,
                   double max_factorOfDirection, hit_record &record) const = 0;
  virtual ~hittable() = default;
};

#endif