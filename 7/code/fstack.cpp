#include"common.h"
#include"lightfield.hpp"
#include<string>


rgbImage refocus(const rgbImageList& list, float alpha) {
    int listSize = sqrt(list.size());
    int x0 = listSize/2;
    int y0 = listSize/2;
    rgbImage output(list[0]);
    output.fill(0);
    for (int y = 0; y < listSize; y++){
        for (int x = 0; x < listSize; x++){
            int index = y*listSize + x;
            float dv = (x - x0)*alpha;
            float du = (y - y0)*alpha;

            cimg_forXYC(output, _x, _y, _c){
                output(_x,_y,_c) += list[index].linear_atXYZC(_x + dv, _y + du, 1, _c);
            }
        }
    }
    output /= list.size();
    return output;
}

int main() {
    rgbImageList list = loadLightField("../../data/9x9/frog.jpg", 9);

    int j = 0;
    float from = -1.25, to = 1.25, step = 0.1;
    for (float i = from; i <= to; i += step){
        j++;
        auto outLine = refocus(list, i);
        std::string fname = "/tmp/lfs/refocus/" + std::to_string(j) + ".bmp";
        outLine.save(fname.c_str());
        printf("%d/%d\n", j, (int)((to - from)/step) + 1);
    }

    return 0;
}
