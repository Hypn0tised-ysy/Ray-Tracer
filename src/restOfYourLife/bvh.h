#ifndef BVH_H
#define BVH_H

#include "aabb.h"
#include "hittable.h"
#include "hittable_list.h"
#include "interval.h"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

class bvh_node : public hittable {
public:
  bvh_node(hittable_list hittable_list)
      : bvh_node(hittable_list.objects, 0, hittable_list.objects.size()){};
  bvh_node(std::vector<std::shared_ptr<hittable>> &objects, size_t start,
           size_t end) {
    bbox = aabb::Empty_bbox;

    for (size_t ith_object = start; ith_object < end; ith_object++) {
      aabb ith_bbox = objects[ith_object]->bounding_box();
      bbox = aabb(bbox, ith_bbox);
    }

    int longest = bbox.longest_axis();

    auto comparator = (longest == 0)
                          ? box_x_compare
                          : ((longest == 1) ? box_y_compare : box_z_compare);

    size_t object_span = end - start;

    if (object_span == 1) {
      left = right = objects[start];
    } else if (object_span == 2) {
      left = objects[start];
      right = objects[start + 1];
    } else {
      std::sort(objects.begin() + start, objects.begin() + end, comparator);
      auto mid = start + object_span / 2;
      left = std::make_shared<bvh_node>(objects, start, mid);
      right = std::make_shared<bvh_node>(objects, mid, end);
    }
  };

  virtual bool hit(const Ray &ray_in, interval ray_range,
                   hit_record &record) const override {
    if (!bbox.hit(ray_in, ray_range))
      return false;

    bool hit_left = left->hit(ray_in, ray_range, record);
    bool hit_right =
        right->hit(ray_in,
                   hit_left ? interval(ray_range.min, record.factorOfDirection)
                            : ray_range,
                   record);

    return hit_left || hit_right;
  }

  virtual aabb bounding_box() const override { return bbox; };

private:
  aabb bbox;
  shared_ptr<hittable> left, right;

  static bool box_compare(const shared_ptr<hittable> &a,
                          const shared_ptr<hittable> &b, int ith_axis) {
    auto a_bbox = a->bounding_box();
    auto b_bbox = b->bounding_box();
    return a_bbox.get_axis_interval(ith_axis).min <
           b_bbox.get_axis_interval(ith_axis).min;
  }
  static bool box_x_compare(const shared_ptr<hittable> &a,
                            const shared_ptr<hittable> &b) {
    return box_compare(a, b, 0);
  }
  static bool box_y_compare(const shared_ptr<hittable> &a,
                            const shared_ptr<hittable> &b) {
    return box_compare(a, b, 1);
  }
  static bool box_z_compare(const shared_ptr<hittable> &a,
                            const shared_ptr<hittable> &b) {
    return box_compare(a, b, 2);
  }
};

#endif // BVH_H