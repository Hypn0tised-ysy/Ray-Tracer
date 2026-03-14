#ifndef CUDA_RENDERER_BRIDGE_CUH
#define CUDA_RENDERER_BRIDGE_CUH

#include <vector>

#include "color.h"
#include "hittable.h"

namespace cuda_backend {

bool render_with_cuda(double aspect_ratio, int image_width, int image_height,
                      int samples_per_pixel, int max_depth,
                      color3 const &background, point3 const &lookfrom,
                      point3 const &lookat, vec3 const &up, double vFov,
                      double focus_distance, double defocus_angle,
                      hittable const &world_objects, hittable const &lights,
                      std::vector<color3> &out_pixels);

} // namespace cuda_backend

#ifdef __CUDACC__

#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include <curand_kernel.h>
#include <cuda_runtime.h>

#include "hittable_list.h"
#include "material.h"
#include "quad.h"
#include "sphere.h"
#include "texture.h"

namespace cuda_backend {
namespace detail {

constexpr double kPi = 3.1415926535897932385;
constexpr double kInfinity = 1e308;
constexpr int kThreadsX = 16;
constexpr int kThreadsY = 16;
constexpr int kTextureResolveMaxDepth = 8;

enum DeviceTextureType : int { TEX_SOLID = 0, TEX_CHECKER = 1 };
enum DeviceMaterialType : int {
  MAT_LAMBERTIAN = 0,
  MAT_METAL = 1,
  MAT_DIELECTRIC = 2,
  MAT_DIFFUSE_LIGHT = 3,
  MAT_ISOTROPIC = 4,
};
enum DevicePrimitiveType : int { PRIM_SPHERE = 0, PRIM_QUAD = 1 };
enum DevicePdfType : int { PDF_NONE = 0, PDF_COSINE = 1, PDF_SPHERE = 2 };

struct DeviceVec3 {
  double x;
  double y;
  double z;

  __host__ __device__ DeviceVec3() : x(0.0), y(0.0), z(0.0) {}
  __host__ __device__ DeviceVec3(double x_, double y_, double z_)
      : x(x_), y(y_), z(z_) {}
};

struct DeviceRay {
  DeviceVec3 origin;
  DeviceVec3 direction;
  double time;

  __host__ __device__ DeviceRay() : origin(), direction(), time(0.0) {}
  __host__ __device__ DeviceRay(DeviceVec3 const &o, DeviceVec3 const &d,
                                double t)
      : origin(o), direction(d), time(t) {}

  __host__ __device__ DeviceVec3 at(double t) const;
};

struct DeviceHitRecord {
  DeviceVec3 point;
  DeviceVec3 normal;
  double t;
  double u;
  double v;
  int front_face;
  int material_id;
};

struct DeviceTexture {
  int type;
  DeviceVec3 albedo;
  int even_texture_id;
  int odd_texture_id;
  double scale;
};

struct DeviceMaterial {
  int type;
  int texture_id;
  DeviceVec3 albedo;
  double fuzziness;
  double refraction_index;
};

struct DevicePrimitive {
  int type;
  int material_id;

  DeviceVec3 p0;
  DeviceVec3 p1;
  DeviceVec3 p2;

  double radius;
  int moving;

  DeviceVec3 normal;
  DeviceVec3 w;
  double d;
  double area;
};

struct DeviceLightPrimitive {
  int type;

  DeviceVec3 p0;
  DeviceVec3 p1;
  DeviceVec3 p2;

  double radius;
  int moving;

  DeviceVec3 normal;
  DeviceVec3 w;
  double d;
  double area;
};

struct DeviceScatterRecord {
  DeviceVec3 attenuation;
  DeviceRay scattered;
  int skip_pdf;
  int pdf_type;
  DeviceVec3 pdf_normal;
};

struct DeviceCameraParams {
  int image_width;
  int image_height;
  int sqrt_spp;
  int max_depth;

  double sample_scale;
  double reciprocal_sqrt_spp;

  DeviceVec3 background;

  DeviceVec3 center;
  DeviceVec3 pixel00;
  DeviceVec3 pixel_delta_u;
  DeviceVec3 pixel_delta_v;

  DeviceVec3 defocus_disk_u;
  DeviceVec3 defocus_disk_v;
  int enable_defocus;
};

struct HostMat3 {
  double m[3][3];
};

struct HostTransform {
  HostMat3 rotation;
  vec3 translation;
};

inline HostMat3 make_identity_mat3() {
  HostMat3 mat{};
  mat.m[0][0] = 1.0;
  mat.m[1][1] = 1.0;
  mat.m[2][2] = 1.0;
  return mat;
}

inline HostTransform make_identity_transform() {
  HostTransform tr{};
  tr.rotation = make_identity_mat3();
  tr.translation = vec3(0.0, 0.0, 0.0);
  return tr;
}

inline HostMat3 multiply_mat3(HostMat3 const &a, HostMat3 const &b) {
  HostMat3 out{};
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      out.m[i][j] = 0.0;
      for (int k = 0; k < 3; ++k) {
        out.m[i][j] += a.m[i][k] * b.m[k][j];
      }
    }
  }
  return out;
}

inline vec3 apply_mat3(HostMat3 const &m, vec3 const &v) {
  return vec3(m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z,
              m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z,
              m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z);
}

inline HostTransform compose_transform(HostTransform const &parent,
                                       HostTransform const &local) {
  HostTransform out{};
  out.rotation = multiply_mat3(parent.rotation, local.rotation);
  out.translation = apply_mat3(parent.rotation, local.translation) +
                    parent.translation;
  return out;
}

inline HostTransform make_translate_transform(vec3 const &offset) {
  HostTransform tr = make_identity_transform();
  tr.translation = offset;
  return tr;
}

inline HostTransform make_rotate_y_transform(double cos_theta,
                                             double sin_theta) {
  HostTransform tr = make_identity_transform();
  tr.rotation.m[0][0] = cos_theta;
  tr.rotation.m[0][2] = sin_theta;
  tr.rotation.m[2][0] = -sin_theta;
  tr.rotation.m[2][2] = cos_theta;
  return tr;
}

inline point3 apply_point(HostTransform const &tr, point3 const &p) {
  vec3 rotated = apply_mat3(tr.rotation, p);
  return rotated + tr.translation;
}

inline vec3 apply_vector(HostTransform const &tr, vec3 const &v) {
  return apply_mat3(tr.rotation, v);
}

inline DeviceVec3 to_device_vec3(vec3 const &v) {
  return DeviceVec3(v.x, v.y, v.z);
}

inline vec3 to_host_vec3(DeviceVec3 const &v) { return vec3(v.x, v.y, v.z); }

class SceneConverter {
public:
  std::vector<DeviceTexture> textures;
  std::vector<DeviceMaterial> materials;
  std::vector<DevicePrimitive> primitives;
  std::vector<DeviceLightPrimitive> light_primitives;

  bool build(hittable const &world, hittable const &lights) {
    textures.clear();
    materials.clear();
    primitives.clear();
    light_primitives.clear();
    texture_ids.clear();
    material_ids.clear();
    error_message.clear();

    if (!flatten_world(world, make_identity_transform())) {
      return false;
    }
    if (!flatten_lights(lights, make_identity_transform())) {
      return false;
    }

    if (primitives.empty()) {
      error_message = "world has no GPU-compatible primitives";
      return false;
    }

    return true;
  }

  std::string const &error() const { return error_message; }

private:
  std::unordered_map<texture const *, int> texture_ids;
  std::unordered_map<Material const *, int> material_ids;
  std::string error_message;

  int register_texture(shared_ptr<texture> const &tex) {
    if (!tex) {
      error_message = "null texture is not GPU-compatible";
      return -1;
    }

    auto found = texture_ids.find(tex.get());
    if (found != texture_ids.end()) {
      return found->second;
    }

    DeviceTexture device_tex{};

    if (auto solid = dynamic_cast<solid_color const *>(tex.get())) {
      device_tex.type = TEX_SOLID;
      device_tex.albedo = to_device_vec3(solid->get_albedo());
      device_tex.even_texture_id = -1;
      device_tex.odd_texture_id = -1;
      device_tex.scale = 1.0;
    } else if (auto checker = dynamic_cast<checker_texture const *>(tex.get())) {
      int even_id =
          register_texture(std::static_pointer_cast<texture>(checker->get_even()));
      if (even_id < 0) {
        return -1;
      }

      int odd_id =
          register_texture(std::static_pointer_cast<texture>(checker->get_odd()));
      if (odd_id < 0) {
        return -1;
      }

      device_tex.type = TEX_CHECKER;
      device_tex.albedo = DeviceVec3(0.0, 0.0, 0.0);
      device_tex.even_texture_id = even_id;
      device_tex.odd_texture_id = odd_id;
      device_tex.scale = checker->get_scale();
    } else {
      error_message = "unsupported texture type for CUDA path";
      return -1;
    }

    int id = static_cast<int>(textures.size());
    textures.push_back(device_tex);
    texture_ids.emplace(tex.get(), id);
    return id;
  }

  int register_material(shared_ptr<Material> const &material) {
    if (!material) {
      error_message = "null material is not GPU-compatible";
      return -1;
    }

    auto found = material_ids.find(material.get());
    if (found != material_ids.end()) {
      return found->second;
    }

    DeviceMaterial mat{};
    mat.texture_id = -1;
    mat.albedo = DeviceVec3(0.0, 0.0, 0.0);
    mat.fuzziness = 0.0;
    mat.refraction_index = 1.0;

    if (auto lambert = dynamic_cast<lambertian const *>(material.get())) {
      int tex_id = register_texture(lambert->get_texture());
      if (tex_id < 0) {
        return -1;
      }
      mat.type = MAT_LAMBERTIAN;
      mat.texture_id = tex_id;
    } else if (auto m = dynamic_cast<metal const *>(material.get())) {
      mat.type = MAT_METAL;
      mat.albedo = to_device_vec3(m->get_albedo());
      mat.fuzziness = m->get_fuzziness();
    } else if (auto d = dynamic_cast<dielectric const *>(material.get())) {
      mat.type = MAT_DIELECTRIC;
      mat.refraction_index = d->get_refraction_index();
    } else if (auto light = dynamic_cast<diffuse_light const *>(material.get())) {
      int tex_id = register_texture(light->get_texture());
      if (tex_id < 0) {
        return -1;
      }
      mat.type = MAT_DIFFUSE_LIGHT;
      mat.texture_id = tex_id;
    } else if (auto iso = dynamic_cast<isotropic const *>(material.get())) {
      int tex_id = register_texture(iso->get_texture());
      if (tex_id < 0) {
        return -1;
      }
      mat.type = MAT_ISOTROPIC;
      mat.texture_id = tex_id;
    } else {
      error_message = "unsupported material type for CUDA path";
      return -1;
    }

    int id = static_cast<int>(materials.size());
    materials.push_back(mat);
    material_ids.emplace(material.get(), id);
    return id;
  }

  bool append_sphere_primitive(sphere const &s, HostTransform const &tr,
                               bool as_light) {
    point3 center_t0 = apply_point(tr, s.center_at(0.0));
    point3 center_t1 = apply_point(tr, s.center_at(1.0));
    vec3 delta = center_t1 - center_t0;
    int moving = delta.norm_square() > 1e-12 ? 1 : 0;

    if (as_light) {
      DeviceLightPrimitive light{};
      light.type = PRIM_SPHERE;
      light.p0 = to_device_vec3(center_t0);
      light.p1 = to_device_vec3(center_t1);
      light.p2 = DeviceVec3(0.0, 0.0, 0.0);
      light.radius = s.get_radius();
      light.moving = moving;
      light.normal = DeviceVec3(0.0, 0.0, 0.0);
      light.w = DeviceVec3(0.0, 0.0, 0.0);
      light.d = 0.0;
      light.area = 0.0;
      light_primitives.push_back(light);
      return true;
    }

    int mat_id = register_material(s.get_material());
    if (mat_id < 0) {
      return false;
    }

    DevicePrimitive prim{};
    prim.type = PRIM_SPHERE;
    prim.material_id = mat_id;
    prim.p0 = to_device_vec3(center_t0);
    prim.p1 = to_device_vec3(center_t1);
    prim.p2 = DeviceVec3(0.0, 0.0, 0.0);
    prim.radius = s.get_radius();
    prim.moving = moving;
    prim.normal = DeviceVec3(0.0, 0.0, 0.0);
    prim.w = DeviceVec3(0.0, 0.0, 0.0);
    prim.d = 0.0;
    prim.area = 0.0;
    primitives.push_back(prim);
    return true;
  }

  bool append_quad_primitive(quad const &q, HostTransform const &tr,
                             bool as_light) {
    point3 p0 = apply_point(tr, q.get_p0());
    vec3 u = apply_vector(tr, q.get_u());
    vec3 v = apply_vector(tr, q.get_v());

    vec3 n = crossProduct(u, v);
    double n_len_sq = n.norm_square();
    double area = std::sqrt(n_len_sq);
    if (area <= 1e-12) {
      error_message = "degenerate quad is not GPU-compatible";
      return false;
    }

    vec3 normal = n / area;
    vec3 w = n / n_len_sq;
    double d = dotProduct(normal, p0);

    if (as_light) {
      DeviceLightPrimitive light{};
      light.type = PRIM_QUAD;
      light.p0 = to_device_vec3(p0);
      light.p1 = to_device_vec3(u);
      light.p2 = to_device_vec3(v);
      light.radius = 0.0;
      light.moving = 0;
      light.normal = to_device_vec3(normal);
      light.w = to_device_vec3(w);
      light.d = d;
      light.area = area;
      light_primitives.push_back(light);
      return true;
    }

    int mat_id = register_material(q.get_material());
    if (mat_id < 0) {
      return false;
    }

    DevicePrimitive prim{};
    prim.type = PRIM_QUAD;
    prim.material_id = mat_id;
    prim.p0 = to_device_vec3(p0);
    prim.p1 = to_device_vec3(u);
    prim.p2 = to_device_vec3(v);
    prim.radius = 0.0;
    prim.moving = 0;
    prim.normal = to_device_vec3(normal);
    prim.w = to_device_vec3(w);
    prim.d = d;
    prim.area = area;
    primitives.push_back(prim);
    return true;
  }

  bool flatten_world(hittable const &node, HostTransform const &tr) {
    if (auto list = dynamic_cast<hittable_list const *>(&node)) {
      for (auto const &child : list->objects) {
        if (!child) {
          error_message = "null child in hittable_list";
          return false;
        }
        if (!flatten_world(*child, tr)) {
          return false;
        }
      }
      return true;
    }

    if (auto translated = dynamic_cast<translate const *>(&node)) {
      HostTransform local = make_translate_transform(translated->get_offset());
      return flatten_world(*translated->get_object(), compose_transform(tr, local));
    }

    if (auto rotated = dynamic_cast<rotate_y const *>(&node)) {
      HostTransform local =
          make_rotate_y_transform(rotated->get_cos_theta(), rotated->get_sin_theta());
      return flatten_world(*rotated->get_object(), compose_transform(tr, local));
    }

    if (auto s = dynamic_cast<sphere const *>(&node)) {
      return append_sphere_primitive(*s, tr, false);
    }

    if (auto q = dynamic_cast<quad const *>(&node)) {
      return append_quad_primitive(*q, tr, false);
    }

    error_message = "unsupported hittable type in world for CUDA path";
    return false;
  }

  bool flatten_lights(hittable const &node, HostTransform const &tr) {
    if (auto list = dynamic_cast<hittable_list const *>(&node)) {
      for (auto const &child : list->objects) {
        if (!child) {
          error_message = "null child in light list";
          return false;
        }
        if (!flatten_lights(*child, tr)) {
          return false;
        }
      }
      return true;
    }

    if (auto translated = dynamic_cast<translate const *>(&node)) {
      HostTransform local = make_translate_transform(translated->get_offset());
      return flatten_lights(*translated->get_object(), compose_transform(tr, local));
    }

    if (auto rotated = dynamic_cast<rotate_y const *>(&node)) {
      HostTransform local =
          make_rotate_y_transform(rotated->get_cos_theta(), rotated->get_sin_theta());
      return flatten_lights(*rotated->get_object(), compose_transform(tr, local));
    }

    if (auto s = dynamic_cast<sphere const *>(&node)) {
      return append_sphere_primitive(*s, tr, true);
    }

    if (auto q = dynamic_cast<quad const *>(&node)) {
      return append_quad_primitive(*q, tr, true);
    }

    error_message = "unsupported hittable type in lights for CUDA path";
    return false;
  }
};

__host__ __device__ inline DeviceVec3 make_vec3(double x, double y, double z) {
  return DeviceVec3(x, y, z);
}

__host__ __device__ inline DeviceVec3 add(DeviceVec3 const &a,
                                           DeviceVec3 const &b) {
  return DeviceVec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

__host__ __device__ inline DeviceVec3 sub(DeviceVec3 const &a,
                                           DeviceVec3 const &b) {
  return DeviceVec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

__host__ __device__ inline DeviceVec3 mul(DeviceVec3 const &a,
                                           DeviceVec3 const &b) {
  return DeviceVec3(a.x * b.x, a.y * b.y, a.z * b.z);
}

__host__ __device__ inline DeviceVec3 mul(DeviceVec3 const &a, double k) {
  return DeviceVec3(a.x * k, a.y * k, a.z * k);
}

__host__ __device__ inline DeviceVec3 div(DeviceVec3 const &a, double k) {
  return mul(a, 1.0 / k);
}

__host__ __device__ inline double dot(DeviceVec3 const &a, DeviceVec3 const &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

__host__ __device__ inline DeviceVec3 cross(DeviceVec3 const &a,
                                             DeviceVec3 const &b) {
  return DeviceVec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                    a.x * b.y - a.y * b.x);
}

__host__ __device__ inline double length_squared(DeviceVec3 const &a) {
  return dot(a, a);
}

__host__ __device__ inline double length(DeviceVec3 const &a) {
  return sqrt(length_squared(a));
}

__host__ __device__ inline DeviceVec3 unit(DeviceVec3 const &a) {
  return div(a, length(a));
}

__host__ __device__ inline int near_zero(DeviceVec3 const &a) {
  double s = 1e-12;
  return fabs(a.x) < s && fabs(a.y) < s && fabs(a.z) < s;
}

__host__ __device__ inline DeviceVec3 reflect(DeviceVec3 const &v,
                                               DeviceVec3 const &n) {
  return sub(v, mul(n, 2.0 * dot(v, n)));
}

__host__ __device__ inline DeviceVec3 refract(DeviceVec3 const &uv,
                                              DeviceVec3 const &n,
                                              double eta_ratio) {
  double cos_theta = fmin(dot(mul(uv, -1.0), n), 1.0);
  DeviceVec3 r_out_perp = mul(add(uv, mul(n, cos_theta)), eta_ratio);
  DeviceVec3 r_out_parallel = mul(n, -sqrt(fabs(1.0 - length_squared(r_out_perp))));
  return add(r_out_perp, r_out_parallel);
}

__host__ __device__ inline DeviceVec3 DeviceRay::at(double t) const {
  return add(origin, mul(direction, t));
}

__device__ inline double rand_double(curandState *state) {
  return curand_uniform_double(state);
}

__device__ inline DeviceVec3 random_in_unit_sphere(curandState *state) {
  while (true) {
    DeviceVec3 p = make_vec3(rand_double(state) * 2.0 - 1.0,
                             rand_double(state) * 2.0 - 1.0,
                             rand_double(state) * 2.0 - 1.0);
    if (length_squared(p) < 1.0) {
      return p;
    }
  }
}

__device__ inline DeviceVec3 random_unit_vector(curandState *state) {
  return unit(random_in_unit_sphere(state));
}

__device__ inline DeviceVec3 random_in_unit_disk(curandState *state) {
  while (true) {
    DeviceVec3 p = make_vec3(rand_double(state) * 2.0 - 1.0,
                             rand_double(state) * 2.0 - 1.0, 0.0);
    if (length_squared(p) < 1.0) {
      return p;
    }
  }
}

struct DeviceONB {
  DeviceVec3 u;
  DeviceVec3 v;
  DeviceVec3 w;

  __device__ explicit DeviceONB(DeviceVec3 const &n) {
    w = unit(n);
    DeviceVec3 a = fabs(w.x) > 0.9 ? make_vec3(0.0, 1.0, 0.0)
                                   : make_vec3(1.0, 0.0, 0.0);
    v = unit(cross(w, a));
    u = cross(w, v);
  }

  __device__ DeviceVec3 local(DeviceVec3 const &a) const {
    return add(add(mul(u, a.x), mul(v, a.y)), mul(w, a.z));
  }
};

__device__ inline DeviceVec3 random_cosine_direction(DeviceVec3 const &normal,
                                                      curandState *state) {
  double r1 = rand_double(state);
  double r2 = rand_double(state);
  double phi = 2.0 * kPi * r1;

  double x = cos(phi) * sqrt(r2);
  double y = sin(phi) * sqrt(r2);
  double z = sqrt(1.0 - r2);

  DeviceONB onb(normal);
  return onb.local(make_vec3(x, y, z));
}

__device__ inline DeviceVec3 random_to_sphere(double radius,
                                               double distance_squared,
                                               curandState *state) {
  double r1 = rand_double(state);
  double r2 = rand_double(state);
  double z = 1.0 + r2 * (sqrt(1.0 - radius * radius / distance_squared) - 1.0);

  double phi = 2.0 * kPi * r1;
  double x = cos(phi) * sqrt(1.0 - z * z);
  double y = sin(phi) * sqrt(1.0 - z * z);
  return make_vec3(x, y, z);
}

__device__ inline void set_face_normal(DeviceRay const &ray,
                                       DeviceVec3 const &outward,
                                       DeviceHitRecord &rec) {
  rec.front_face = dot(ray.direction, outward) < 0.0 ? 1 : 0;
  rec.normal = rec.front_face ? outward : mul(outward, -1.0);
}

__device__ inline int hit_sphere(DevicePrimitive const &primitive,
                                 DeviceRay const &ray, double t_min,
                                 double t_max, DeviceHitRecord &rec) {
  DeviceVec3 center = primitive.moving
                          ? add(primitive.p0,
                                mul(sub(primitive.p1, primitive.p0), ray.time))
                          : primitive.p0;

  DeviceVec3 center_minus_origin = sub(center, ray.origin);
  double a = dot(ray.direction, ray.direction);
  double half_b = dot(ray.direction, center_minus_origin);
  double c = dot(center_minus_origin, center_minus_origin) -
             primitive.radius * primitive.radius;
  double discriminant = half_b * half_b - a * c;
  if (discriminant < 0.0) {
    return 0;
  }

  double sqrt_d = sqrt(discriminant);
  double root = (half_b - sqrt_d) / a;
  if (root <= t_min || root >= t_max) {
    root = (half_b + sqrt_d) / a;
    if (root <= t_min || root >= t_max) {
      return 0;
    }
  }

  rec.t = root;
  rec.point = ray.at(root);
  DeviceVec3 outward_normal = div(sub(rec.point, center), primitive.radius);
  set_face_normal(ray, outward_normal, rec);
  rec.material_id = primitive.material_id;

  double theta = acos(-rec.normal.y);
  double phi = atan2(-rec.normal.z, rec.normal.x) + kPi;
  rec.u = phi / (2.0 * kPi);
  rec.v = theta / kPi;

  return 1;
}

__device__ inline int hit_quad(DevicePrimitive const &primitive,
                               DeviceRay const &ray, double t_min,
                               double t_max, DeviceHitRecord &rec) {
  double denom = dot(primitive.normal, ray.direction);
  if (fabs(denom) < 1e-8) {
    return 0;
  }

  double t = (primitive.d - dot(primitive.normal, ray.origin)) / denom;
  if (t <= t_min || t >= t_max) {
    return 0;
  }

  DeviceVec3 hit = ray.at(t);
  DeviceVec3 p0_to_hit = sub(hit, primitive.p0);
  double alpha = dot(primitive.w, cross(p0_to_hit, primitive.p2));
  double beta = dot(primitive.w, cross(primitive.p1, p0_to_hit));
  if (alpha < 0.0 || alpha > 1.0 || beta < 0.0 || beta > 1.0) {
    return 0;
  }

  rec.t = t;
  rec.point = hit;
  rec.u = alpha;
  rec.v = beta;
  rec.material_id = primitive.material_id;
  set_face_normal(ray, primitive.normal, rec);
  return 1;
}

__device__ inline int hit_world(DevicePrimitive const *primitives,
                                int primitive_count, DeviceRay const &ray,
                                double t_min, double t_max,
                                DeviceHitRecord &record) {
  DeviceHitRecord temp{};
  int hit_anything = 0;
  double closest = t_max;

  for (int i = 0; i < primitive_count; ++i) {
    int hit = 0;
    if (primitives[i].type == PRIM_SPHERE) {
      hit = hit_sphere(primitives[i], ray, t_min, closest, temp);
    } else if (primitives[i].type == PRIM_QUAD) {
      hit = hit_quad(primitives[i], ray, t_min, closest, temp);
    }

    if (hit) {
      hit_anything = 1;
      closest = temp.t;
      record = temp;
    }
  }

  return hit_anything;
}

__device__ inline int hit_light_sphere(DeviceLightPrimitive const &light,
                                       DeviceRay const &ray, double t_min,
                                       double t_max, DeviceHitRecord &rec) {
  DeviceVec3 center = light.moving
                          ? add(light.p0, mul(sub(light.p1, light.p0), ray.time))
                          : light.p0;

  DeviceVec3 center_minus_origin = sub(center, ray.origin);
  double a = dot(ray.direction, ray.direction);
  double half_b = dot(ray.direction, center_minus_origin);
  double c = dot(center_minus_origin, center_minus_origin) -
             light.radius * light.radius;
  double discriminant = half_b * half_b - a * c;
  if (discriminant < 0.0) {
    return 0;
  }

  double sqrt_d = sqrt(discriminant);
  double root = (half_b - sqrt_d) / a;
  if (root <= t_min || root >= t_max) {
    root = (half_b + sqrt_d) / a;
    if (root <= t_min || root >= t_max) {
      return 0;
    }
  }

  rec.t = root;
  rec.point = ray.at(root);
  DeviceVec3 outward_normal = div(sub(rec.point, center), light.radius);
  set_face_normal(ray, outward_normal, rec);
  return 1;
}

__device__ inline int hit_light_quad(DeviceLightPrimitive const &light,
                                     DeviceRay const &ray, double t_min,
                                     double t_max, DeviceHitRecord &rec) {
  double denom = dot(light.normal, ray.direction);
  if (fabs(denom) < 1e-8) {
    return 0;
  }

  double t = (light.d - dot(light.normal, ray.origin)) / denom;
  if (t <= t_min || t >= t_max) {
    return 0;
  }

  DeviceVec3 hit = ray.at(t);
  DeviceVec3 p0_to_hit = sub(hit, light.p0);
  double alpha = dot(light.w, cross(p0_to_hit, light.p2));
  double beta = dot(light.w, cross(light.p1, p0_to_hit));
  if (alpha < 0.0 || alpha > 1.0 || beta < 0.0 || beta > 1.0) {
    return 0;
  }

  rec.t = t;
  rec.point = hit;
  set_face_normal(ray, light.normal, rec);
  return 1;
}

__device__ inline double light_pdf_value(DeviceLightPrimitive const &light,
                                         DeviceVec3 const &origin,
                                         DeviceVec3 const &direction) {
  DeviceHitRecord rec{};
  int hit = 0;

  if (light.type == PRIM_SPHERE) {
    hit = hit_light_sphere(light, DeviceRay(origin, direction, 0.0), 0.001,
                           kInfinity, rec);
    if (!hit) {
      return 0.0;
    }

    DeviceVec3 center = light.p0;
    double distance_squared = length_squared(sub(center, origin));
    double rr = light.radius * light.radius;
    if (distance_squared <= rr + 1e-12) {
      return 1.0 / (4.0 * kPi);
    }

    double cos_theta_max = sqrt(fmax(0.0, 1.0 - rr / distance_squared));
    double solid_angle = 2.0 * kPi * (1.0 - cos_theta_max);
    return solid_angle <= 1e-12 ? 0.0 : 1.0 / solid_angle;
  }

  if (light.type == PRIM_QUAD) {
    hit = hit_light_quad(light, DeviceRay(origin, direction, 0.0), 0.001,
                         kInfinity, rec);
    if (!hit) {
      return 0.0;
    }

    double direction_len = length(direction);
    if (direction_len <= 1e-12) {
      return 0.0;
    }

    double distance_squared = rec.t * rec.t * length_squared(direction);
    double cosine = fabs(dot(rec.normal, direction) / direction_len);
    if (cosine <= 1e-12) {
      return 0.0;
    }

    return distance_squared / (cosine * light.area);
  }

  return 0.0;
}

__device__ inline DeviceVec3 random_light_direction(
    DeviceLightPrimitive const &light, DeviceVec3 const &origin,
    curandState *state) {
  if (light.type == PRIM_SPHERE) {
    DeviceVec3 center = light.p0;
    DeviceVec3 direction = sub(center, origin);
    double distance_squared = length_squared(direction);
    if (distance_squared <= light.radius * light.radius + 1e-12) {
      return random_unit_vector(state);
    }

    DeviceONB onb(direction);
    return onb.local(random_to_sphere(light.radius, distance_squared, state));
  }

  if (light.type == PRIM_QUAD) {
    double random_u = rand_double(state);
    double random_v = rand_double(state);
    DeviceVec3 random_point =
        add(light.p0, add(mul(light.p1, random_u), mul(light.p2, random_v)));
    return unit(sub(random_point, origin));
  }

  return make_vec3(1.0, 0.0, 0.0);
}

__device__ inline double lights_pdf_value(DeviceLightPrimitive const *lights,
                                          int light_count,
                                          DeviceVec3 const &origin,
                                          DeviceVec3 const &direction) {
  if (light_count <= 0) {
    return 0.0;
  }

  double weight = 1.0 / static_cast<double>(light_count);
  double sum = 0.0;
  for (int i = 0; i < light_count; ++i) {
    sum += weight * light_pdf_value(lights[i], origin, direction);
  }
  return sum;
}

__device__ inline DeviceVec3 sample_light_direction(
    DeviceLightPrimitive const *lights, int light_count,
    DeviceVec3 const &origin, curandState *state) {
  if (light_count <= 0) {
    return make_vec3(1.0, 0.0, 0.0);
  }

  int idx = static_cast<int>(rand_double(state) * light_count);
  if (idx >= light_count) {
    idx = light_count - 1;
  }
  return random_light_direction(lights[idx], origin, state);
}

__device__ inline DeviceVec3 sample_texture(int texture_id,
                                            DeviceTexture const *textures,
                                            int texture_count,
                                            DeviceVec3 const &point,
                                            double u, double v) {
  (void)u;
  (void)v;

  if (texture_id < 0 || texture_id >= texture_count) {
    return make_vec3(1.0, 0.0, 1.0);
  }

  int current = texture_id;
  for (int depth = 0; depth < kTextureResolveMaxDepth; ++depth) {
    DeviceTexture const &tex = textures[current];
    if (tex.type == TEX_SOLID) {
      return tex.albedo;
    }

    if (tex.type == TEX_CHECKER) {
      if (fabs(tex.scale) <= 1e-12) {
        return make_vec3(1.0, 0.0, 1.0);
      }

      double inv = 1.0 / tex.scale;
      int ix = static_cast<int>(floor(point.x * inv));
      int iy = static_cast<int>(floor(point.y * inv));
      int iz = static_cast<int>(floor(point.z * inv));
      int is_even = ((ix + iy + iz) % 2) == 0 ? 1 : 0;
      current = is_even ? tex.even_texture_id : tex.odd_texture_id;
      if (current < 0 || current >= texture_count) {
        return make_vec3(1.0, 0.0, 1.0);
      }
      continue;
    }

    return make_vec3(1.0, 0.0, 1.0);
  }

  return make_vec3(1.0, 0.0, 1.0);
}

__device__ inline DeviceVec3 material_emitted(DeviceMaterial const &material,
                                               DeviceTexture const *textures,
                                               int texture_count,
                                               DeviceHitRecord const &rec,
                                               DeviceRay const &ray_in) {
  (void)ray_in;
  if (material.type != MAT_DIFFUSE_LIGHT) {
    return make_vec3(0.0, 0.0, 0.0);
  }
  if (!rec.front_face) {
    return make_vec3(0.0, 0.0, 0.0);
  }
  return sample_texture(material.texture_id, textures, texture_count, rec.point,
                        rec.u, rec.v);
}

__device__ inline double schlick_reflectance(double cosine,
                                             double refraction_ratio) {
  double r0 = (1.0 - refraction_ratio) / (1.0 + refraction_ratio);
  r0 = r0 * r0;
  return r0 + (1.0 - r0) * pow(1.0 - cosine, 5.0);
}

__device__ inline int material_scatter(DeviceMaterial const &material,
                                       DeviceTexture const *textures,
                                       int texture_count, DeviceRay const &ray,
                                       DeviceHitRecord const &rec,
                                       DeviceScatterRecord &scatter_rec,
                                       curandState *state) {
  scatter_rec.attenuation = make_vec3(1.0, 1.0, 1.0);
  scatter_rec.scattered = DeviceRay(rec.point, make_vec3(0.0, 0.0, 0.0), ray.time);
  scatter_rec.skip_pdf = 0;
  scatter_rec.pdf_type = PDF_NONE;
  scatter_rec.pdf_normal = rec.normal;

  if (material.type == MAT_LAMBERTIAN) {
    scatter_rec.attenuation =
        sample_texture(material.texture_id, textures, texture_count, rec.point,
                       rec.u, rec.v);
    scatter_rec.skip_pdf = 0;
    scatter_rec.pdf_type = PDF_COSINE;
    scatter_rec.pdf_normal = rec.normal;
    return 1;
  }

  if (material.type == MAT_METAL) {
    DeviceVec3 reflected =
        unit(reflect(unit(ray.direction), rec.normal));
    reflected = unit(add(reflected,
                         mul(random_unit_vector(state), material.fuzziness)));

    scatter_rec.attenuation = material.albedo;
    scatter_rec.skip_pdf = 1;
    scatter_rec.pdf_type = PDF_NONE;
    scatter_rec.scattered = DeviceRay(rec.point, reflected, ray.time);
    return dot(reflected, rec.normal) > 0.0 ? 1 : 0;
  }

  if (material.type == MAT_DIELECTRIC) {
    scatter_rec.attenuation = make_vec3(1.0, 1.0, 1.0);
    scatter_rec.skip_pdf = 1;
    scatter_rec.pdf_type = PDF_NONE;

    double eta_incident_over_eta_refract =
        rec.front_face ? (1.0 / material.refraction_index)
                       : material.refraction_index;

    DeviceVec3 unit_direction = unit(ray.direction);
    double cos_theta = fmin(dot(mul(unit_direction, -1.0), rec.normal), 1.0);
    double sin_theta = sqrt(fmax(0.0, 1.0 - cos_theta * cos_theta));

    int cannot_refract = eta_incident_over_eta_refract * sin_theta > 1.0;
    DeviceVec3 direction;

    if (cannot_refract ||
        schlick_reflectance(cos_theta, eta_incident_over_eta_refract) >
            rand_double(state)) {
      direction = reflect(unit_direction, rec.normal);
    } else {
      direction = refract(unit_direction, rec.normal,
                          eta_incident_over_eta_refract);
    }

    scatter_rec.scattered = DeviceRay(rec.point, direction, ray.time);
    return 1;
  }

  if (material.type == MAT_DIFFUSE_LIGHT) {
    return 0;
  }

  if (material.type == MAT_ISOTROPIC) {
    scatter_rec.attenuation =
        sample_texture(material.texture_id, textures, texture_count, rec.point,
                       rec.u, rec.v);
    scatter_rec.skip_pdf = 0;
    scatter_rec.pdf_type = PDF_SPHERE;
    scatter_rec.pdf_normal = rec.normal;
    return 1;
  }

  return 0;
}

__device__ inline DeviceVec3 sample_pdf_direction(int pdf_type,
                                                  DeviceVec3 const &normal,
                                                  curandState *state) {
  if (pdf_type == PDF_COSINE) {
    return random_cosine_direction(normal, state);
  }
  if (pdf_type == PDF_SPHERE) {
    return random_unit_vector(state);
  }
  return normal;
}

__device__ inline double pdf_value_from_type(int pdf_type,
                                             DeviceVec3 const &normal,
                                             DeviceVec3 const &direction) {
  if (pdf_type == PDF_COSINE) {
    double cosine = dot(unit(direction), normal);
    return cosine < 0.0 ? 0.0 : cosine / kPi;
  }
  if (pdf_type == PDF_SPHERE) {
    return 1.0 / (4.0 * kPi);
  }
  return 0.0;
}

__device__ inline double material_scatter_pdf(DeviceMaterial const &material,
                                              DeviceHitRecord const &rec,
                                              DeviceVec3 const &direction) {
  if (material.type == MAT_LAMBERTIAN) {
    double cosine = dot(rec.normal, unit(direction));
    return cosine < 0.0 ? 0.0 : cosine / kPi;
  }
  if (material.type == MAT_ISOTROPIC) {
    return 1.0 / (4.0 * kPi);
  }
  return 0.0;
}

__device__ inline DeviceRay get_sample_ray(DeviceCameraParams const &camera,
                                           int x, int y, int stratified_x,
                                           int stratified_y,
                                           curandState *state) {
  double offset_x =
      (rand_double(state) + stratified_x) * camera.reciprocal_sqrt_spp - 0.5;
  double offset_y =
      (rand_double(state) + stratified_y) * camera.reciprocal_sqrt_spp - 0.5;

  DeviceVec3 sample_pixel_center =
      add(camera.pixel00,
          add(mul(camera.pixel_delta_u, x + offset_x),
              mul(camera.pixel_delta_v, y + offset_y)));

  DeviceVec3 ray_origin = camera.center;
  if (camera.enable_defocus) {
    DeviceVec3 disk = random_in_unit_disk(state);
    ray_origin = add(camera.center,
                     add(mul(camera.defocus_disk_u, disk.x),
                         mul(camera.defocus_disk_v, disk.y)));
  }

  DeviceVec3 ray_direction = sub(sample_pixel_center, ray_origin);
  return DeviceRay(ray_origin, ray_direction, rand_double(state));
}

__device__ inline DeviceVec3 trace_ray(
    DeviceRay const &initial_ray, DeviceCameraParams const &camera,
    DevicePrimitive const *primitives, int primitive_count,
    DeviceMaterial const *materials, DeviceTexture const *textures,
    int texture_count, DeviceLightPrimitive const *lights, int light_count,
    curandState *state) {
  DeviceVec3 throughput = make_vec3(1.0, 1.0, 1.0);
  DeviceVec3 accumulated = make_vec3(0.0, 0.0, 0.0);
  DeviceRay ray = initial_ray;

  for (int depth = 0; depth < camera.max_depth; ++depth) {
    DeviceHitRecord rec{};
    if (!hit_world(primitives, primitive_count, ray, 0.001, kInfinity, rec)) {
      accumulated = add(accumulated, mul(throughput, camera.background));
      break;
    }

    DeviceMaterial const &material = materials[rec.material_id];
    DeviceVec3 emitted =
        material_emitted(material, textures, texture_count, rec, ray);
    accumulated = add(accumulated, mul(throughput, emitted));

    DeviceScatterRecord scatter_rec{};
    if (!material_scatter(material, textures, texture_count, ray, rec,
                          scatter_rec, state)) {
      break;
    }

    if (scatter_rec.skip_pdf) {
      throughput = mul(throughput, scatter_rec.attenuation);
      ray = scatter_rec.scattered;
      continue;
    }

    DeviceVec3 material_direction =
        sample_pdf_direction(scatter_rec.pdf_type, scatter_rec.pdf_normal, state);
    DeviceVec3 scattered_direction = material_direction;

    if (light_count > 0 && rand_double(state) < 0.5) {
      scattered_direction =
          sample_light_direction(lights, light_count, rec.point, state);
    }

    if (near_zero(scattered_direction)) {
      break;
    }

    double light_pdf = light_count > 0
                           ? lights_pdf_value(lights, light_count, rec.point,
                                              scattered_direction)
                           : 0.0;
    double material_pdf =
        pdf_value_from_type(scatter_rec.pdf_type, scatter_rec.pdf_normal,
                            scattered_direction);

    double mixture_pdf =
        light_count > 0 ? (0.5 * light_pdf + 0.5 * material_pdf) : material_pdf;
    if (mixture_pdf <= 1e-12) {
      break;
    }

    double scattering_pdf =
        material_scatter_pdf(material, rec, scattered_direction);
    throughput = mul(throughput, scatter_rec.attenuation);
    throughput = mul(throughput, scattering_pdf / mixture_pdf);

    ray = DeviceRay(rec.point, scattered_direction, ray.time);
  }

  return accumulated;
}

__global__ void init_rng_kernel(curandState *rng_states, int pixel_count,
                                unsigned long seed) {
  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  if (idx >= pixel_count) {
    return;
  }
  curand_init(seed + static_cast<unsigned long>(idx), 0, 0, &rng_states[idx]);
}

__global__ void render_kernel(DeviceCameraParams camera,
                              DevicePrimitive const *primitives,
                              int primitive_count,
                              DeviceMaterial const *materials,
                              DeviceTexture const *textures, int texture_count,
                              DeviceLightPrimitive const *lights,
                              int light_count, curandState *rng_states,
                              DeviceVec3 *out_pixels) {
  int x = blockIdx.x * blockDim.x + threadIdx.x;
  int y = blockIdx.y * blockDim.y + threadIdx.y;

  if (x >= camera.image_width || y >= camera.image_height) {
    return;
  }

  int index = y * camera.image_width + x;
  curandState local_rng = rng_states[index];

  DeviceVec3 pixel_color = make_vec3(0.0, 0.0, 0.0);

  for (int sy = 0; sy < camera.sqrt_spp; ++sy) {
    for (int sx = 0; sx < camera.sqrt_spp; ++sx) {
      DeviceRay ray = get_sample_ray(camera, x, y, sx, sy, &local_rng);
      DeviceVec3 sample_color =
          trace_ray(ray, camera, primitives, primitive_count, materials,
                    textures, texture_count, lights, light_count, &local_rng);
      pixel_color = add(pixel_color, sample_color);
    }
  }

  out_pixels[index] = mul(pixel_color, camera.sample_scale);
  rng_states[index] = local_rng;
}

inline DeviceCameraParams build_camera_params(
    double aspect_ratio, int image_width, int image_height, int spp,
    int max_depth, color3 const &background, point3 const &lookfrom,
    point3 const &lookat, vec3 const &up, double vFov, double focus_distance,
    double defocus_angle) {
  DeviceCameraParams camera{};

  camera.image_width = image_width;
  camera.image_height = image_height;
  camera.max_depth = max_depth;

  int sqrt_spp = static_cast<int>(std::sqrt(static_cast<double>(spp)));
  if (sqrt_spp < 1) {
    sqrt_spp = 1;
  }
  camera.sqrt_spp = sqrt_spp;
  camera.reciprocal_sqrt_spp = 1.0 / static_cast<double>(sqrt_spp);
  camera.sample_scale =
      1.0 / static_cast<double>(sqrt_spp * sqrt_spp);

  camera.background = to_device_vec3(background);
  camera.center = to_device_vec3(lookfrom);

  double theta = degrees_to_radians(vFov);
  double viewport_height = 2.0 * focus_distance * std::tan(theta / 2.0);
  double viewport_width = viewport_height *
                          (static_cast<double>(image_width) /
                           static_cast<double>(image_height));

  vec3 w = unit_vector(lookfrom - lookat);
  vec3 u = unit_vector(crossProduct(up, w));
  vec3 v = unit_vector(crossProduct(w, u));

  vec3 viewport_u = viewport_width * u;
  vec3 viewport_v = -(viewport_height * v);

  vec3 pixel_delta_u = viewport_u / static_cast<double>(image_width);
  vec3 pixel_delta_v = viewport_v / static_cast<double>(image_height);

  point3 viewport_upper_left =
      lookfrom - focus_distance * w - viewport_u / 2.0 - viewport_v / 2.0;
  point3 pixel00 = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

  camera.pixel00 = to_device_vec3(pixel00);
  camera.pixel_delta_u = to_device_vec3(pixel_delta_u);
  camera.pixel_delta_v = to_device_vec3(pixel_delta_v);

  double defocus_radius =
      focus_distance * std::tan(degrees_to_radians(defocus_angle / 2.0));
  camera.defocus_disk_u = to_device_vec3(u * defocus_radius);
  camera.defocus_disk_v = to_device_vec3(v * defocus_radius);
  camera.enable_defocus = defocus_angle > 0.0 ? 1 : 0;

  (void)aspect_ratio;
  return camera;
}

inline void log_cuda_failure(char const *step, cudaError_t err) {
  std::cerr << "CUDA backend failure in " << step << ": "
            << cudaGetErrorString(err) << "\n";
}

} // namespace detail

bool render_with_cuda(double aspect_ratio, int image_width, int image_height,
                      int samples_per_pixel, int max_depth,
                      color3 const &background, point3 const &lookfrom,
                      point3 const &lookat, vec3 const &up, double vFov,
                      double focus_distance, double defocus_angle,
                      hittable const &world_objects, hittable const &lights,
                      std::vector<color3> &out_pixels) {
  using namespace detail;

  if (image_width <= 0 || image_height <= 0 || samples_per_pixel <= 0 ||
      max_depth <= 0) {
    return false;
  }

  int device_count = 0;
  cudaError_t err = cudaGetDeviceCount(&device_count);
  if (err != cudaSuccess || device_count == 0) {
    return false;
  }

  SceneConverter converter;
  if (!converter.build(world_objects, lights)) {
    std::clog << "CUDA backend fallback: " << converter.error() << "\n";
    return false;
  }

  DevicePrimitive *d_primitives = nullptr;
  DeviceMaterial *d_materials = nullptr;
  DeviceTexture *d_textures = nullptr;
  DeviceLightPrimitive *d_lights = nullptr;
  curandState *d_rng_states = nullptr;
  DeviceVec3 *d_output = nullptr;

  bool success = false;
  int pixel_count = image_width * image_height;

  do {
    err = cudaMalloc(reinterpret_cast<void **>(&d_primitives),
                     converter.primitives.size() * sizeof(DevicePrimitive));
    if (err != cudaSuccess) {
      log_cuda_failure("cudaMalloc primitives", err);
      break;
    }

    err = cudaMalloc(reinterpret_cast<void **>(&d_materials),
                     converter.materials.size() * sizeof(DeviceMaterial));
    if (err != cudaSuccess) {
      log_cuda_failure("cudaMalloc materials", err);
      break;
    }

    err = cudaMalloc(reinterpret_cast<void **>(&d_textures),
                     converter.textures.size() * sizeof(DeviceTexture));
    if (err != cudaSuccess) {
      log_cuda_failure("cudaMalloc textures", err);
      break;
    }

    if (!converter.light_primitives.empty()) {
      err = cudaMalloc(reinterpret_cast<void **>(&d_lights),
                       converter.light_primitives.size() *
                           sizeof(DeviceLightPrimitive));
      if (err != cudaSuccess) {
        log_cuda_failure("cudaMalloc lights", err);
        break;
      }
    }

    err = cudaMalloc(reinterpret_cast<void **>(&d_rng_states),
                     pixel_count * sizeof(curandState));
    if (err != cudaSuccess) {
      log_cuda_failure("cudaMalloc rng", err);
      break;
    }

    err = cudaMalloc(reinterpret_cast<void **>(&d_output),
                     pixel_count * sizeof(DeviceVec3));
    if (err != cudaSuccess) {
      log_cuda_failure("cudaMalloc output", err);
      break;
    }

    err = cudaMemcpy(d_primitives, converter.primitives.data(),
                     converter.primitives.size() * sizeof(DevicePrimitive),
                     cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
      log_cuda_failure("cudaMemcpy primitives", err);
      break;
    }

    err = cudaMemcpy(d_materials, converter.materials.data(),
                     converter.materials.size() * sizeof(DeviceMaterial),
                     cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
      log_cuda_failure("cudaMemcpy materials", err);
      break;
    }

    err = cudaMemcpy(d_textures, converter.textures.data(),
                     converter.textures.size() * sizeof(DeviceTexture),
                     cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
      log_cuda_failure("cudaMemcpy textures", err);
      break;
    }

    if (!converter.light_primitives.empty()) {
      err = cudaMemcpy(d_lights, converter.light_primitives.data(),
                       converter.light_primitives.size() *
                           sizeof(DeviceLightPrimitive),
                       cudaMemcpyHostToDevice);
      if (err != cudaSuccess) {
        log_cuda_failure("cudaMemcpy lights", err);
        break;
      }
    }

    DeviceCameraParams camera =
        build_camera_params(aspect_ratio, image_width, image_height,
                            samples_per_pixel, max_depth, background, lookfrom,
                            lookat, up, vFov, focus_distance, defocus_angle);

    dim3 init_threads(256, 1, 1);
    dim3 init_blocks((pixel_count + init_threads.x - 1) / init_threads.x, 1, 1);
    init_rng_kernel<<<init_blocks, init_threads>>>(d_rng_states, pixel_count,
                                                    1337UL);

    err = cudaGetLastError();
    if (err != cudaSuccess) {
      log_cuda_failure("init_rng_kernel launch", err);
      break;
    }

    err = cudaDeviceSynchronize();
    if (err != cudaSuccess) {
      log_cuda_failure("init_rng_kernel synchronize", err);
      break;
    }

    dim3 threads(kThreadsX, kThreadsY, 1);
    dim3 blocks((image_width + kThreadsX - 1) / kThreadsX,
                (image_height + kThreadsY - 1) / kThreadsY, 1);

    render_kernel<<<blocks, threads>>>(
        camera, d_primitives, static_cast<int>(converter.primitives.size()),
        d_materials, d_textures, static_cast<int>(converter.textures.size()),
        d_lights, static_cast<int>(converter.light_primitives.size()),
        d_rng_states, d_output);

    err = cudaGetLastError();
    if (err != cudaSuccess) {
      log_cuda_failure("render_kernel launch", err);
      break;
    }

    err = cudaDeviceSynchronize();
    if (err != cudaSuccess) {
      log_cuda_failure("render_kernel synchronize", err);
      break;
    }

    std::vector<DeviceVec3> host_output(pixel_count);
    err = cudaMemcpy(host_output.data(), d_output,
                     pixel_count * sizeof(DeviceVec3),
                     cudaMemcpyDeviceToHost);
    if (err != cudaSuccess) {
      log_cuda_failure("cudaMemcpy output", err);
      break;
    }

    out_pixels.resize(pixel_count);
    for (int i = 0; i < pixel_count; ++i) {
      out_pixels[i] = to_host_vec3(host_output[i]);
    }

    success = true;
  } while (false);

  if (d_output) {
    cudaFree(d_output);
  }
  if (d_rng_states) {
    cudaFree(d_rng_states);
  }
  if (d_lights) {
    cudaFree(d_lights);
  }
  if (d_textures) {
    cudaFree(d_textures);
  }
  if (d_materials) {
    cudaFree(d_materials);
  }
  if (d_primitives) {
    cudaFree(d_primitives);
  }

  return success;
}

} // namespace cuda_backend

#else

namespace cuda_backend {

inline bool render_with_cuda(double aspect_ratio, int image_width,
                             int image_height, int samples_per_pixel,
                             int max_depth, color3 const &background,
                             point3 const &lookfrom, point3 const &lookat,
                             vec3 const &up, double vFov,
                             double focus_distance, double defocus_angle,
                             hittable const &world_objects,
                             hittable const &lights,
                             std::vector<color3> &out_pixels) {
  (void)aspect_ratio;
  (void)image_width;
  (void)image_height;
  (void)samples_per_pixel;
  (void)max_depth;
  (void)background;
  (void)lookfrom;
  (void)lookat;
  (void)up;
  (void)vFov;
  (void)focus_distance;
  (void)defocus_angle;
  (void)world_objects;
  (void)lights;
  (void)out_pixels;
  return false;
}

} // namespace cuda_backend

#endif

#endif