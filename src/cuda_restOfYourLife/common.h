#ifndef COMMON_H
#define COMMON_H

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>

using std::make_shared;
using std::shared_ptr;

#ifndef CUDA_RT_HD
#ifdef __CUDACC__
#define CUDA_RT_HD __host__ __device__
#else
#define CUDA_RT_HD
#endif
#endif

double constexpr Infinity_double = std::numeric_limits<double>::infinity();
double constexpr PI = 3.1415926535897932385;

CUDA_RT_HD inline double degrees_to_radians(double degrees) {
  return degrees * PI / 180.0;
}

CUDA_RT_HD inline double random_double() {
  // return a double in [0, 1)
#ifdef __CUDA_ARCH__
  // Stateless fallback RNG for device-side utility calls.
  unsigned int seed = static_cast<unsigned int>(clock64());
  seed ^= static_cast<unsigned int>(threadIdx.x + blockIdx.x * blockDim.x + 1);
  seed = 1664525u * seed + 1013904223u;
  return static_cast<double>(seed & 0x00FFFFFFu) / 16777216.0;
#else
  return std::rand() / (RAND_MAX + 1.0);
#endif
}

CUDA_RT_HD inline double random_double(double min, double max) {
  // return a double in [min, max)
  return min + (max - min) * random_double();
}

CUDA_RT_HD inline int random_int(int min, int max) {
  // generate a random int in [min,max]
  double random_number = random_double(min, max + 1);
  return int(random_number);
}

#endif // COMMON_H