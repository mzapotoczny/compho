#include<sys/stat.h>
#include<cstdio>
#include<string>
#include<vector>
#include<tuple>
#include<string>
#include<cmath>
#include<OpenEXR/ImfRgba.h>
#include<OpenEXR/ImfRgbaFile.h>
#include<algorithm>
#include"common.h"
#include"gamma.h"

using namespace Imf;

const float max_eps = 0.01;

std::vector<std::string> files;
std::vector<float> times;

//std::vector<std::string> files = {
    //"/home/michal/studia/comph/2/Lab3/2_hdr/1200px-StLouisArchMultExpEV-4.72.jpeg",
    //"/home/michal/studia/comph/2/Lab3/2_hdr/1200px-StLouisArchMultExpEV-1.82.jpeg",
    //"/home/michal/studia/comph/2/Lab3/2_hdr/1200px-StLouisArchMultExpEV+1.51.jpeg",
    //"/home/michal/studia/comph/2/Lab3/2_hdr/1200px-StLouisArchMultExpEV+4.09.jpeg"
//};
//std::vector<float> times = {1.0/15.0, 0.5, 4, 15};

//std::vector<std::string> files = {
    //"/home/michal/studia/comph/2/Lab3/2_hdr/Memorial_SourceImages/memorial0061.png",
    //"/home/michal/studia/comph/2/Lab3/2_hdr/Memorial_SourceImages/memorial0062.png",
    //"/home/michal/studia/comph/2/Lab3/2_hdr/Memorial_SourceImages/memorial0063.png",
    //"/home/michal/studia/comph/2/Lab3/2_hdr/Memorial_SourceImages/memorial0064.png",
    //"/home/michal/studia/comph/2/Lab3/2_hdr/Memorial_SourceImages/memorial0065.png",
    //"/home/michal/studia/comph/2/Lab3/2_hdr/Memorial_SourceImages/memorial0066.png",
    //"/home/michal/studia/comph/2/Lab3/2_hdr/Memorial_SourceImages/memorial0067.png",
    //"/home/michal/studia/comph/2/Lab3/2_hdr/Memorial_SourceImages/memorial0068.png",
    //"/home/michal/studia/comph/2/Lab3/2_hdr/Memorial_SourceImages/memorial0069.png",
    //"/home/michal/studia/comph/2/Lab3/2_hdr/Memorial_SourceImages/memorial0070.png",
    //"/home/michal/studia/comph/2/Lab3/2_hdr/Memorial_SourceImages/memorial0071.png",
    //"/home/michal/studia/comph/2/Lab3/2_hdr/Memorial_SourceImages/memorial0072.png",
    //"/home/michal/studia/comph/2/Lab3/2_hdr/Memorial_SourceImages/memorial0073.png",
    //"/home/michal/studia/comph/2/Lab3/2_hdr/Memorial_SourceImages/memorial0074.png",
    //"/home/michal/studia/comph/2/Lab3/2_hdr/Memorial_SourceImages/memorial0075.png",
    //"/home/michal/studia/comph/2/Lab3/2_hdr/Memorial_SourceImages/memorial0076.png"
//};
//std::vector<float> times = {
                            //1.0/0.03125,
                            //1.0/0.0625 ,
                            //1.0/0.125  ,
                            //1.0/0.25   ,
                            //1.0/0.5    ,
                            //1.0/1      ,
                            //1.0/2      ,
                            //1.0/4      ,
                            //1.0/8      ,
                            //1.0/16     ,
                            //1.0/32     ,
                            //1.0/64     ,
                            //1.0/128    ,
                            //1.0/256    ,
                            //1.0/512    ,
                            //1.0/1024   
                            //};

std::vector<rgbImage*> images;
std::vector<std::vector<std::tuple<int,int,int>>> histogram(256);

inline bool file_exists(const std::string& name) {
    struct stat buffer;   
    return (stat (name.c_str(), &buffer) == 0); 
}

float w(float ij) {
    return exp(-4.0 * pow((ij - 127.5), 2)/(pow(127.5,2)));
}

int get_img_value(const rgbImage& img, int x, int y) {
    return 0.21*img.atXY(x, y, 0, 0) + 0.72*img.atXY(x, y, 0, 1) + 0.07*img.atXY(x, y, 0, 2);
}

float get_x(int x, int y, int c, std::vector<float>& I) {
    float num = 0, denom = 0;
    for (size_t i = 0; i < images.size(); i++){
        unsigned char val;
        if (c >= 0)
            val = images[i]->atXY(x, y, 0, c);
        else
            val = get_img_value(*images[i], x, y);
        num += w(val)*times[i]*I[val];
        denom += w(val)*times[i]*times[i];
    }
    return num/denom;
}

int main(int argc, char** argv){

    if (argc != 2)
    {
        printf("Wrong number of parameters\n");
        exit(1);
    }
    FILE* f = fopen(argv[1], "r");
    int n;
    fscanf(f, "%d", &n);
    for (int i = 0; i < n; i++){
        char buf[255]; float e;
        fscanf(f, "%s %f", buf, &e);
        files.push_back(buf);
        times.push_back(e);
    }
    fclose(f);

    for (auto& fname : files){
        images.push_back(new rgbImage(fname.c_str()));
        apply_gamma(*images[images.size()-1], 2.2);
    }

    for (size_t i = 0; i < images.size(); i++){
        for (int y = 0; y < images[i]->height(); y++){
            for (int x = 0; x < images[i]->width(); x++){
                int val = get_img_value(*images[i], x, y);
                histogram[val].push_back(
                        std::make_tuple(i, x, y));
            }
        }
    }

    int width = images[0]->width();
    int height = images[0]->height();

    std::vector<float> I;
    std::vector<float> xi;
    for (int i = 0; i < 255; i++)
        I.push_back(i/128.0);
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            xi.push_back(get_x(x, y, -1, I));
    std::vector<float> I_next(I);
    std::vector<float> xi_next(xi);

    for (int l = 0; l < 50; l++){
        float eps = 0;
        for (int i = 0; i < 255; i++){
            if (histogram[i].size() > 0){
                float sum = 0;
                for (size_t j = 0; j < histogram[i].size(); j++){
                    int t,x,y;
                    std::tie(t,x,y) = histogram[i][j];
                    sum += times[t]*xi[AT(x,y,width)];
                }
                I_next[i] = sum/histogram[i].size();
            }else{
                I_next[i] = 0;
            }
        }
        float in128 = I_next[128];
        for (int i = 0; i < 255; i++){
            I_next[i] /= in128;
            if (std::abs(I_next[i] - I[i]) > eps)
                eps = std::abs(I_next[i] - I[i]);
        }

        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                xi_next[AT(x,y,width)] = get_x(x, y, -1, I_next);

        std::swap(I, I_next);
        std::swap(xi, xi_next);
        printf("End of iteration %d, eps = %f\n", l, eps);
        if (eps < max_eps)
            break;
    }
    CImg<float> resultingImage(width, height, 1, 3);
    for (int channel = 0; channel < 3; channel++){
        for (int y = 0; y < height; y++){
            for (int x = 0; x < width; x++){
                resultingImage.atXY(x, y, 0, channel) = get_x(x, y, channel, I);
            }
        }
    }
    resultingImage.normalize(0.0, 1.0);
    Rgba* pixels = new Rgba[width*height];
    for (int x = 0; x < width; x++){
        for (int y = 0; y < height; y++){
            Rgba color; color.a = 1.0;
            color.r = resultingImage.atXY(x,y,0,0);
            color.g = resultingImage.atXY(x,y,0,1);
            color.b = resultingImage.atXY(x,y,0,2);
            pixels[AT(x,y,width)] = color;
        }
    }
    RgbaOutputFile file ("./out.exr", width, height, WRITE_RGBA);
    file.setFrameBuffer (pixels, 1, width);
    file.writePixels (height);
    delete[] pixels;
}

