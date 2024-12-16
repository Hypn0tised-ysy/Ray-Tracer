#ifndef MATERIAL_H
#define MATERIAL_H

#include "color.h"
#include "common.h"
#include "hittable.h"
#include "ray.h"
#include "texture.h"
#include "vec3.h"
#include <cmath>
#include <memory>

class Material {
public:
  virtual ~Material() = default;

  virtual bool Scatter(const Ray &ray_in, const hit_record &record,
                       color3 &attenuation, Ray &scattered) const = 0;
};

class lambertian : public Material {
public:
  lambertian(const color3 &albedo) : tex(make_shared<solid_color>(albedo)) {}
  lambertian(shared_ptr<texture> const &tex) : tex(tex) {}

  bool Scatter(const Ray &ray_in, const hit_record &record, color3 &attenuation,
               Ray &scattered) const override {
    vec3 diffuse_direction = generate_diffuse_direction(record);
    scattered = Ray(record.hitPoint, diffuse_direction, ray_in.getTime());
    attenuation = tex->value(record.textureCoordinate);
    return true;
  }

private:
  shared_ptr<texture> tex;

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
    scattered = Ray(record.hitPoint, reflected, ray_in.getTime());
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
    bool TIR = isTotalInternalReflection(ray_in, record);
    // reflect
    if (TIR || reflectance(ray_in, record) > random_double())
      scattered_direction =
          reflect(ray_in.getDirection(), record.normalAgainstRay);
    // refract
    else
      scattered_direction =
          refract(ray_in.getDirection(), record.normalAgainstRay,
                  etaIncidentOverEtaRefract);

    scattered = Ray(record.hitPoint, scattered_direction, ray_in.getTime());
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

  double reflectance(const Ray &ray_in, const hit_record &record) const {
    // Schlick's approximation for reflectance
    vec3 unit_direction = unit_vector(ray_in.getDirection());
    vec3 unit_normal = unit_vector(record.normalAgainstRay);
    double cos_theta = -unit_direction * unit_normal;
    double etaIncidentOverEtaRefract =
        record.frontFace ? 1.0 / refraction_index : refraction_index;

    auto r0 = (1 - etaIncidentOverEtaRefract) / (1 + etaIncidentOverEtaRefract);
    r0 = r0 * r0;
    return r0 + (1 - r0) * std::pow((1 - cos_theta), 5);
  }
};

#endif // MATERIAL_H