#include "camera.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <memory>

#include "color.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"
#include "vec3.h"

hittable_list initialize_world_object();
Camera initialize_camera();

int main() {
  hittable_list world_objects = initialize_world_object();
  Camera camera = initialize_camera();

  camera.render(world_objects);

  return 0;
}

hittable_list initialize_world_object() { // material
  auto material_ground = std::make_shared<lambertian>(color3(0.8, 0.8, 0.0));
  auto material_center_sphere = make_shared<lambertian>(color3(0.1, 0.2, 0.5));
  auto material_left_sphere = make_shared<dielectric>(1.00 / 1.33);
  auto material_right_sphere = make_shared<metal>(color3(0.8, 0.6, 0.2), 1.0);

  // world objects
  hittable_list world_objects;
  auto ground =
      std::make_shared<sphere>(point3(0, -100.5, -1), 100, material_ground);
  auto center_sphere =
      std::make_shared<sphere>(point3(0, 0, -1.2), 0.5, material_center_sphere);
  auto left_sphere =
      std::make_shared<sphere>(point3(-1.0, 0, -1), 0.5, material_left_sphere);
  auto right_sphere =
      std::make_shared<sphere>(point3(1.0, 0, -1), 0.5, material_right_sphere);

  world_objects.add(ground);
  world_objects.add(center_sphere);
  world_objects.add(left_sphere);
  world_objects.add(right_sphere);

  return std::move(world_objects);
}

Camera initialize_camera() {
  Camera camera;

  camera.aspect_ratio = 16.0 / 9.0;
  camera.image_width = 400;
  camera.sample_per_pixel = 100;
  camera.max_depth = 50;

  return std::move(camera);
}