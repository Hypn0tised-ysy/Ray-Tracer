#ifndef MATERIAL_H
#define MATERIAL_H

#include "color.h"
#include "hittable.h"
#include "ray.h"
#include "vec3.h"

class Material {
public:
  virtual ~Material() = default;

  virtual bool Scatter(const Ray &ray_in, const hit_record &record,
                       color3 &attenuation, Ray &scattered) const = 0;
};

class lambertian : public Material {
public:
  lambertian(const color3 &albedo) : albedo(albedo) {}

  bool Scatter(const Ray &ray_in, const hit_record &record, color3 &attenuation,
               Ray &scattered) const override {
    vec3 diffuse_direction = generate_diffuse_direction(record);
    scattered = Ray(record.hitPoint, diffuse_direction);
    attenuation = albedo;
    return true;
  }

private:
  color3 albedo;

  vec3 generate_diffuse_direction(const hit_record &record) const {
    vec3 diffuse_direction =
        record.normalAgainstRay + generate_random_diffused_unitVector();
    if (diffuse_direction.nearZero())
      diffuse_direction = record.normalAgainstRay;
    return unit_vector(diffuse_direction);
  }
};

class metal : public Material {
public:
  metal(color3 const &albedo) : albedo(albedo) {}

  bool Scatter(const Ray &ray_in, const hit_record &record, color3 &attenuation,
               Ray &scattered) const override {
    vec3 reflected =
        reflect(unit_vector(ray_in.getDirection()), record.normalAgainstRay);
    scattered = Ray(record.hitPoint, reflected);
    attenuation = albedo;
    return true;
  }

private:
  color3 albedo;
};
#endif // MATERIAL_H