#ifndef CAMERA_H
#define CAMERA_H

#include "common.h"
#include "hittable.h"

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

color3 normal_color(Ray const &ray, hit_record &record) {
  return 0.5 * (record.normalAgainstRay + color3(1.0, 1.0, 1.0));
}

class Camera {
public:
  double aspect_ratio = 1.0;
  int image_width = 100;

  void render(hittable const &world_objects) {
    initialize();
    std::cout << "P3\n" << image_width << " " << image_height << "\n255\n";

    for (int y = 0; y < image_height; y++) {
      std::clog << "\rScanlines remaining: " << image_height - y << std::flush;
      for (int x = 0; x < image_width; x++) {
        point3 pixel_center =
            viewport_00_pixel_position + x * pixel_delta_u + y * pixel_delta_v;
        vec3 ray_direction = pixel_center - center;
        Ray ray(center, ray_direction);

        color3 pixel_color = ray_color(ray, world_objects);
        write_color(std::cout, pixel_color);
      }
    }
    std::clog << "\rDone  \n";
  }

private:
  int image_height;
  point3 center;
  point3 viewport_00_pixel_position;
  vec3 pixel_delta_u, pixel_delta_v;

  void initialize() {
    // image
    image_height = int(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;

    // camera
    double focal_length = 1.0;
    double viewport_height = 2.0;
    double viewport_width =
        viewport_height * (double(image_width) / image_height);
    center = point3(0, 0, 0);

    // viewport
    vec3 u_viewport(viewport_width, 0.0, 0.0);
    vec3 v_viewport(0.0, -viewport_height, 0.0);
    pixel_delta_u = u_viewport / image_width;
    pixel_delta_v = v_viewport / image_height;
    point3 viewport_upper_left =
        center - vec3(0.0, 0.0, focal_length) - u_viewport / 2 - v_viewport / 2;
    viewport_00_pixel_position =
        viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
  }

  color3 ray_color(Ray const &ray, hittable const &world_objects) {
    hit_record record;
    if (world_objects.hit(ray, interval(0.0, Infinity_double), record)) {
      return normal_color(ray, record);
    } else
      return background_color(ray);
  }
};

#endif // CAMERA_H