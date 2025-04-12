#ifndef PDF_H
#define PDF_H

#include "nextWeek/common.h"
#include "nextWeek/vec3.h"
#include "restOfYourLife/orthonormalbasis.h"
#include "restOfYourLife/vec3.h"
#include <cmath>
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

#endif // PDF_H