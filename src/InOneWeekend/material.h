#ifndef MATERIAL_H
#define MATERIAL_H

#include "color.h"
#include "hittable.h"
#include "ray.h"
#include "vec3.h"
#include <cmath>

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
  metal(color3 const &albedo, double fuzziness)
      : albedo(albedo), fuzziness(fuzziness) {}

  bool Scatter(const Ray &ray_in, const hit_record &record, color3 &attenuation,
               Ray &scattered) const override {
    vec3 reflected = unit_vector(
        reflect(unit_vector(ray_in.getDirection()), record.normalAgainstRay));
    reflected = unit_vector(reflected +
                            fuzziness * generate_random_diffused_unitVector());
    scattered = Ray(record.hitPoint, reflected);
    attenuation = albedo;
    return (scattered.getDirection() * record.normalAgainstRay > 0);
  }

private:
  color3 albedo;
  double fuzziness;
};

class dielectric : public Material {
public:
  dielectric(double refraction_index) : refraction_index(refraction_index) {}

  bool Scatter(const Ray &ray_in, const hit_record &record, color3 &attenuation,
               Ray &scattered) const override {
    attenuation = color3(1.0, 1.0, 1.0);
    double etaIncidentOverEtaRefract =
        record.frontFace ? 1.0 / refraction_index : refraction_index;

    vec3 scattered_direction;
    if (isTotalInternalReflection(ray_in, record)) {
      // reflect
      scattered_direction =
          reflect(ray_in.getDirection(), record.normalAgainstRay);
    } else {
      // can refract
      scattered_direction =
          refract(ray_in.getDirection(), record.normalAgainstRay,
                  etaIncidentOverEtaRefract);
    }

    scattered = Ray(record.hitPoint, scattered_direction);
    return true;
  }

private:
  double refraction_index;
  bool isTotalInternalReflection(Ray const &ray_in,
                                 hit_record const &record) const {
    double etaIncidentOverEtaRefract =
        record.frontFace ? 1.0 / refraction_index : refraction_index;

    vec3 unit_direction = unit_vector(ray_in.getDirection());
    vec3 unit_normal = unit_vector(record.normalAgainstRay);

    double cos_theta = std::fmin(-unit_direction * unit_normal, 1.0);
    double sin_theta = std::sqrt(1 - cos_theta * cos_theta);

    return etaIncidentOverEtaRefract * sin_theta > 1.0;
  }
};

#endif // MATERIAL_H