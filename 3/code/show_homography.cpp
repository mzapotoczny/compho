#include"apply_homography.h"
#include"common.h"

using namespace Eigen;

int main() {
    Matrix3f h;
    h << 0.8025, 0.0116, -78.2148,
        -0.0058, 0.8346, -141.3292,
        -0.0006, -0.0002, 1.;

    rgbImage original("../../Lab4/green.png");
    rgbImage poster("../../Lab4/poster.png");

    auto outputN = applyHomography(original, poster, h);
    auto outputB = applyHomography(original, poster, h, true);

    CImgList<unsigned char> list(outputN, outputB);
    CImgDisplay dspl(list, "Image");

    while (!dspl.is_closed()) {}

    outputN.save_png("output_nn.png");
    outputB.save_png("output_bilinear.png");

    return 0;
}
