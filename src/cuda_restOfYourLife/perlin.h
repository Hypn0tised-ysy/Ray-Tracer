#ifndef PERLIN_H
#define PERLIN_H

#include "common.h"
#include "vec3.h"
#include <cmath>
class perlin {
public:
  perlin() {
    for (int i = 0; i < point_count; i++) {
      randomVector[i] = unit_vector(vec3::generate_random_vector(-1, 1));
    }

    perlin_generate_perm(perm_x);
    perlin_generate_perm(perm_y);
    perlin_generate_perm(perm_z);
  }

  double noise(const point3 &p) const {

    // 小数部分，即canonical坐标
    auto u = p.x - std::floor(p.x);
    auto v = p.y - std::floor(p.y);
    auto w = p.z - std::floor(p.z);

    auto x_integer_part = int(std::floor(p.x));
    auto y_integer_part = int(std::floor(p.y));
    auto z_integer_part = int(std::floor(p.z));

    vec3 data[2][2][2]; // 给定坐标点临近八个网格点的梯度
    for (int di = 0; di < 2; di++)
      for (int dj = 0; dj < 2; dj++)
        for (int dk = 0; dk < 2; dk++)
          data[di][dj][dk] = randomVector[perm_x[(x_integer_part + di) & 255] ^
                                          perm_y[(y_integer_part + dj) & 255] ^
                                          perm_z[(z_integer_part + dk) & 255]];

    return perlin_interpolation(data, u, v, w);
  }
  double turbulence(point3 const &point, int depth = 7) const {
    double weight = 1.0;
    double accumulate = 0.0;
    point3 temp_point = point;

    for (int i = 0; i < depth; i++) {
      accumulate += weight * noise(temp_point);
      weight *= 0.5;
      temp_point *= 2;
    }
    return std::fabs(accumulate);
  }

private:
  static const int point_count = 256;
  vec3 randomVector[point_count];
  int perm_x[point_count];
  int perm_y[point_count];
  int perm_z[point_count];

  static void perlin_generate_perm(int *p) {
    for (int i = 0; i < point_count; i++)
      p[i] = i;

    permute(p, point_count);
  }

  static void permute(int *p, int n) {
    for (int i = n - 1; i > 0; i--) {
      int target = random_int(0, i);
      int tmp = p[i];
      p[i] = p[target];
      p[target] = tmp;
    }
  }
  double perlin_interpolation(vec3 data[2][2][2], double u, double v,
                              double w) const {
    // hermite cubic,用三次函数代替一次函数，得到更平滑的过渡
    auto uu = u * u * (3 - 2 * u);
    auto vv = v * v * (3 - 2 * v);
    auto ww = w * w * (3 - 2 * w);

    auto accumulate = 0.0;
    for (int i = 0; i < 2; i++)
      for (int j = 0; j < 2; j++)
        for (int k = 0; k < 2; k++) {
          vec3 deviationFromGridPoint(u - i, v - j, w - k);
          accumulate += (i * uu + (1 - i) * (1 - uu)) *
                        (j * vv + (1 - j) * (1 - vv)) *
                        (k * ww + (1 - k) * (1 - ww)) *
                        dotProduct(deviationFromGridPoint, data[i][j][k]);
          // 这样加权之后权值之和还是1吗？
        }
    return accumulate;
  }
  double trilinear_interpolation(vec3 data[2][2][2],
                                 point3 const &canonical_coordinate) const {
    return perlin_interpolation(data, canonical_coordinate.x,
                                canonical_coordinate.y, canonical_coordinate.z);
  }
};

#endif