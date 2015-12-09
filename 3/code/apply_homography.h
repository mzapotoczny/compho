#include"common.h"
#ifdef Success
  #undef Success
#endif
#include"Eigen/Dense"

#pragma once


rgbImage applyHomography(rgbImage& original, rgbImage& poster, Eigen::Matrix3f h, bool bilinear = false);
