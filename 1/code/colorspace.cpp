#include"colorspace.h"
#include<cmath>

color convertToColor(float a, float b, float c) {
    int ca, cb, cc;
    ca = round(clip<>(a, 0.0f, 255.0f));
    cb = round(clip<>(b, 0.0f, 255.0f));
    cc = round(clip<>(c, 0.0f, 255.0f));
    return std::make_tuple(ca, cb, cc);
}

color rgb_to_ycbcr(color in) {
    int r,g,b;
    std::tie(r,g,b) = in;
    
    float y, cb, cr;
    y = 0.299*r + 0.587*g + 0.144*b;
    cb = 128 - 0.168736*r - 0.331264*g + 0.5*b;
    cr = 128 + 0.5*r - 0.418688*g - 0.081312*b;
    return convertToColor(y, cb, cr);
}

color ycbcr_to_rgb(color in) {
    int y, cb, cr;
    std::tie(y, cb, cr) = in;

    float r, g, b;
    r = y + 1.402*(cr - 128);
    g = y - 0.34414*(cb - 128) - 0.71414*(cr - 128);
    b = y + 1.772 * (cb - 128);
    return convertToColor(r, g, b);
}

void convert_colorspace(rgbImage& source, std::function<color(color)> converter) {
    for (int y = 0; y < source.height(); y++){
        for (int x = 0; x < source.width(); x++){
            color inColor = std::make_tuple(
                    source.atXY(x, y, 0, 0),
                    source.atXY(x, y, 0, 1),
                    source.atXY(x, y, 0, 2)
                    );
            std::tie(source.atXY(x, y, 0, 0),
                     source.atXY(x, y, 0, 1),
                     source.atXY(x, y, 0, 2)) = converter(inColor);
        }
    }
}

