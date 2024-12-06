#include "camera.h"

#include <cassert>
#include <cmath>
#include <memory>

#include "hittable_list.h"
#include "sphere.h"
#include "vec3.h"

int main() {

  // world objects
  hittable_list world_objects;
  world_objects.add(std::make_shared<sphere>(point3(0, 0, -1), 0.5));
  world_objects.add(std::make_shared<sphere>(point3(0.0, -100.5, -1), 100));

  // render
  Camera camera;
  camera.aspect_ratio = 16.0 / 9.0;
  camera.image_width = 400;
  camera.sample_per_pixel = 100;
  camera.max_depth = 50;
  camera.render(world_objects);

  return 0;
}