#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include "aabb.h"
#include "hittable.h"
#include "interval.h"
#include "nextWeek/common.h"
#include "nextWeek/vec3.h"
#include "ray.h"

#include <cstdlib>
#include <memory>
#include <vector>

using std::make_shared;
using std::shared_ptr;

class hittable_list : public hittable {
public:
  std::vector<shared_ptr<hittable>> objects;

  hittable_list() {}
  hittable_list(shared_ptr<hittable> object) { add(object); }

  void clear() { objects.clear(); }

  void add(shared_ptr<hittable> object) {
    objects.push_back(object);
    bbox = aabb(bbox, object->bounding_box());
  }

  bool hit(Ray const &ray, interval ray_range,
           hit_record &record) const override {
    hit_record temp_record;
    bool hit_anything = false;
    for (const auto &object : objects) {
      if (object->hit(ray, ray_range, temp_record)) {
        hit_anything = true;
        ray_range.max = temp_record.factorOfDirection;
        record = temp_record;
      }
    }
    return hit_anything;
  }

  double pdf_value(point3 const &origin, vec3 const &direction) const override {
    auto weight = 1.0 / objects.size();
    auto sum = 0.0;

    for (auto const &object : objects) {
      sum += weight * object->pdf_value(origin, direction);
    }

    return sum;
  }

  vec3 random(point3 const &origin) const override {
    auto int_size = int(objects.size());
    return objects[random_int(0, int_size - 1)]->random(origin);
  }

  aabb bounding_box() const override { return bbox; }

private:
  aabb bbox;
};

#endif // HITTABLE_LIST_H