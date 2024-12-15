#ifndef HITTABLE_H
#define HITTABLE_H

#include "aabb.h"
#include "common.h"
#include "interval.h"
#include "ray.h"
#include "texture.h"
#include <memory>

class Material;
class hit_record {
public:
  double factorOfDirection;
  bool frontFace;
  point3 hitPoint;
  vec3 normalAgainstRay;
  std::shared_ptr<Material> material;
  texture_coordinate textureCoordinate;

  void set_surface_normal(const Ray &ray, const vec3 &unitOutwardNormal) {
    frontFace = ray.getDirection() * unitOutwardNormal < 0;
    normalAgainstRay = frontFace ? unitOutwardNormal : -unitOutwardNormal;
  }
};

class hittable {
public:
  virtual bool hit(const Ray &ray, interval ray_range,
                   hit_record &record) const = 0;
  virtual ~hittable() = default;
  virtual aabb bounding_box() const = 0;
};

#endif