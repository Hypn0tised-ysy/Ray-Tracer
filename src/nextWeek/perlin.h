#ifndef PERLIN_H
#define PERLIN_H

#include "nextWeek/common.h"
#include "nextWeek/vec3.h"
#include <cmath>
class perlin {
public:
  perlin() {
    for (int i = 0; i < point_count; i++) {
      randfloat[i] = random_double();
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

    // hermite cubic,用于消除mach band，降低线性程度，让结果看上去更随机
    // 在这里就是用一个三次函数代替一次函数
    u = u * u * (3 - 2 * u);
    v = u * v * (3 - 2 * v);
    w = u * w * (3 - 2 * w);

    auto x_integer_part = int(std::floor(p.x));
    auto y_integer_part = int(std::floor(p.y));
    auto z_integer_part = int(std::floor(p.z));

    double data[2][2][2]; // 给定坐标点临近八个网格点的梯度
    for (int di = 0; di < 2; di++)
      for (int dj = 0; dj < 2; dj++)
        for (int dk = 0; dk < 2; dk++)
          data[di][dj][dk] = randfloat[perm_x[(x_integer_part + di) & 255] ^
                                       perm_y[(y_integer_part + dj) & 255] ^
                                       perm_z[(z_integer_part + dk) & 255]];

    return trilinear_interpolation(data, u, v, w);
  }

private:
  static const int point_count = 256;
  double randfloat[point_count];
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
  double trilinear_interpolation(double data[2][2][2], double u, double v,
                                 double w) const {
    // 三维的三线性插值，神乎其神的算法,uvw为映射到标准立方体的坐标，即[0,1]^3内
    auto accumulate = 0.0;
    for (int i = 0; i < 2; i++)
      for (int j = 0; j < 2; j++)
        for (int k = 0; k < 2; k++)
          accumulate += (i * u + (1 - i) * (1 - u)) *
                        (j * v + (1 - j) * (1 - v)) *
                        (k * w + (1 - k) * (1 - w)) * data[i][j][k];
    return accumulate;
  }
  double trilinear_interpolation(double data[2][2][2],
                                 point3 const &canonical_coordinate) const {
    return trilinear_interpolation(data, canonical_coordinate.x,
                                   canonical_coordinate.y,
                                   canonical_coordinate.z);
  }
};

#endif