#include"gamma.h"
#include"common.h"

void apply_gamma(rgbImage& source, float gamma) {
    for (int y = 0; y < source.height(); y++){
        for (int x = 0; x < source.width(); x++){
            for (int channel = 0; channel < source.spectrum(); channel++){
                source.atXY(x, y, 0, channel) = clip<float>(
                        round(pow(source.atXY(x, y, 0, channel)/255.0, gamma)*255.0),
                        0.0f,
                        255.0f);
            }
        }
    }
}
