#include "bvh.h"
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
void generate_random_world_objects(hittable_list &world_objects);
Camera initialize_camera();

int main() {
  hittable_list world_objects = initialize_world_object();
  Camera camera = initialize_camera();

  camera.render(world_objects);

  return 0;
}

hittable_list initialize_world_object() { // material
  auto material_ground = std::make_shared<lambertian>(color3(0.5, 0.5, 0.5));
  auto dielectric_material = make_shared<dielectric>(1.5);
  auto lambertian_material = make_shared<lambertian>(color3(0.4, 0.2, 0.1));
  auto metal_material = make_shared<metal>(color3(0.7, 0.6, 0.5), 0.0);

  auto ground =
      std::make_shared<sphere>(point3(0, -1000, -1), 1000, material_ground);
  auto center_sphere =
      make_shared<sphere>(point3(0, 1, 0), 1.0, dielectric_material);
  auto left_sphere =
      make_shared<sphere>(point3(-4.0, 1, 0), 1.0, lambertian_material);
  auto right_sphere =
      make_shared<sphere>(point3(4.0, 1, 0), 1.0, metal_material);

  hittable_list world_objects;
  world_objects.add(ground);
  world_objects.add(center_sphere);
  world_objects.add(left_sphere);
  world_objects.add(right_sphere);
  generate_random_world_objects(world_objects);
  world_objects = hittable_list(make_shared<bvh_node>(world_objects));

  return world_objects;
}

void generate_random_world_objects(hittable_list &world_objects) {
  double radius = 0.2;
  color3 albedo;

  for (int x = -11; x < 11; x++) {
    for (int z = -11; z < 11; z++) {
      albedo = cwiseProduct(color3::generate_random_vector(),
                            color3::generate_random_vector());
      double choose_material = random_double();
      point3 sphere_origin =
          point3(x + 0.9 * random_double(), 0.2, z + 0.9 * random_double());
      std::shared_ptr<Material> sphere_material;
      std::shared_ptr<sphere> random_sphere;
      point3 sphere_destination = sphere_origin;
      if ((sphere_origin - vec3(4, 0.2, 0)).norm() > 0.9) {
        if (choose_material < 0.8) {
          sphere_destination += vec3(0, random_double(0.0, 0.5), 0);
          sphere_material = make_shared<lambertian>(albedo);
        } else if (choose_material < 0.95) {
          double fuzz = random_double(0, 0.5);
          sphere_material = make_shared<metal>(albedo, fuzz);
        } else {
          sphere_material = make_shared<dielectric>(1.5);
        }
        random_sphere = make_shared<sphere>(sphere_origin, sphere_destination,
                                            radius, sphere_material);
        world_objects.add(random_sphere);
      }
    }
  }
}

Camera initialize_camera() {
  Camera camera;

  camera.aspect_ratio = 16.0 / 9.0;
  camera.vFov = 20;
  camera.image_width = 400;
  camera.sample_per_pixel = 100;
  camera.max_depth = 50;

  camera.lookfrom = point3(13, 2, 3);
  camera.lookat = point3(0, 0, 0);
  camera.up = vec3(0, 1, 0);

  camera.defocus_angle = 0.6;
  camera.focus_distance = 10.0;

  return std::move(camera);
}
