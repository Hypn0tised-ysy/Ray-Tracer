#ifndef CAMERA_H
#define CAMERA_H

#include "color.h"
#include "common.h"
#include "hittable.h"
#include "ray.h"
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

color3 normal_color(Ray const &ray, hit_record &record) {
  return 0.5 * (record.normalAgainstRay + color3(1.0, 1.0, 1.0));
}

class Camera {
public:
  double aspect_ratio = 1.0;
  int image_width = 100;
  int sample_per_pixel = 10;

  void render(hittable const &world_objects) {
    initialize();
    std::cout << "P3\n" << image_width << " " << image_height << "\n255\n";

    for (int y = 0; y < image_height; y++) {
      std::clog << "\rScanlines remaining: " << image_height - y << std::flush;
      for (int x = 0; x < image_width; x++) {
        color3 pixel_color(0, 0, 0);
        for (int ith_sample_ray = 0; ith_sample_ray < sample_per_pixel;
             ith_sample_ray++) {
          Ray sampleRay = getSampleRay(x, y);
          color3 sample_pixel_color = ray_color(sampleRay, world_objects);
          pixel_color += sample_pixel_color;
        }

        pixel_color *= sample_scale;
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
  double sample_scale;

  void initialize() {
    sample_scale = 1.0 / sample_per_pixel;

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

  Ray getSampleRay(int x, int y) const {
    vec3 offset = sample_square();
    point3 sample_pixel_center = viewport_00_pixel_position +
                                 (x + offset.x) * pixel_delta_u +
                                 (y + offset.y) * pixel_delta_v;
    vec3 ray_direction = sample_pixel_center - center;
    Ray sample_ray(center, ray_direction);
    return sample_ray;
  }

  vec3 sample_square() const {
    vec3 offset(random_double(-0.5, 0.5), random_double(-0.5, 0.5), 0.0);
    return offset;
  }
};

#endif // CAMERA_H