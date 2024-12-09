#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"
#include "interval.h"
#include "ray.h"
#include "vec3.h"
#include <algorithm>
#include <cmath>
#include <memory>
class sphere : public hittable {
public:
  sphere(point3 const &_center, double _radius,
         std::shared_ptr<Material> _material)
      : center(_center), radius(std::fmax(0.0, _radius)), material(_material) {}

  hit_record generate_hit_record(Ray const &ray,
                                 double factorOfDirection) const {
    hit_record record;

    record.material = material;
    record.factorOfDirection = factorOfDirection;
    record.hitPoint = ray.at(factorOfDirection);
    vec3 unitOutwardNormal = (record.hitPoint - center) / radius;
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
  point3 center;
  double radius;
  std::shared_ptr<Material> material;

  bool solveIntersection(Ray const &ray, interval ray_range,
                         double &factorOfDirection) const {
    // solve quadratic formula
    vec3 centerMinusRayOrigin = center - ray.getOrigin();
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
};

#endif // SPHERE_H