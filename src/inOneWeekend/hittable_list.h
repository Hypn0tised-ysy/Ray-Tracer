#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include "hittable.h"
#include "interval.h"
#include "ray.h"

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

  void add(shared_ptr<hittable> object) { objects.push_back(object); }

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
};

#endif // HITTABLE_LIST_H