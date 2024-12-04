#ifndef VEC_H
#define VEC_H

#include <cassert>
#include <cmath>
#include <iostream>
#include <ostream>

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

  double operator[](int ith_element) const {
    assert(0 <= ith_element && ith_element <= 2);
    return element[ith_element];
  }
  double &operator[](int ith_element) {
    assert(0 <= ith_element && ith_element <= 2);
    return element[ith_element];
  }

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

  double norm() const { return std::sqrt(norm_square()); };
  double norm_square() const { return x * x + y * y + z * z; }
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

#endif