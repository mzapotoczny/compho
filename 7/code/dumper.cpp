#include"dumper.h"
#include<cstdlib>

void dumpImage(const rgbImage& im, std::string prefix, int n, bool normalize){
    std::string path = "/tmp/depth/";
    if (prefix.length() > 0){
        path += prefix + "/";
        std::string cmd = "mkdir -p "+path;
        system(cmd.c_str());
    }

    std::string out_name = path+prefix+std::to_string(n)+".bmp";
    rgbImage out(im);
    if (normalize)
        out.normalize(0,255);
    else{
        if (im.max() <= 1)
            out *= 255.0;
        clipImage<float>(out, 0, 255);
    }
    out.save_bmp(out_name.c_str());
}

void dumpHeightmap(const rgbImage& im, std::string name, int n){
    rgbImage img(im.width(), im.height(), 1, 3);
    float min = im.min();
    float max = im.max();
    cimg_forXY(img, x, y){
        img(x,y,0) = (im(x,y,0)-min)/max*255.0;
        img(x,y,1) = 0;
        img(x,y,2) = 255 - img(x,y,0);
        //img(x,y,0) = (im(x,y,0)-min)/max*359.0;
        //img(x,y,1) = img(x,y,2) = 50.0;
    }
    //img.HSLtoRGB();
    dumpImage(img, name, n);
}

