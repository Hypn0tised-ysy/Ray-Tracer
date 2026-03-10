#ifndef ORTHONORMALBASIS_H
#define ORTHONORMALBASIS_H

#include "vec3.h"
#include <cmath>

class onb {
public:
  onb(vec3 const &normal) {
    w = unit_vector(normal);
    vec3 random =
        std::fabs(w.x) < 0.9 ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 1.0, 0.0);
    v = unit_vector(crossProduct(w, random));
    u = crossProduct(w, v);
  }

  vec3 getu() const { return u; }
  vec3 getv() const { return v; }
  vec3 getw() const { return w; }

  vec3 transform(vec3 const &random_vec) const {
    // 根据随机向量生成相对坐标系下的随机向量
    double x = random_vec.x, y = random_vec.y, z = random_vec.z;
    return x * u + y * v + z * w;
  }

  vec3 generate_random_relative_vec(vec3 const &random_vec) const {
    return transform(random_vec);
  }

private:
  vec3 u, v, w;
};

#endif // ORTHONORMALBASIS_H