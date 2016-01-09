#include"common.h"
#include"lightfield.hpp"
#include<string>
#include"dumper.h"

rgbImage convertListTo4DGrayImage(const rgbImageList& list){
    int s = sqrt(list.size());

    rgbImage image(list[0].width(), list[0].height(), s, s);

    for (int v = 0; v < s; v++){
        for (int u = 0; u < s; u++){
            cimg_forXY(image, x, y){
                image(x, y, u, v) = 0.21*list[v*s + u](x,y,0) +
                                    0.72*list[v*s + u](x,y,1) +
                                    0.07*list[v*s + u](x,y,2);
            }
        }
    }
    return image;
}

rgbImage shear(const rgbImage& L0, float alpha){
    rgbImage output(L0);
    cimg_forXYZC(output, x, y, u, v){
        output(x, y, u, v) = L0.linear_atXYZC(x + u*(1.0 - 1.0/alpha), y + v*(1.0 - 1.0/alpha), u, v);
    }
    return output;
}

rgbImage integrate(const rgbImage& L){
    rgbImage output(L.width(), L.height(), 1, 1);
    cimg_forXYZC(L, x, y, u, v){
        output(x, y, 0, 0) += L(x, y, u, v);
    }
    output /= L.spectrum()*L.spectrum();
    return output;
}

rgbImage defo(const rgbImage& Lint, int window = 2){
    rgbImage output(Lint.width(), Lint.height(), 1, 1);

    cimg_forXY(Lint, x, y){
        float sum = 0;
        for (int i = -window; i <= window; i++){
            for (int j = -window; j <= window; j++){
                int nx = x + i;
                int ny = y + i;

                sum += Lint.atXY(nx, ny-1) + Lint.atXY(nx, ny+1) +
                       Lint.atXY(nx-1, ny) + Lint.atXY(nx+1, ny) 
                    -4*Lint.atXY(nx, ny);
            }
        }
        sum /= (2*window + 1)*(2*window + 1);
        output(x,y) = sum;
    }

    return output;
}

rgbImage corr(const rgbImage& La, const rgbImage& Lint, int window = 2){
    rgbImage output(Lint.width(), Lint.height(), 1, 1);

    cimg_forXY(Lint, x, y){
        float sum = 0;
        for (int i = -window; i <= window; i++){
            for (int j = -window; j <= window; j++){

                int nx = x + i;
                int ny = y + j;

                float variance = 0;
                for (int v = 0; v < La.spectrum(); v++){
                    for (int u = 0; u < La.spectrum(); u++){
                        variance += pow(La.atXYZC(nx, ny, u, v) - Lint.atXY(nx,ny), 2);
                    }
                }
                variance /= La.spectrum()*La.spectrum();
                variance = sqrt(variance);

                sum += variance;

            }

        }
        sum /= (2*window + 1)*(2*window + 1);
        output(x,y) = sum;
    }

    return output;
}

template<typename T>
bool aOPb(T a, T b, bool max){
    if (max)
        return a > b;
    else
        return a < b;
}

std::pair<int, int> getExtreme2(const rgbImageList& list, int x, int y, bool max = true){
    int max0 = 0, max1 = 1;

    if (aOPb<>(list[max1](x, y), list[max0](x, y), max))
        std::swap(max0, max1);

    for (unsigned int i = 2; i < list.size(); i++){
        if (aOPb<>(list[i](x, y), list[max1](x, y), max))
        {
            max1 = i;
            if (aOPb<>(list[max1](x, y), list[max0](x, y), max))
                std::swap(max0, max1);
        }
    }

    return std::make_pair(max0, max1);
}

int main() {
    rgbImageList list = loadLightField("../../data/9x9/computer.jpg", 9);

    auto L0 = convertListTo4DGrayImage(list);

    float alpha_min = 0.3, alpha_max = 3.5, alpha_step = 0.1;
    //float alpha_min = 0.2, alpha_max = 2, alpha_step = 0.007;

    int i = 0;

    rgbImageList LDa, LCa;
    for (float alpha = alpha_min; alpha <= alpha_max; alpha += alpha_step){
        i++;

        auto La = shear(L0, alpha);
        auto Laint = integrate(La);
        auto Da = defo(Laint);
        auto Ca = corr(La, Laint);

        //LLa.push_back(La);
        LDa.push_back(Da);
        LCa.push_back(Ca);

        dumpImage(Da, "defo", i);
        dumpImage(Ca, "corr", i);

        printf("%d/%d\n", i, int((alpha_max - alpha_min)/alpha_step) + 1);
    }


    rgbImage output(L0.width(), L0.height(), 1, 1);

    cimg_forXY(output, x, y){
        auto extD = getExtreme2(LDa, x, y, true);
        auto extC = getExtreme2(LCa, x, y, false);
        int index = 0;
        if (extD.first != extC.first){
            float Dconf = LDa[extD.first](x) / LDa[extD.second](x);
            float Cconf = LCa[extC.first](x) / LCa[extC.second](x);

            if (Dconf > Cconf)
                index = extD.first;
            else
                index = extC.first;
        }else
            index = extD.first;

        output(x, y) = index;
    }

    //CImgDisplay dspl(output);
    //while (!dspl.is_closed()) { dspl.wait(); }

    dumpHeightmap(output, "depth", 0);

    output.normalize(0, 255);
    output.save("../../depth.bmp");

    return 0;
}
