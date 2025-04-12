#ifndef PDF_H
#define PDF_H

#include "common.h"
#include "hittable.h"
#include "orthonormalbasis.h"
#include "vec3.h"
#include <cmath>
#include <memory>

class pdf {
public:
  pdf() {}
  virtual double value(vec3 const &direction) const = 0;
  virtual vec3 generate() const = 0;

private:
};

class sphere_pdf : public pdf {
public:
  sphere_pdf();
  double value(vec3 const &direction) const override { return 1 / (4 * PI); }
  vec3 generate() const override {
    return generate_random_diffused_unitVector();
  }

private:
};

class cosine_pdf : public pdf {
public:
  cosine_pdf(vec3 const &w) : uvw(w) {}

  double value(vec3 const &direction) const override {
    auto cosine_theta = dotProduct(unit_vector(direction), uvw.getw());
    return cosine_theta > 0.0 ? cosine_theta / PI : 0.0;
  }

  vec3 generate() const override {
    return uvw.transform(random_cosine_direction());
  }

private:
  onb uvw;
};

class hittable_pdf : public pdf {
public:
  hittable_pdf(hittable const &objects, point3 const &origin)
      : objects(objects), origin(origin) {}

  double value(vec3 const &direction) const override {
    return objects.pdf_value(origin, direction);
  }

  vec3 generate() const override { return objects.random(origin); }

private:
  hittable const &objects;
  point3 origin;
};

class mixture_pdf : public pdf {
public:
  mixture_pdf(std::shared_ptr<pdf> p0, std::shared_ptr<pdf> p1) {
    p[0] = p0;
    p[1] = p1;
  }
  double value(vec3 const &direction) const override {
    return 0.5 * p[0]->value(direction) + 0.5 * p[1]->value(direction);
  }
  vec3 generate() const override {
    if (random_double() < 0.5)
      return p[0]->generate();
    else
      return p[1]->generate();
  }

private:
  shared_ptr<pdf> p[2];
};

#endif // PDF_H