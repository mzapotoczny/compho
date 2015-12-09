#pragma once

#include"CImg.h"
#include<utility>

using namespace cimg_library;
typedef float img_t;
typedef CImg<img_t> rgbImage;
typedef CImgList<img_t> rgbImageList;
//typedef CImg<float> hdrImage;

typedef std::pair<unsigned int, unsigned int> point;
//typedef std::tuple<unsigned char, unsigned char, unsigned char> color;

template<typename T>
inline T clip(T n, T lower, T upper) {
    return n <= lower ? lower : n >= upper ? upper : n;
}

template<typename T>
inline void clipImage(CImg<T>& img, T lower, T upper) {
    cimg_forXYC(img, x, y, c){
        img(x,y,c) = clip<T>(img(x,y,c), lower, upper);
    }
}

template<typename T>
inline T inRange(T n, T lower, T upper) {
    return (n >= lower && n <= upper);
}
