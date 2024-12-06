#include "color.h"
#include "common.h"

#include <cassert>
#include <cmath>
#include <memory>
#include <ostream>

#include "hittable.h"
#include "hittable_list.h"
#include "interval.h"
#include "sphere.h"
#include "vec3.h"

template <typename toBlend>
toBlend lerp(double fromStartToEnd, toBlend &startValue, toBlend &endValue) {
  assert(fromStartToEnd >= 0.0 && fromStartToEnd <= 1.0);
  toBlend blendValue =
      (1 - fromStartToEnd) * startValue + fromStartToEnd * endValue;
  return blendValue;
}

color3 background_color(Ray const &ray) {
  color3 result;
  vec3 unit_direction = unit_vector(ray.getDirection());
  double yFromBottomToTop = 0.5 * (unit_direction.y + 1.0);
  color3 blue(0.5, 0.7, 1.0), white(1.0, 1.0, 1.0);
  result = lerp(yFromBottomToTop, white, blue);
  return result;
}

color3 ray_color(Ray const &ray, hittable const &world_objects) {
  hit_record record;
  if (world_objects.hit(ray, interval(0.0, Infinity_double), record)) {
    return 0.5 * (record.normalAgainstRay + color3(1.0, 1.0, 1.0));
  } else
    return background_color(ray);
}

int main() {
  // image
  double constexpr aspect_ratio = 16.0 / 9.0;
  int constexpr WIDTH_IMAGE = 400;
  int constexpr HEIGHT_IMAGE =
      int(WIDTH_IMAGE / aspect_ratio) < 1 ? 1 : int(WIDTH_IMAGE / aspect_ratio);

  // camera
  double constexpr FOCAL_LENGTH = 1.0;
  double height_viewport = 2.0;
  double width_viewport =
      height_viewport * (double(WIDTH_IMAGE) / HEIGHT_IMAGE);
  point3 camera_center = point3(0, 0, 0);

  // viewport
  vec3 u_viewport(width_viewport, 0.0, 0.0);
  vec3 v_viewport(0.0, -height_viewport, 0.0);
  vec3 pixel_delta_u = u_viewport / WIDTH_IMAGE;
  vec3 pixel_delta_v = v_viewport / HEIGHT_IMAGE;
  point3 viewport_upper_left = camera_center - vec3(0.0, 0.0, FOCAL_LENGTH) -
                               u_viewport / 2 - v_viewport / 2;
  point3 viewport_00_pixel_position =
      viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

  // world objects
  hittable_list world_objects;
  world_objects.add(std::make_shared<sphere>(point3(0, 0, -1), 0.5));
  world_objects.add(std::make_shared<sphere>(point3(0.0, -100.5, -1), 100));

  // render
  std::cout << "P3\n" << WIDTH_IMAGE << " " << HEIGHT_IMAGE << "\n255\n";

  for (int y = 0; y < HEIGHT_IMAGE; y++) {
    std::clog << "\rScanlines remaining: " << HEIGHT_IMAGE - y << std::flush;
    for (int x = 0; x < WIDTH_IMAGE; x++) {
      point3 pixel_center =
          viewport_00_pixel_position + x * pixel_delta_u + y * pixel_delta_v;
      vec3 ray_direction = pixel_center - camera_center;
      Ray ray(camera_center, ray_direction);

      color3 pixel_color = ray_color(ray, world_objects);
      write_color(std::cout, pixel_color);
    }
  }
  std::clog << "\rDone  \n";
}