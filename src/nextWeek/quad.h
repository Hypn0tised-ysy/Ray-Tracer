#ifndef QUAD_H
#define QUAD_H

#include "nextWeek/aabb.h"
#include "nextWeek/hittable.h"
#include "nextWeek/interval.h"
#include "nextWeek/material.h"
#include "nextWeek/ray.h"
#include "nextWeek/vec3.h"
#include <cmath>
#include <memory>
class quad : public hittable {
public:
  quad(point3 _p0, vec3 _u, vec3 _v, std::shared_ptr<Material> _material)
      : p0(_p0), u(_u), v(_v), material(_material) {
    vec3 n = crossProduct(u, v);
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

#endif // QUAD_H