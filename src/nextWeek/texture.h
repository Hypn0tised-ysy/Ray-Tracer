#ifndef TEXTURE_H
#define TEXTURE_H

#include "color.h"
#include "common.h"
#include "perlin.h"
#include "rtw_image.h"
#include "vec3.h"
#include <cmath>

class texture_coordinate {
public:
  texture_coordinate() {}
  texture_coordinate(double u, double v, point3 const &point)
      : u(u), v(v), point(point) {}
  texture_coordinate(point3 const &point) : u(0), v(0), point(point) {}

  double u, v;
  point3 point;
};

class texture {
public:
  virtual ~texture() = default;
  virtual color3 value(texture_coordinate const &,
                       point3 const &hitPoint) const = 0;
};

class solid_color : public texture {
public:
  solid_color(color3 const &albedo) : albedo(albedo) {}
  solid_color(double red, double green, double blue)
      : solid_color(color3(red, green, blue)) {}
  virtual color3 value(texture_coordinate const &,
                       point3 const &hitPoint) const override {
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

  virtual color3 value(texture_coordinate const &tex_coordinate,
                       point3 const &hitPoint) const override {
    double scale_reciprocal = 1 / scale;
    double x_scale = scale_reciprocal * hitPoint.x,
           y_scale = scale_reciprocal * hitPoint.y,
           z_scale = scale_reciprocal * hitPoint.z;

    int ix = int(std::floor(x_scale)), iy = int(std::floor(y_scale)),
        iz = int(std::floor(z_scale));

    bool isEven = (ix + iy + iz) % 2 == 0;
    return isEven ? even->value(tex_coordinate, hitPoint)
                  : odd->value(tex_coordinate, hitPoint);
  }

private:
  double scale;
  shared_ptr<solid_color> even;
  shared_ptr<solid_color> odd;
};

class image_texture : public texture {
public:
  image_texture(const char *filename) : image(filename) {}

  color3 value(texture_coordinate const &tex_coordinate,
               point3 const &hitPoint) const override {
    if (image.height() <= 0)
      return color3(0, 1, 1);

    double u = interval(0, 1).clamp(tex_coordinate.u);
    double v = 1.0 - interval(0, 1).clamp(tex_coordinate.v);

    auto x = int(u * image.width());
    auto y = int(v * image.height());
    auto pixel = image.pixel_data(x, y);

    auto color_scale = 1.0 / 255.0;
    return color_scale * color3(pixel[0], pixel[1], pixel[2]);
  }

private:
  rtw_image image;
};

class perlin_noise_texture : public texture {
public:
  perlin_noise_texture(double _scale = 1.0) : scale(_scale) {}
  color3 value(texture_coordinate const &tex_coordinate,
               point3 const &hitPoint) const override {
    return color3(1, 1, 1) * perlin_noise.noise(hitPoint * scale);
  }

private:
  perlin perlin_noise;
  double scale;
};

#endif // TEXTURE_H