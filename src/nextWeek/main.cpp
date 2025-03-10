#include "bvh.h"
#include "camera.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>

#include "color.h"
#include "common.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "nextWeek/quad.h"
#include "sphere.h"
#include "texture.h"
#include "vec3.h"

void bouncing_spheres();
void checker_spheres();
void earth();

void parse_arguments(int argc, char **argv);
void command_prompt_hint();
bool isValidArgument(std::string const &str, int &ith_scene);

void render_scene(int ith_scene);

hittable_list initialize_world_object_forBouncingSpheres();
hittable_list initialize_world_object_forCheckerSpheres();
hittable_list initialize_world_object_forEarth();
hittable_list initialize_world_object_forQuadScene();

void generate_random_world_objects(hittable_list &world_objects);
Camera initialize_camera_forBouncingSpheres();
Camera initialize_camera_forCheckerSpheres();
Camera initialize_camera_forEarth();
Camera initialize_camera_forQuadScene();

int main(int argc, char **argv) {
  try {
    parse_arguments(argc, argv);
  } catch (std::invalid_argument const &err) {
    std::cerr << err.what() << std::endl;
    command_prompt_hint();
  }

  return 0;
}

hittable_list initialize_world_object_forBouncingSpheres() { // material
  // texture
  auto checker = make_shared<checker_texture>(0.32, color3(0.2, 0.3, 0.1),
                                              color3(0.9, 0.9, 0.9));

  // material
  auto material_ground = make_shared<lambertian>(checker);
  auto dielectric_material = make_shared<dielectric>(1.5);
  auto lambertian_material = make_shared<lambertian>(checker);
  auto metal_material = make_shared<metal>(color3(0.7, 0.6, 0.5), 0.0);

  // object
  auto ground =
      std::make_shared<sphere>(point3(0, -1000, -1), 1000, material_ground);
  auto center_sphere =
      make_shared<sphere>(point3(0, 1, 0), 1.0, dielectric_material);
  auto left_sphere =
      make_shared<sphere>(point3(-4.0, 1, 0), 1.0, lambertian_material);
  auto right_sphere =
      make_shared<sphere>(point3(4.0, 1, 0), 1.0, metal_material);

  //
  hittable_list world_objects;
  world_objects.add(ground);
  world_objects.add(center_sphere);
  world_objects.add(left_sphere);
  world_objects.add(right_sphere);
  generate_random_world_objects(world_objects);
  world_objects = hittable_list(make_shared<bvh_node>(world_objects));
  ;
  return world_objects;
}

hittable_list initialize_world_object_forCheckerSpheres() {
  hittable_list world_objects;

  auto checker = make_shared<checker_texture>(0.32, color3(0.2, 0.3, 0.1),
                                              color3(0.9, 0.9, 0.9));

  auto lambertian_material = make_shared<lambertian>(checker);

  auto sphere_upper =
      make_shared<sphere>(point3(0, 10, 0), 10.0, lambertian_material);
  auto sphere_lower =
      make_shared<sphere>(point3(0, -10, 0), 10.0, lambertian_material);

  world_objects.add(sphere_upper);
  world_objects.add(sphere_lower);

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

hittable_list initialize_world_object_forEarth() {
  auto earth_texture = make_shared<image_texture>("earthmap.jpg");
  auto earth_material = make_shared<lambertian>(earth_texture);
  auto globe = make_shared<sphere>(point3(0, 0, 0), 2, earth_material);

  hittable_list world_objects;
  world_objects.add(globe);
  return world_objects;
}
hittable_list initialize_world_object_forPerlinSpheres() {
  auto perlin_texture = make_shared<perlin_noise_texture>(4.0);
  auto ground = make_shared<sphere>(point3(0, -1000, 0), 1000,
                                    make_shared<lambertian>(perlin_texture));
  auto center_sphere = make_shared<sphere>(
      point3(0, 2, 0), 2, make_shared<lambertian>(perlin_texture));

  hittable_list world_objects;
  world_objects.add(ground);
  world_objects.add(center_sphere);
  return world_objects;
}
hittable_list initialize_world_object_forQuadScene() {
  hittable_list world;

  // Materials
  auto left_red = make_shared<lambertian>(color3(1.0, 0.2, 0.2));
  auto back_green = make_shared<lambertian>(color3(0.2, 1.0, 0.2));
  auto right_blue = make_shared<lambertian>(color3(0.2, 0.2, 1.0));
  auto upper_orange = make_shared<lambertian>(color3(1.0, 0.5, 0.0));
  auto lower_teal = make_shared<lambertian>(color3(0.2, 0.8, 0.8));

  // Quads
  world.add(make_shared<quad>(point3(-3, -2, 5), vec3(0, 0, -4), vec3(0, 4, 0),
                              left_red));
  world.add(make_shared<quad>(point3(-2, -2, 0), vec3(4, 0, 0), vec3(0, 4, 0),
                              back_green));
  world.add(make_shared<quad>(point3(3, -2, 1), vec3(0, 0, 4), vec3(0, 4, 0),
                              right_blue));
  world.add(make_shared<quad>(point3(-2, 3, 1), vec3(4, 0, 0), vec3(0, 0, 4),
                              upper_orange));
  world.add(make_shared<quad>(point3(-2, -3, 5), vec3(4, 0, 0), vec3(0, 0, -4),
                              lower_teal));

  return world;
}

Camera initialize_camera_forBouncingSpheres() {
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

  camera.background = color3(0.70, 0.80, 1.0);

  return std::move(camera);
}

Camera initialize_camera_forCheckerSpheres() {
  Camera camera;

  camera.aspect_ratio = 16.0 / 9.0;
  camera.vFov = 20;
  camera.image_width = 400;
  camera.sample_per_pixel = 100;
  camera.max_depth = 50;

  camera.lookfrom = point3(13, 2, 3);
  camera.lookat = point3(0, 0, 0);
  camera.up = vec3(0, 1, 0);

  camera.defocus_angle = 0.0;

  camera.background = color3(0.70, 0.80, 1.0);

  return std::move(camera);
}

Camera initialize_camera_forEarth() {
  Camera camera;

  camera.aspect_ratio = 16.0 / 9.0;
  camera.vFov = 20;
  camera.image_width = 400;
  camera.sample_per_pixel = 100;
  camera.max_depth = 50;

  camera.lookfrom = point3(0, 0, 12);
  camera.lookat = point3(0, 0, 0);
  camera.up = vec3(0, 1, 0);

  camera.defocus_angle = 0.0;

  camera.background = color3(0.70, 0.80, 1.0);

  return std::move(camera);
}
Camera initialize_camera_forPerlinSpheres() {
  Camera camera;

  camera.aspect_ratio = 16.0 / 9.0;
  camera.vFov = 20;
  camera.image_width = 400;
  camera.sample_per_pixel = 100;
  camera.max_depth = 50;

  camera.lookfrom = point3(13, 2, 3);
  camera.lookat = point3(0, 0, 0);
  camera.up = vec3(0, 1, 0);

  camera.defocus_angle = 0.0;

  camera.background = color3(0.70, 0.80, 1.0);

  return std::move(camera);
}

Camera initialize_camera_forQuadScene() {
  Camera camera;

  camera.aspect_ratio = 1.0;
  camera.image_width = 400;
  camera.sample_per_pixel = 100;
  camera.max_depth = 50;

  camera.vFov = 80;
  camera.lookfrom = point3(0, 0, 9);
  camera.lookat = point3(0, 0, 0);
  camera.up = vec3(0, 1, 0);

  camera.defocus_angle = 0;

  camera.background = color3(0.70, 0.80, 1.0);

  return std::move(camera);
}

void bouncing_spheres() {
  hittable_list world_objects = initialize_world_object_forBouncingSpheres();
  Camera camera = initialize_camera_forBouncingSpheres();
  camera.render(world_objects);
}

void checker_spheres() {
  hittable_list world_objects = initialize_world_object_forCheckerSpheres();
  Camera camera = initialize_camera_forCheckerSpheres();
  camera.render(world_objects);
}

void earth() {
  hittable_list world_objects = initialize_world_object_forEarth();
  Camera camera = initialize_camera_forEarth();
  camera.render(world_objects);
}
void perlin_spheres() {
  hittable_list world_objects = initialize_world_object_forPerlinSpheres();
  Camera camera = initialize_camera_forPerlinSpheres();
  camera.render(world_objects);
}
void quad_scene() {
  hittable_list world_objects = initialize_world_object_forQuadScene();
  Camera camera = initialize_camera_forQuadScene();
  camera.render(world_objects);
}
void simple_light_scene() {
  hittable_list world;

  auto pertext = make_shared<perlin_noise_texture>(4);
  world.add(make_shared<sphere>(point3(0, -1000, 0), 1000,
                                make_shared<lambertian>(pertext)));
  world.add(make_shared<sphere>(point3(0, 2, 0), 2,
                                make_shared<lambertian>(pertext)));

  auto difflight = make_shared<diffuse_light>(color3(4, 4, 4));
  world.add(make_shared<quad>(point3(3, 1, -2), vec3(2, 0, 0), vec3(0, 2, 0),
                              difflight));
  world.add(make_shared<sphere>(point3(0, 7, 0), 2, difflight));

  Camera camera;

  camera.aspect_ratio = 16.0 / 9.0;
  camera.image_width = 400;
  camera.sample_per_pixel = 100;
  camera.max_depth = 50;
  camera.background = color3(0, 0, 0);

  camera.vFov = 20;
  camera.lookfrom = point3(26, 3, 6);
  camera.lookat = point3(0, 2, 0);
  camera.up = vec3(0, 1, 0);

  camera.defocus_angle = 0;

  camera.render(world);
}
void cornell_box() {
  hittable_list world;

  auto red = make_shared<lambertian>(color3(.65, .05, .05));
  auto white = make_shared<lambertian>(color3(.73, .73, .73));
  auto green = make_shared<lambertian>(color3(.12, .45, .15));
  auto light = make_shared<diffuse_light>(color3(15, 15, 15));

  world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0),
                              vec3(0, 0, 555), green));
  world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555),
                              red));
  world.add(make_shared<quad>(point3(343, 554, 332), vec3(-130, 0, 0),
                              vec3(0, 0, -105), light));
  world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555),
                              white));
  world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0),
                              vec3(0, 0, -555), white));
  world.add(make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0),
                              vec3(0, 555, 0), white));

  shared_ptr<hittable> box1 =
      box(point3(0, 0, 0), point3(165, 330, 165), white);
  box1 = make_shared<rotate_y>(box1, 15);
  box1 = make_shared<translate>(box1, vec3(265, 0, 295));
  world.add(box1);

  shared_ptr<hittable> box2 =
      box(point3(0, 0, 0), point3(165, 165, 165), white);
  box2 = make_shared<rotate_y>(box2, -18);
  box2 = make_shared<translate>(box2, vec3(130, 0, 65));
  world.add(box2);

  Camera camera;

  camera.aspect_ratio = 1.0;
  camera.image_width = 600;
  camera.sample_per_pixel = 200;
  camera.max_depth = 50;
  camera.background = color3(0, 0, 0);

  camera.vFov = 40;
  camera.lookfrom = point3(278, 278, -800);
  camera.lookat = point3(278, 278, 0);
  camera.up = vec3(0, 1, 0);

  camera.defocus_angle = 0;

  camera.render(world);
}

void command_prompt_hint() {
  std::cerr << "argument 1: render bouncing spheres scene" << std::endl;
  std::cerr << "argument 2: render checker_spheres scene" << std::endl;
  std::cerr << "argument 3: render earth scene" << std::endl;
  std::cerr << "argument 4: render perlin_noise scene" << std::endl;
  std::cerr << "argument 5: render quad scene" << std::endl;
  std::cerr << "argument 6: render  scene" << std::endl;
  std::cerr << "argument 7: render  scene" << std::endl;
  std::cerr << "argument 8: render  scene" << std::endl;
  std::cerr << "argument 9: render  scene" << std::endl;
}

void parse_arguments(int argc, char **argv) {
  if (argc != 2) {
    throw std::invalid_argument(
        "invalid argument, program expects 1 argument except program name");
  }

  int ith_scene;
  if (!isValidArgument(argv[1], ith_scene)) {
    throw std::invalid_argument(
        "invalid argument, expect argument to be {1-9}");
  }

  render_scene(ith_scene);
}

bool isValidArgument(std::string const &str, int &ith_scene) {
  for (char c : str) {
    if (!std::isdigit(c))
      return false;
  }

  ith_scene = std::stoi(str);
  bool valid;
  switch (ith_scene) {
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
  case 7:
  case 8:
  case 9:
    valid = true;
    break;
  default:
    valid = false;
    break;
  }
  return valid;
}

void render_scene(int ith_scene) {
  switch (ith_scene) {
  case 1:
    bouncing_spheres();
    break;
  case 2:
    checker_spheres();
    break;
  case 3:
    earth();
    break;
  case 4:
    perlin_spheres();
    break;
  case 5:
    quad_scene();
    break;
  case 6:
    simple_light_scene();
    break;
  case 7:
    cornell_box();
    break;
  case 8:
    break;
  case 9:
    break;
  default:
    checker_spheres();
    break;
  }
}
