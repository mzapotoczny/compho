#pragma once
#include"common.h"
#include<functional>

color rgb_to_ycbcr(color in);
color ycbcr_to_rgb(color in);

void convert_colorspace(rgbImage& source, std::function<color(color)> converter);
