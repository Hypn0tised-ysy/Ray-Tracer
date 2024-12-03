#include <iostream>
int main() {
  // image
  int constexpr width = 256;
  int constexpr height = 256;

  // render
  std::cout << "P3\n" << width << " " << height << "\n255\n";

  int r, g, b;
  for (g = 0; g < height; g++) {
    for (r = 0; r < width; r++) {
      double rRatio = (double)r / (width - 1);
      double gRatio = (double)g / (height - 1);
      int rIntensity = int(255.999 * rRatio);
      int gIntensity = int(255.999 * gRatio);
      int bIntensity = 0;

      std::cout << rIntensity << " " << gIntensity << " " << bIntensity << "\n";
    }
  }
}