#ifndef TEXTURE_H
#define TEXTURE_H

#include "color.h"
#include "common.h"
#include "vec3.h"
#include <cmath>

class texture_coordinate {
public:
  texture_coordinate() {}
  texture_coordinate(double u, double v, point3 const &point)
      : u(u), v(v), point(point) {}
  double u, v;
  point3 point;
};

class texture {
public:
  virtual ~texture() = default;
  virtual color3 value(texture_coordinate const &) const = 0;
};

class solid_color : public texture {
public:
  solid_color(color3 const &albedo) : albedo(albedo) {}
  solid_color(double red, double green, double blue)
      : solid_color(color3(red, green, blue)) {}
  virtual color3 value(texture_coordinate const &) const override {
    return albedo;
  }

private:
  color3 albedo;
};

class checker_texture : public texture {
public:
  checker_texture(double scale, shared_ptr<solid_color> even,
                  shared_ptr<solid_color> odd)
      : scale(scale), even(even), odd(odd) {}
  checker_texture(double scale, color3 const &even, color3 const &odd)
      : checker_texture(scale, make_shared<solid_color>(even),
                        make_shared<solid_color>(odd)) {}

  virtual color3
  value(texture_coordinate const &tex_coordinate) const override {
    double scale_reciprocal = 1 / scale;

    double x_scale = scale_reciprocal * tex_coordinate.point.x,
           y_scale = scale_reciprocal * tex_coordinate.point.y,
           z_scale = scale_reciprocal * tex_coordinate.point.z;

    int ix = int(std::floor(x_scale)), iy = int(std::floor(y_scale)),
        iz = int(std::floor(z_scale));

    bool isEven = (ix + iy + iz) % 2 == 0;
    return isEven ? even->value(tex_coordinate) : odd->value(tex_coordinate);
  }

private:
  double scale;
  shared_ptr<solid_color> even;
  shared_ptr<solid_color> odd;
};

#endif // TEXTURE_H