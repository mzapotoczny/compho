#include<iostream>
#include"apply_homography.h"

using namespace Eigen;

void copyPixel(rgbImage& source, int sx, int sy, rgbImage& dest, int dx, int dy){
    for (int i = 0; i < std::min(source.spectrum(), dest.spectrum()); i++){
        dest(dx, dy, 0, i) = source(sx, sy, 0, i);
    }
}

void copyPixelBilinear(rgbImage& source, float sx, float sy, rgbImage& dest, int dx, int dy){
    Matrix2f values;
    int width = source.width(), height = source.height();

    float lsx = sx, lsy = sy;
    float usx = clip<float>(sx+1,0,width), usy = clip<float>(sy+1,0,height);
    sx /= width; sy /= height;
    
    for (int i = 0; i < std::min(source.spectrum(), dest.spectrum()); i++){
        values << source(lsx, lsy, 0, i), source(lsx, usy, 0, i),
                  source(usx, lsy, 0, i), source(usx, usy, 0, i);
        float val = (Vector2f(1 - sx, sx).transpose()*values)*Vector2f(1 - sy, sy);

        dest(dx, dy, 0, i) = clip<int>(round(val), 0, 255);
    }
}

rgbImage applyHomography(rgbImage& original, rgbImage& poster, Matrix3f h, bool bilinear ){
    rgbImage output(original);

    Vector3f p(0,0,1);

    for (int y = 0; y < output.height(); y++){
        p(1) = y;
        for (int x = 0; x < output.width(); x++){
            float nx, ny;
            p(0) = x;
            Vector3f values = h*p;
            nx = values(0)/values(2);
            ny = values(1)/values(2);
            if (nx >= 0 && nx < poster.width() &&
                ny >= 0 && ny < poster.height()){
                if (!bilinear)
                    copyPixel(poster, round(nx), round(ny), output, x, y);
                else
                    copyPixelBilinear(poster, round(nx), round(ny), output, x, y);
            }
        }
    }
    return output;
}

