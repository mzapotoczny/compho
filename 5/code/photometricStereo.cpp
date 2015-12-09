#include<cstdio>
#include"common.h"
#include<vector>
#include<string>
#include<tuple>
#include<armadillo>

std::vector<std::string> files = {
                                  "../../2_photometricstereo/teapot_1_1_1.png",
                                  "../../2_photometricstereo/teapot_-1_1_1.png",
                                  "../../2_photometricstereo/teapot_0_-1_1.png"
};

std::vector<std::tuple<int,int,int>> positions = {
                                                   std::make_tuple(1,  1, 1),
                                                   std::make_tuple(-1, 1, 1),
                                                   std::make_tuple(0, -1, 1)
};

int main() {
    std::vector<rgbImage*> images;
    for (auto& s : files)
        images.push_back(new rgbImage(s.c_str()));

    arma::mat matrix(images.size(), images[0]->width() * images[1]->height());

    for (unsigned int i = 0; i < images.size(); i++){
        int j = 0;
        for (int y = 0; y < images[i]->height(); y++){
            for (int x = 0; x < images[i]->width(); x++){
                matrix(i, j++) = images[i]->atXY(x, y, 0, 0)/255.0;
            }
        }
    }

    arma::mat lightDirs(positions.size(), 3);
    for (unsigned int i = 0; i < positions.size(); i++){
        lightDirs(i, 0) = std::get<0>(positions[i]);
        lightDirs(i, 1) = std::get<1>(positions[i]);
        lightDirs(i, 2) = std::get<2>(positions[i]);
    }

    arma::mat result = (lightDirs.i()*matrix).t();

    rgbImage outImage(images[0]->width(), images[0]->height(), 1, 3);
    int j = 0;
    double r,g;
    for (int y = 0; y < images[0]->height(); y++){
        for (int x = 0; x < images[0]->width(); x++){
            int current_elem = j++;
            if (result(current_elem, 2) != 0){
                r = result(current_elem, 0)/result(current_elem, 2);
                g = result(current_elem, 1)/result(current_elem, 2);
            }else{
                r = g = 0;
            }
            outImage.atXY(x, y, 0, 0) = clip<int>(r*255.0, 0, 255);
            outImage.atXY(x, y, 0, 1) = clip<int>(g*255.0, 0, 255);
            outImage.atXY(x, y, 0, 2) = 0;
        }
    }

    CImgDisplay dspl(outImage, "Mein God What a Beautiful Teapot!");
    while (!dspl.is_closed()) {}

    for (auto image : images)
        delete image;
}
