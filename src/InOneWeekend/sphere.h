#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"
#include "vec3.h"
#include <algorithm>
#include <cmath>

bool clamp(double min, double max, double value) {
  return value <= min || value >= max;
}

class sphere : public hittable {
public:
  sphere(point3 const &_center, double _radius)
      : center(_center), radius(std::fmax(0.0, _radius)) {}

  hit_record calculate_hit_record(Ray const &ray,
                                  double factorOfDirection) const {
    hit_record record;
    record.factorOfDirection = factorOfDirection;
    record.hitPoint = ray.at(factorOfDirection);
    vec3 unitOutwardNormal = (record.hitPoint - center) / radius;
    record.set_surface_normal(ray, unitOutwardNormal);
    return record;
  }

  virtual bool hit(const Ray &ray, double min_factorOfDirection,
                   double max_factorOfDirection,
                   hit_record &record) const override {
    // solve quadratic formula
    vec3 centerMinusRayOrigin = center - ray.getOrigin();
    double a = ray.getDirection() * ray.getDirection(),
           negative_half_b = ray.getDirection() * centerMinusRayOrigin,
           c = centerMinusRayOrigin * centerMinusRayOrigin - radius * radius;
    double delta2 = negative_half_b * negative_half_b - a * c;
    double factorOfDirection;
    if (delta2 < 0.0) {
      return false;
    }

    double delta2_sqrt = std::sqrt(delta2);
    factorOfDirection = (negative_half_b - delta2_sqrt) / a;
    if (clamp(min_factorOfDirection, max_factorOfDirection,
              factorOfDirection)) {
      factorOfDirection = (negative_half_b + delta2_sqrt) / a;
      if (clamp(min_factorOfDirection, max_factorOfDirection,
                factorOfDirection))
        return false;
    }

    record = calculate_hit_record(ray, factorOfDirection);

    return true;
  };

private:
  point3 center;
  double radius;
};

#endif // SPHERE_H