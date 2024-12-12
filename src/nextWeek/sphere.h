#ifndef SPHERE_H
#define SPHERE_H

#include "aabb.h"
#include "hittable.h"
#include "interval.h"
#include "moving_center.h"
#include "ray.h"
#include "vec3.h"

#include <algorithm>
#include <cmath>
#include <memory>

class sphere : public hittable {
public:
  // stationary
  sphere(point3 const &_center, double _radius,
         std::shared_ptr<Material> _material)
      : center(_center, _center), radius(std::fmax(0.0, _radius)),
        material(_material) {
    calculate_bbox();
  }

  // moving
  sphere(point3 const &origin, point3 const &destination, double radius,
         std::shared_ptr<Material> material)
      : center(origin, destination), radius(std::fmax(0.0, radius)),
        material(material) {
    calculate_bbox();
  }

  aabb bounding_box() const override { return bbox; }

  hit_record generate_hit_record(Ray const &ray,
                                 double factorOfDirection) const {
    hit_record record;

    record.material = material;
    record.factorOfDirection = factorOfDirection;
    record.hitPoint = ray.at(factorOfDirection);
    vec3 unitOutwardNormal =
        (record.hitPoint - center.at(ray.getTime())) / radius;
    record.set_surface_normal(ray, unitOutwardNormal);

    return record;
  }

  virtual bool hit(const Ray &ray, interval ray_range,
                   hit_record &record) const override {

    double factorOfDirection;
    if (solveIntersection(ray, ray_range, factorOfDirection)) {
      record = generate_hit_record(ray, factorOfDirection);

      return true;
    }

    return false;
  };

private:
  moving_center center;
  double radius;
  std::shared_ptr<Material> material;
  aabb bbox;

  bool solveIntersection(Ray const &ray, interval ray_range,
                         double &factorOfDirection) const {
    // solve quadratic formula
    double time = ray.getTime();
    vec3 centerMinusRayOrigin = center.at(time) - ray.getOrigin();
    double a = ray.getDirection() * ray.getDirection(),
           negative_half_b = ray.getDirection() * centerMinusRayOrigin,
           c = centerMinusRayOrigin * centerMinusRayOrigin - radius * radius;
    double delta2 = negative_half_b * negative_half_b - a * c;
    if (delta2 < 0.0) {
      return false;
    }

    double delta2_sqrt = std::sqrt(delta2);
    factorOfDirection = (negative_half_b - delta2_sqrt) / a;
    if (!ray_range.surroud(factorOfDirection)) {
      factorOfDirection = (negative_half_b + delta2_sqrt) / a;
      if (!ray_range.surroud(factorOfDirection))
        return false;
    }

    return true;
  }

  void calculate_bbox() {
    vec3 r(radius, radius, radius);
    aabb bbox_t0 = aabb(center.at(0) - r, center.at(1) + r);
    aabb bbox_t1 = aabb(center.at(1) - r, center.at(1) + r);
    bbox = aabb(bbox_t0, bbox_t1);
  }
};

#endif // SPHERE_H