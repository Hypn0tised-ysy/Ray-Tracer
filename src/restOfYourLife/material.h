#ifndef MATERIAL_H
#define MATERIAL_H

#include "color.h"
#include "common.h"
#include "hittable.h"
#include "orthonormalbasis.h"
#include "pdf.h"
#include "ray.h"
#include "sphere.h"
#include "texture.h"
#include "vec3.h"
#include <cmath>
#include <memory>

class scatter_record {
public:
  color3 attenuation;
  std::shared_ptr<pdf> pdf_ptr;
  bool skip_pdf;
  Ray skip_pdf_ray;
};
class Material {
public:
  virtual ~Material() = default;

  virtual color3 emitted(double u, double v, point3 const &point) const {
    return color3(0, 0, 0);
  }
  virtual color3 emitted(texture_coordinate const &texture_coordinate,
                         point3 const &point) const {
    return emitted(texture_coordinate.u, texture_coordinate.v, point);
  }
  virtual color3 emitted(const Ray &ray_in, const hit_record &record, double u,
                         double v, const point3 &point) const {
    return color3(0, 0, 0);
  }

  virtual bool Scatter(const Ray &ray_in, const hit_record &record,
                       scatter_record &scatter_rec) const {
    return false;
  }

  virtual double Scatter_pdf(const Ray &ray_in, const hit_record &record,
                             const Ray &scattered) const {
    return 0.0;
  }
};

class lambertian : public Material {
public:
  lambertian(const color3 &albedo) : tex(make_shared<solid_color>(albedo)) {}
  lambertian(shared_ptr<texture> const &tex) : tex(tex) {}

  bool Scatter(const Ray &ray_in, const hit_record &record,
               scatter_record &scatter_rec) const override {
    scatter_rec.attenuation =
        tex->value(record.textureCoordinate, record.hitPoint);
    scatter_rec.pdf_ptr = make_shared<cosine_pdf>(record.normalAgainstRay);
    scatter_rec.skip_pdf = false;
    return true;
  }

  double Scatter_pdf(const Ray &ray_in, const hit_record &record,
                     const Ray &scattered) const override {
    auto cos_theta = dotProduct(record.normalAgainstRay,
                                unit_vector(scattered.getDirection()));
    return cos_theta < 0.0 ? 0.0 : cos_theta / PI;
  }

private:
  shared_ptr<texture> tex;
};

class metal : public Material {
public:
  metal(color3 const &albedo, double fuzziness)
      : albedo(albedo), fuzziness(fuzziness) {}

  bool Scatter(const Ray &ray_in, const hit_record &record,
               scatter_record &scatter_rec) const override {
    vec3 reflected = unit_vector(
        reflect(unit_vector(ray_in.getDirection()), record.normalAgainstRay));
    reflected = unit_vector(reflected +
                            fuzziness * generate_random_diffused_unitVector());

    scatter_rec.attenuation = albedo;
    scatter_rec.skip_pdf = true;
    scatter_rec.skip_pdf_ray =
        Ray(record.hitPoint, reflected, ray_in.getTime());
    return true;
  }

private:
  color3 albedo;
  double fuzziness;
};

class dielectric : public Material {
public:
  dielectric(double refraction_index) : refraction_index(refraction_index) {}

  bool Scatter(const Ray &ray_in, const hit_record &record,
               scatter_record &scatter_rec) const override {
    scatter_rec.attenuation = color3(1.0, 1.0, 1.0);
    scatter_rec.pdf_ptr = nullptr;
    scatter_rec.skip_pdf = true;

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

    scatter_rec.skip_pdf_ray =
        Ray(record.hitPoint, scattered_direction, ray_in.getTime());
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

class diffuse_light : public Material {
public:
  diffuse_light(shared_ptr<texture> const &tex) : tex(tex) {}
  diffuse_light(color3 const &emit) : tex(make_shared<solid_color>(emit)) {}

  color3 emitted(double u, double v, point3 const &point) const override {
    texture_coordinate tex_coordinate = texture_coordinate(u, v);
    return tex->value(tex_coordinate, point);
  }

  color3 emitted(texture_coordinate const &tex_coordinate,
                 point3 const &point) const override {
    return tex->value(tex_coordinate, point);
  }

  color3 emitted(const Ray &ray_in, const hit_record &record, double u,
                 double v, const point3 &point) const override {
    if (!record.frontFace)
      return color3(0.0, 0.0, 0.0);
    return tex->value(texture_coordinate(u, v), point);
  }

  bool Scatter(const Ray &ray_in, const hit_record &record,
               scatter_record &scatter_rec) const override {
    return false; // Diffuse light does not scatter rays
  }

private:
  shared_ptr<texture> tex;
};

class isotropic : public Material {
public:
  isotropic(shared_ptr<texture> _tex) : tex(_tex) {}
  isotropic(color3 const &albedo) : tex(make_shared<solid_color>(albedo)) {}

  bool Scatter(const Ray &ray_in, const hit_record &record,
               scatter_record &scatter_rec) const override {
    scatter_rec.attenuation =
        tex->value(record.textureCoordinate, record.hitPoint);
    scatter_rec.pdf_ptr = make_shared<sphere_pdf>();
    scatter_rec.skip_pdf = false;
    return true;
  }

  double Scatter_pdf(const Ray &ray_in, const hit_record &record,
                     const Ray &scattered) const override {
    return 1 / (4 * PI);
  }

private:
  shared_ptr<texture> tex;
};

#endif // MATERIAL_H