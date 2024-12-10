#ifndef CAMERA_H
#define CAMERA_H

#include "color.h"
#include "common.h"
#include "hittable.h"
#include "material.h"
#include "ray.h"
#include "vec3.h"
#include <cmath>

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
class Camera {
public:
  double aspect_ratio = 1.0;
  double vFov = 90.0;
  int image_width = 100;

  point3 lookfrom = point3(0, 0, 0);
  point3 lookat = point3(0, 0, -1);
  vec3 up = vec3(0, 1, 0);

  int sample_per_pixel = 10;
  int max_depth = 10;

  double focus_distance = 10;
  double defocus_angle = 0;

  void render(hittable const &world_objects) {
    initialize();
    std::cout << "P3\n" << image_width << " " << image_height << "\n255\n";

    for (int y = 0; y < image_height; y++) {
      std::clog << "\rScanlines remaining: " << image_height - y << "    "
                << std::flush;
      for (int x = 0; x < image_width; x++) {
        color3 pixel_color(0, 0, 0);
        for (int ith_sample_ray = 0; ith_sample_ray < sample_per_pixel;
             ith_sample_ray++) {
          Ray sampleRay = getSampleRay(x, y);
          color3 sample_pixel_color =
              ray_color(sampleRay, max_depth, world_objects);
          pixel_color += sample_pixel_color;
        }

        pixel_color *= sample_scale;
        write_color(std::cout, pixel_color);
      }
    }

    std::clog << "\rDone                              \n";
  }

private:
  int image_height;
  point3 center;
  point3 viewport_00_pixel_position;
  vec3 pixel_delta_u, pixel_delta_v;
  vec3 defocus_disk_u, defocus_disk_v;
  vec3 u, v, w; // w指向观测方向的反方向（右手系），u指向相机右侧，v指向相机上侧
  double sample_scale;

  void initialize() {
    sample_scale = 1.0 / sample_per_pixel;

    // image
    image_height = int(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;

    // camera
    center = lookfrom;
    double theta = degrees_to_radians(vFov);
    double height = focus_distance * std::tan(theta / 2);
    double viewport_height = 2 * height;
    double viewport_width =
        viewport_height * (double(image_width) / image_height);

    w = unit_vector(lookfrom - lookat);
    u = unit_vector(crossProduct(up, w));
    v = unit_vector(crossProduct(w, u));

    double defocus_disk_radius =
        focus_distance * std::tan(degrees_to_radians(defocus_angle / 2));
    defocus_disk_u = u * defocus_disk_radius;
    defocus_disk_v = v * defocus_disk_radius;

    // viewport
    vec3 u_viewport = viewport_width * u;
    vec3 v_viewport = -(viewport_height * v);
    pixel_delta_u = u_viewport / image_width;
    pixel_delta_v = v_viewport / image_height;
    point3 viewport_upper_left =
        center - (focus_distance * w) - u_viewport / 2 - v_viewport / 2;
    viewport_00_pixel_position =
        viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
  }

  color3 ray_color(Ray const &ray, int depth, hittable const &world_objects) {
    if (depth <= 0)
      return color3(0, 0, 0);
    hit_record record;
    if (world_objects.hit(ray, interval(0.001, Infinity_double), record)) {
      return scattered_color(ray, depth, record, world_objects);
    } else
      return background_color(ray);
  }

  color3 scattered_color(Ray const &ray, int depth, hit_record &record,
                         hittable const &world_objects) {
    color3 attenuation;
    Ray scattered_ray;
    color3 result(0, 0, 0);
    if (record.material->Scatter(ray, record, attenuation, scattered_ray)) {
      color3 scattered_color =
          ray_color(scattered_ray, depth - 1, world_objects);
      result = cwiseProduct(attenuation, scattered_color);
    }
    return result;
  }

  Ray getSampleRay(int x, int y) const {
    vec3 offset = sample_square();
    point3 sample_pixel_center = viewport_00_pixel_position +
                                 (x + offset.x) * pixel_delta_u +
                                 (y + offset.y) * pixel_delta_v;
    vec3 ray_origin = defocus_angle <= 0 ? center : sample_defocusDisk();
    vec3 ray_direction = sample_pixel_center - ray_origin;
    Ray sample_ray(ray_origin, ray_direction);
    return sample_ray;
  }

  vec3 sample_square() const {
    vec3 offset(random_double(-0.5, 0.5), random_double(-0.5, 0.5), 0.0);
    return offset;
  }

  point3 sample_defocusDisk() const {

    vec3 random_distribution_onUnitDisk =
        vec3::generate_random_vector_onUnitDisk();
    point3 random_point_onDefocusDisk =
        center + defocus_disk_u * random_distribution_onUnitDisk[0] +
        defocus_disk_v * random_distribution_onUnitDisk[1];
    return random_point_onDefocusDisk;
  }
};

#endif // CAMERA_H