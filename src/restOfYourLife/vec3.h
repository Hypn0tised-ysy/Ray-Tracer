#ifndef VEC3_H
#define VEC3_H

#include "common.h"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <ostream>

double constexpr AvoidDivideByZero = 1e-160;
class vec3 {
public:
  union {
    struct {
      double x;
      double y;
      double z;
    };
    struct {
      double r;
      double g;
      double b;
    };
    double element[3];
  };
  vec3() : element{0.0, 0.0, 0.0} {};
  vec3(double _x, double _y, double _z) : element{_x, _y, _z} {};

  double norm() const { return std::sqrt(norm_square()); };
  double norm_square() const { return x * x + y * y + z * z; }

  bool nearZero() {
    double epsilon = 1e-8;
    return (std::fabs(x) < epsilon) && (std::fabs(y) < epsilon) &&
           (std::fabs(z) < epsilon);
  }

  static vec3 generate_random_vector() { // 生成[0,1]^3空间内的随机向量
    return vec3(random_double(), random_double(), random_double());
  }
  static vec3 generate_random_vector(double min, double max) {
    return vec3(random_double(min, max), random_double(min, max),
                random_double(min, max));
  }
  static vec3 generate_random_vector_onUnitDisk() {
    while (true) {
      vec3 randomVector = vec3(random_double(), random_double(), 0.0);
      if (randomVector.norm_square() <= 1.0)
        return randomVector;
    }
  }

  double operator[](int ith_element) const {
    assert(0 <= ith_element && ith_element <= 2);
    return element[ith_element];
  }
  double &operator[](int ith_element) {
    assert(0 <= ith_element && ith_element <= 2);
    return element[ith_element];
  }

  vec3 operator-() const { return vec3(-x, -y, -z); }

  vec3 &operator+=(vec3 const &vectorToAdd) {
    x += vectorToAdd.x;
    y += vectorToAdd.y;
    z += vectorToAdd.z;
    return *this;
  }

  vec3 &operator*=(double factor) {
    x *= factor;
    y *= factor;
    z *= factor;
    return *this;
  }

  vec3 &operator/=(double divisor) {
    double reciprocal_divisor = 1 / divisor;
    *this *= reciprocal_divisor;
    return *this;
  }
};

using point3 = vec3;

std::ostream &operator<<(std::ostream &out, vec3 const &vector) {
  out << vector.x << ' ' << vector.y << ' ' << vector.z;
  return out;
}

vec3 operator+(vec3 const &leftVector, vec3 const &rightVector) {
  return vec3(leftVector.x + rightVector.x, leftVector.y + rightVector.y,
              leftVector.z + rightVector.z);
}

vec3 operator-(vec3 const &leftVector, vec3 const &rightVector) {
  return vec3(leftVector.x - rightVector.x, leftVector.y - rightVector.y,
              leftVector.z - rightVector.z);
}

vec3 cwiseProduct(vec3 const &leftVector, vec3 const &rightVector) {
  return vec3(leftVector.x * rightVector.x, leftVector.y * rightVector.y,
              leftVector.z * rightVector.z);
}

vec3 operator*(double factor, vec3 const &vector) {
  return vec3(vector.x * factor, vector.y * factor, vector.z * factor);
}

vec3 operator*(vec3 const &vector, double factor) { return factor * vector; }

vec3 operator/(vec3 const &vector, double divisor) {
  return (1 / divisor) * vector;
}

double dotProduct(vec3 const &leftVector, vec3 const &rightVector) {
  return leftVector.x * rightVector.x + leftVector.y * rightVector.y +
         leftVector.z * rightVector.z;
}

double operator*(vec3 const &leftVector, vec3 const &rightVector) {
  return leftVector.x * rightVector.x + leftVector.y * rightVector.y +
         leftVector.z * rightVector.z;
}

vec3 crossProduct(vec3 const &leftVector, vec3 const &rightVector) {
  return vec3(leftVector.y * rightVector.z - leftVector.z * rightVector.y,
              leftVector.z * rightVector.x - leftVector.x * rightVector.z,
              leftVector.x * rightVector.y - leftVector.y * rightVector.x);
}

vec3 unit_vector(vec3 const &vector) { return vector / vector.norm(); }

vec3 generate_random_diffused_unitVector() {
  while (true) {
    vec3 randomVector = vec3::generate_random_vector(-1.0, 1.0);
    double norm2 = randomVector.norm_square();
    if (norm2 > AvoidDivideByZero && norm2 <= 1.0) {
      return randomVector / std::sqrt(norm2);
    }
  }
}

vec3 generate_random_vector(double min, double max) {
  return vec3(random_double(min, max), random_double(min, max),
              random_double(min, max));
}

vec3 generate_random_diffused_unitVector_onHemisphere(
    vec3 const &normalAgainstRay) {
  vec3 randomVector = generate_random_diffused_unitVector();
  randomVector =
      randomVector * normalAgainstRay > 0.0 ? randomVector : -randomVector;
  return randomVector;
}

vec3 project(vec3 const &a, vec3 const &ontoB) {
  return (a * ontoB / ontoB.norm_square()) * ontoB;
}

vec3 reflect(vec3 const &incidentRay, vec3 const &normal) {
  return incidentRay - 2 * project(incidentRay, normal);
}

vec3 refract(vec3 const &incidentDirection, vec3 const &normal,
             double etaIncidentOverEtaRefract) {
  vec3 a = unit_vector(incidentDirection), n = unit_vector(normal);
  vec3 refract_n_parallel, refract_n_perpendicular;
  double cos_theta = std::fmin(-(a * n), 1.0);
  refract_n_perpendicular = etaIncidentOverEtaRefract * (a + cos_theta * n);
  refract_n_parallel =
      -std::sqrt(std::fabs(1 - refract_n_perpendicular.norm_square())) * n;
  return refract_n_parallel + refract_n_perpendicular;
}

vec3 random_cosine_direction() {
  // generate distribution: p(omega)=cos(theta)/pi
  auto r1 = random_double();
  auto r2 = random_double();

  auto phi = 2 * PI * r1;
  auto x = std::cos(phi) * std::sqrt(r2);
  auto y = std::sin(phi) * std::sqrt(r2);
  auto z = std::sqrt(1 - r2);

  return vec3(x, y, z);
}

#endif