#pragma once
#include"CImg.h"
#include<utility>

using namespace cimg_library;

#define AT(x, y, w) (y*w + x)

typedef CImg<unsigned char> rgbImage;
typedef std::pair<int, int> point;
typedef std::tuple<unsigned char, unsigned char, unsigned char> color;
enum BasicColors { R = 0, G = 1, B = 2 };

template<typename T>
inline T clip(T n, T lower, T upper) {
  return n <= lower ? lower : n >= upper ? upper : n;
}

