#ifndef CONSTANT_MEDIUM_H
#define CONSTANT_MEDIUM_H

#include "aabb.h"
#include "color.h"
#include "common.h"
#include "hittable.h"
#include "interval.h"
#include "material.h"
#include "ray.h"
#include "vec3.h"
#include <cmath>
#include <memory>
class constant_medium : public hittable {
public:
  constant_medium(shared_ptr<hittable> _boundary, double density,
                  shared_ptr<texture> tex)
      : boundary(_boundary), negative_inverse_density(-1 / density),
        phase_function(make_shared<isotropic>(tex)) {}
  constant_medium(shared_ptr<hittable> _boundary, double density,
                  color3 const &albedo)
      : boundary(_boundary), negative_inverse_density(-1 / density),
        phase_function(make_shared<isotropic>(albedo)) {}
  bool hit(const Ray &ray, interval ray_range,
           hit_record &record) const override {
    hit_record rec1, rec2;

    if (!boundary->hit(ray, interval::Universe, rec1))
      return false;
    if (!boundary->hit(
            ray, interval(rec1.factorOfDirection + 0.0001, INFINITY_DOUBLE),
            rec2))
      return false;

    if (rec1.factorOfDirection < ray_range.min)
      rec1.factorOfDirection = ray_range.min;
    if (rec2.factorOfDirection > ray_range.max)
      rec2.factorOfDirection = ray_range.max;
    if (rec1.factorOfDirection < 0)
      rec1.factorOfDirection = 0;
    if (rec1.factorOfDirection > rec2.factorOfDirection) // 真会有这种情况吗=-=
      return false;

    double ray_length = ray.getDirection().norm();
    double distance_inside_boundary =
        (rec2.factorOfDirection - rec1.factorOfDirection) * ray_length;
    double hit_distance = negative_inverse_density *
                          std::log(random_double()); // 这里用了一点指数分布应该

    if (hit_distance > distance_inside_boundary)
      return false;

    record.factorOfDirection =
        rec1.factorOfDirection + hit_distance / ray_length;
    record.frontFace = true;
    record.hitPoint = ray.at(record.factorOfDirection);
    record.normalAgainstRay =
        vec3(1, 0, 0); // 这个值似乎不重要，毕竟散射方向是随机的
    record.material = phase_function;

    return true;
  }

  aabb bounding_box() const override { return boundary->bounding_box(); };

private:
  std::shared_ptr<hittable> boundary;
  double negative_inverse_density;
  shared_ptr<Material> phase_function;
};

#endif