#pragma once
#include"common.h"

bool set_mode(const char mode[4]);
rgbImage demosaicing_linear(const rgbImage& source);
rgbImage demosaicing_edge(const rgbImage& source);
