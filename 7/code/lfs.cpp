#include"common.h"
#include"lightfield.hpp"
#include<string>

rgbImage getLineH(const rgbImageList& list, int y0){
    int listSize = sqrt(list.size());
    rgbImage outLine(list[0].width(), listSize, 1, list[0].spectrum());
    for (int i = y0*listSize; i < y0*listSize + listSize; i++){
        int y = list[i].height() / 2;
        for (int c = 0; c < list[i].spectrum(); c++){
            for (int x = 0; x < list[i].width(); x++){
                outLine(x,i - y0*listSize,c) = list[i](x,y,c);
            }
        }
    }
    return outLine;
}

rgbImage getLineW(const rgbImageList& list, int x0){
    int listSize = sqrt(list.size());
    rgbImage outLine(listSize, list[0].height(), 1, list[0].spectrum());
    for (int i = x0*listSize; i < x0*listSize + listSize; i++){
        int x = list[i].height() / 2;
        for (int c = 0; c < list[i].spectrum(); c++){
            for (int y = 0; y < list[i].height(); y++){
                outLine(i - x0*listSize,y,c) = list[i](x,y,c);
            }
        }
    }
    return outLine;
}

int main() {
    rgbImageList list = loadLightField("../../data/9x9/matrioska.jpg", 9);
    //for (int i = 9*4; i < 9*5; i++){
        //std::string fname = "/tmp/lfs/"+std::to_string(i - 9*4) + ".bmp";
        //list[i].save(fname.c_str());
    //}
    auto outLine1 = getLineH(list, 4);
    auto outLine2 = getLineW(list, 4);
    rgbImageList listOut ({outLine1, outLine2});
    
    CImgDisplay dspl(listOut);
    while(!dspl.is_closed()) { dspl.wait(); }
    return 0;
}
