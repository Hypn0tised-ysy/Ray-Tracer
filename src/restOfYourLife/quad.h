#ifndef QUAD_H
#define QUAD_H

#include "aabb.h"
#include "hittable.h"
#include "restOfYourLife/hittable_list.h"
#include "restOfYourLife/interval.h"
#include "restOfYourLife/material.h"
#include "restOfYourLife/ray.h"
#include "restOfYourLife/vec3.h"
#include <cmath>
#include <memory>
class quad : public hittable {
public:
  quad(point3 _p0, vec3 _u, vec3 _v, std::shared_ptr<Material> _material)
      : p0(_p0), u(_u), v(_v), material(_material) {
    vec3 n = crossProduct(u, v);
    area = n.norm();
    normal = unit_vector(n);
    D = dotProduct(normal, p0);
    w = n / dotProduct(n, n);

    calculate_bbox();
  };

  bool hit(const Ray &ray, interval ray_range,
           hit_record &record) const override {
    double factroOfDirection;
    if (solveIntersection(ray, ray_range, factroOfDirection)) {
      auto intersection = ray.at(factroOfDirection);
      if (is_interior(intersection, record)) {
        generate_hit_record(record, ray, factroOfDirection);
        return true;
      }
    }
    return false;
  }

  aabb bounding_box() const override { return bbox; }

  void generate_hit_record(hit_record &record, Ray const &ray,
                           double factorOfDirection) const {
    record.material = material;
    record.factorOfDirection = factorOfDirection;
    record.hitPoint = ray.at(factorOfDirection);
    record.set_surface_normal(ray, normal);
  }

  double pdf_value(point3 const &origin, vec3 const &direction) const override {
    hit_record record;
    if (!this->hit(Ray(origin, direction), interval(0.001, INFINITY_DOUBLE),
                   record))
      return 0.0;

    auto distance_squared = record.factorOfDirection *
                            record.factorOfDirection * direction.norm_square();
    auto cosine = std::fabs(dotProduct(record.normalAgainstRay, direction) /
                            direction.norm());

    return distance_squared / (cosine * area);
  }

  vec3 random(const point3 &origin) const {
    auto random_u = random_double(0, 1);
    auto random_v = random_double(0, 1);
    auto random_point = p0 + random_u * u + random_v * v;
    return unit_vector(random_point - origin);
  }

private:
  // 给定初始点和u，v向量生成平行四边形
  point3 p0;
  vec3 u, v;
  vec3
      w; // https://raytracing.github.io/books/RayTracingTheNextWeek.html#perlinnoise/usingblocksofrandomnumbers
  aabb bbox;
  std::shared_ptr<Material> material;
  vec3 normal;
  double D; // 平面方程系数
  double area;
  void calculate_bbox() {
    aabb diagonal1(p0, p0 + u + v);
    aabb diagonal2(p0 + u, p0 + v);
    bbox = aabb(diagonal1, diagonal2);
  }

  bool solveIntersection(Ray const &ray, interval ray_range,
                         double &factorOfDirection) const {
    auto denominal = dotProduct(normal, ray.getDirection());

    double avoid_diveded_by_zero = 1e-8;
    if (std::fabs(denominal) < avoid_diveded_by_zero)
      return false;

    auto t = (D - dotProduct(normal, ray.getOrigin())) / denominal;
    if (!ray_range.contain(t))
      return false;

    factorOfDirection = t;
    return true;
  }

  bool is_interior(point3 const &intersection, hit_record &record) const {
    vec3 p0_to_hitpoint = intersection - p0;
    auto a = dotProduct(w, crossProduct(p0_to_hitpoint, v));
    auto b = dotProduct(w, crossProduct(u, p0_to_hitpoint));

    interval unit_interval = interval(0, 1);

    if (!unit_interval.contain(a) || !unit_interval.contain(b))
      return false;

    record.textureCoordinate.u = a;
    record.textureCoordinate.v = b;
    return true;
  }
};
inline shared_ptr<hittable_list> box(const point3 &a, const point3 &b,
                                     shared_ptr<Material> mat) {
  // Returns the 3D box (six sides) that contains the two opposite vertices a &
  // b.

  auto sides = make_shared<hittable_list>();

  // Construct the two opposite vertices with the minimum and maximum
  // coordinates.
  auto min =
      point3(std::fmin(a.x, b.x), std::fmin(a.y, b.y), std::fmin(a.z, b.z));
  auto max =
      point3(std::fmax(a.x, b.x), std::fmax(a.y, b.y), std::fmax(a.z, b.z));

  auto dx = vec3(max.x - min.x, 0, 0);
  auto dy = vec3(0, max.y - min.y, 0);
  auto dz = vec3(0, 0, max.z - min.z);

  sides->add(make_shared<quad>(point3(min.x, min.y, max.z), dx, dy,
                               mat)); // front
  sides->add(make_shared<quad>(point3(max.x, min.y, max.z), -dz, dy,
                               mat)); // right
  sides->add(make_shared<quad>(point3(max.x, min.y, min.z), -dx, dy,
                               mat)); // back
  sides->add(make_shared<quad>(point3(min.x, min.y, min.z), dz, dy,
                               mat)); // left
  sides->add(make_shared<quad>(point3(min.x, max.y, max.z), dx, -dz,
                               mat)); // top
  sides->add(make_shared<quad>(point3(min.x, min.y, min.z), dx, dz,
                               mat)); // bottom

  return sides;
}

#endif // QUAD_H