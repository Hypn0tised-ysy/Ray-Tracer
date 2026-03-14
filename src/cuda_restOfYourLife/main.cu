#include "bvh.h"
#include "camera.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>

#include "color.h"
#include "common.h"
#include "constant_medium.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "quad.h"
#include "sphere.h"
#include "texture.h"
#include "vec3.h"

#define CUDA_CHECK(call)                                                       \
  do {                                                                         \
    cudaError_t err = call;                                                    \
    if (err != cudaSuccess) {                                                  \
      std::cerr << "CUDA ERROR " << cudaGetErrorString(err) << " (file " << __FILE__ << ", line " << __LINE__ << ")\n";    \
      exit(1);\
}\
}\
while (0)

void cornell_box();

int print_GPU_info()
{
    // 查询GPU信息
    int device_count;
    CUDA_CHECK(cudaGetDeviceCount(&device_count));
    if (device_count == 0) {
        std::cerr << "错误: 未找到CUDA设备!\n";
        return 1;
    }
    return 0;
}

void print_GPU_property()
{
    cudaDeviceProp prop;
    CUDA_CHECK(cudaGetDeviceProperties(&prop, 0));
    std::clog << "使用GPU: " << prop.name << "\n";
    std::clog << "计算能力: " << prop.major << "." << prop.minor << "\n";
    std::clog << "全局内存: " << (prop.totalGlobalMem / 1e9) << " GB\n";
    std::clog << "最大线程/block: " << prop.maxThreadsPerBlock << "\n";
    std::clog << "\n";
}

int main(int argc, char **argv) {
  if (print_GPU_info() != 0) {
    return 1;
  }
  print_GPU_property();

  cornell_box();
  return 0;
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
      box(point3(0.0, 0.0, 0.0), point3(165, 330, 165), white);
  box1 = make_shared<rotate_y>(box1, 15);
  box1 = make_shared<translate>(box1, vec3(265, 0, 295));
  world.add(box1);

  auto glass = make_shared<dielectric>(1.5);
  world.add(make_shared<sphere>(point3(190, 90, 190), 90, glass));

  // light sources
  auto empty_material = shared_ptr<Material>();
  hittable_list lights;
  lights.add(make_shared<quad>(point3(343, 554, 332), vec3(-130, 0.0, 0.0),
                               vec3(0.0, 0.0, -105), empty_material));

  Camera camera;

  camera.aspect_ratio = 1.0;
  camera.image_width = 600;
  camera.sample_per_pixel = 1000;
  camera.max_depth = 50;
  camera.background = color3(0, 0, 0);

  camera.vFov = 40;
  camera.lookfrom = point3(278, 278, -800);
  camera.lookat = point3(278, 278, 0);
  camera.up = vec3(0, 1, 0);

  camera.defocus_angle = 0;

  camera.render(world, lights);
}
