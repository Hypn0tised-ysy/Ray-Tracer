#ifndef SPHERE_H
#define SPHERE_H

#include "aabb.h"
#include "hittable.h"
#include "interval.h"
#include "moving_center.h"
#include "nextWeek/common.h"
#include "ray.h"
#include "restOfYourLife/orthonormalbasis.h"
#include "texture.h"
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

    record.textureCoordinate.point = record.normalAgainstRay;
    get_sphere_uv(record.textureCoordinate);

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

  static void get_sphere_uv(texture_coordinate &tex_coordinate) {
    auto point = tex_coordinate.point;

    // sphere coordinate base
    auto theta = std::acos(-point.y);
    auto phi = std::atan2(-point.z, point.x) + PI;

    tex_coordinate.u = phi / (2 * PI);
    tex_coordinate.v = theta / PI;
  }

  double pdf_value(point3 const &origin, vec3 const &direction) const override {
    hit_record record;
    if (!this->hit(Ray(origin, direction), interval(0.001, INFINITY_DOUBLE),
                   record))
      return 0;

    auto distance_squared = (center.at(0) - origin).norm_square();
    auto cos_theta_max = std::sqrt(1 - radius * radius / distance_squared);
    auto solid_angle = 2 * PI * (1 - cos_theta_max);

    return 1 / solid_angle;
  }

  vec3 random(point3 const &origin) const override {
    vec3 direction = center.at(0) - origin;
    auto distance_squared = direction.norm_square();
    onb uvw(direction);
    return uvw.transform(random_to_sphere(radius, distance_squared));
  }

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
  static vec3 random_to_sphere(double r, double distance_squared) {
    auto r1 = random_double();
    auto r2 = random_double();
    auto z = 1 + r2 * (std::sqrt(1 - r * r / distance_squared) - 1);

    auto phi = 2 * PI * r1;
    auto x = std::cos(phi) * std::sqrt(1 - z * z);
    auto y = std::sin(phi) * std::sqrt(1 - z * z);

    return vec3(x, y, z);
  }
};

#endif // SPHERE_H