#ifndef HITTABLE_H
#define HITTABLE_H

#include "aabb.h"
#include "common.h"
#include "interval.h"
#include "ray.h"
#include "texture.h"
#include "vec3.h"
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

  virtual double pdf_value(point3 const &origin, vec3 const &direction) const {
    return 0.0;
  }

  virtual vec3 random(point3 const &origin) const {
    return vec3(1.0, 0.0, 0.0);
  }
};

class translate : public hittable {
public:
  translate(shared_ptr<hittable> object, const vec3 &offset)
      : object(object), offset(offset) {}
  bool hit(const Ray &ray, interval ray_range,
           hit_record &record) const override {
    Ray offset_ray(ray.getOrigin() - offset, ray.getDirection(), ray.getTime());
    if (!object->hit(offset_ray, ray_range, record))
      return false;

    record.hitPoint += offset;
    return true;
  }

  aabb bounding_box() const override {
    aabb bbox = object->bounding_box();
    bbox.x_interval = bbox.x_interval + offset.x;
    bbox.y_interval = bbox.y_interval + offset.y;
    bbox.z_interval = bbox.z_interval + offset.z;
    return bbox;
  }

private:
  shared_ptr<hittable> object; // or instance of primitive
  vec3 offset;
  aabb bbox;
};

class rotate_y : public hittable { // 暂时只考虑绕y的旋转
public:
  rotate_y(shared_ptr<hittable> object, double angle) : object(object) {
    auto radians = degrees_to_radians(angle);
    sin_theta = std::sin(radians);
    cos_theta = std::cos(radians);
    bbox = object->bounding_box();

    point3 min(INFINITY_DOUBLE, INFINITY_DOUBLE, INFINITY_DOUBLE);
    point3 max(-INFINITY_DOUBLE, -INFINITY_DOUBLE, -INFINITY_DOUBLE);

    for (int i = 0; i < 2; i++) {
      for (int j = 0; j < 2; j++) {
        for (int k = 0; k < 2; k++) {
          auto x = i * bbox.x_interval.max + (1 - i) * bbox.x_interval.min;
          auto y = j * bbox.y_interval.max + (1 - j) * bbox.y_interval.min;
          auto z = k * bbox.z_interval.max + (1 - k) * bbox.z_interval.min;

          auto newx = cos_theta * x + sin_theta * z;
          auto newz = -sin_theta * x + cos_theta * z;

          vec3 tester(newx, y, newz);

          for (int c = 0; c < 3; c++) {
            min[c] = std::fmin(min[c], tester[c]);
            max[c] = std::fmax(max[c], tester[c]);
          }
        }
      }
    }

    bbox = aabb(min, max);
  }
  bool hit(const Ray &r, interval ray_t, hit_record &rec) const override {

    // Transform the ray from world space to object space.

    auto origin =
        point3((cos_theta * r.getOrigin().x) - (sin_theta * r.getOrigin().z),
               r.getOrigin().y,
               (sin_theta * r.getOrigin().x) + (cos_theta * r.getOrigin().z));

    auto direction = vec3(
        (cos_theta * r.getDirection().x) - (sin_theta * r.getDirection().z),
        r.getDirection().y,
        (sin_theta * r.getDirection().x) + (cos_theta * r.getDirection().z));

    Ray rotated_r(origin, direction, r.getTime());

    // Determine whether an intersection exists in object space (and if so,
    // where).

    if (!object->hit(rotated_r, ray_t, rec))
      return false;

    // Transform the intersection from object space back to world space.

    rec.hitPoint =
        point3((cos_theta * rec.hitPoint.x) + (sin_theta * rec.hitPoint.z),
               rec.hitPoint.y,
               (-sin_theta * rec.hitPoint.x) + (cos_theta * rec.hitPoint.z));

    rec.normalAgainstRay = vec3((cos_theta * rec.normalAgainstRay.x) +
                                    (sin_theta * rec.normalAgainstRay.z),
                                rec.normalAgainstRay.y,
                                (-sin_theta * rec.normalAgainstRay.x) +
                                    (cos_theta * rec.normalAgainstRay.z));

    return true;
  }
  aabb bounding_box() const override { return bbox; }

private:
  shared_ptr<hittable> object;
  double sin_theta;
  double cos_theta;
  aabb bbox;
};

#endif