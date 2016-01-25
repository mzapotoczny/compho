#include"common.h"
#include<algorithm>


int main(){
    rgbImage bg = rgbImage("../../data/waterpool.png");
    rgbImage fg = rgbImage("../../data/bear.png");
    rgbImage mask = rgbImage("../../data/mask.png");

    CImgDisplay dspl(naiveComposite(bg, fg, mask, 0, 0));
    while (!dspl.is_closed()) { dspl.wait(); }
    return 0;
}
