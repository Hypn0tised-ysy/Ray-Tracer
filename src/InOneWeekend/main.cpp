#include "camera.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <memory>

#include "color.h"
#include "common.h"
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
  hittable_list world_object;

  double R = std::cos(PI / 4);

  auto material_left = make_shared<lambertian>(color3(0, 0, 1));
  auto material_right = make_shared<lambertian>(color3(1, 0, 0));

  auto left_sphere = make_shared<sphere>(point3(-R, 0, -1), R, material_left);
  auto right_sphere = make_shared<sphere>(point3(R, 0, -1), R, material_right);

  world_object.add(left_sphere);
  world_object.add(right_sphere);

  return std::move(world_object);
}

Camera initialize_camera() {
  Camera camera;

  camera.aspect_ratio = 16.0 / 9.0;
  camera.vFov = 90.0;
  camera.image_width = 400;
  camera.sample_per_pixel = 100;
  camera.max_depth = 50;

  return std::move(camera);
}