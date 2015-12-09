#include"sobel.h"

const int sobel_x[3][3] = 
    { {-1, 0, 1},
      {-2, 0, 2},
      {-1, 0, 1} };

const int sobel_y[3][3] = 
    { {-1, -2, -1},
      { 0,  0,  0},
      { 1,  2,  1} };

std::vector<int> get_gradient(rgbImage& inputImage) {
    std::vector<int> out(inputImage.width() * inputImage.height(), 0);

    int width = inputImage.width();
    int height = inputImage.height();
    for (int y = 0; y < inputImage.height(); y++){
        for (int x = 0; x < inputImage.width(); x++){
            int val_x = 0, val_y = 0;
            for (int wx = -1; wx <= 1; wx++){
                for (int wy = -1; wy <= 1; wy++){
                    int img_val = inputImage.atXY(
                            clip<>(x + wx, 0, width),
                            clip<>(y + wy, 0, height),
                            0,0);
                    val_x +=
                        sobel_x[wx+1][wy+1]*img_val;
                    val_y +=
                        sobel_y[wx+1][wy+1]*img_val;
                }
            }
            out[AT(x,y,width)] = sqrt(val_x*val_x + val_y*val_y);
        }
    }
    return out;
}
