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

rgbImage shear(const rgbImage& L0, float alpha){
    rgbImage output(L0);
    output.fill(0);
    cimg_forXY(output, x, u){
        output(x, u) = L0.linear_atXYZC(x + u*(1.0 - 1.0/alpha), u, 0);
    }
    return output;
}

rgbImage integrate(const rgbImage& L){
    rgbImage output(L.width(), 1, 1, L.spectrum());
    cimg_forXYC(L, x, u, c){
        output(x, 0, c) += L(x, u, c);
    }
    output /= L.height();
    return output;
}

rgbImage defo(const rgbImage& Lint, int window = 1){
    rgbImage output(Lint.width(), 1, 1, Lint.spectrum());
    cimg_forXY(Lint, x, y){
        float sum = 0;
        for (int i = -window; i <= window; i++){
            int nx = x + i;
            sum += Lint.atXYZ(nx-1, y, 1) - 2*Lint.atXYZ(nx, y, 1) + Lint.atXYZ(nx+1, y, 1);
        }
        sum /= 2*window + 1;
        output(x,y) = sum;
    }

    return output;
}

rgbImage corr(const rgbImage& La, const rgbImage& Lint, int window = 1){
    rgbImage output(Lint.width(), 1, 1, Lint.spectrum());
    cimg_forXY(Lint, x, y){
        float sum = 0;
        for (int i = -window; i <= window; i++){

            float variance = 0;
            for (int u = 0; u < La.height(); u++){
                variance += pow(La(x,u) - Lint(x,0), 2);
            }
            variance /= La.height();
            variance = sqrt(variance);

            sum += variance;

        }
        sum /= 2*window + 1;
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

std::pair<int, int> getExtreme2(const rgbImageList& list, int x, bool max = true){
    int max0 = 0, max1 = 1;

    if (aOPb<>(list[max1](x), list[max0](x), max))
        std::swap(max0, max1);

    for (unsigned int i = 2; i < list.size(); i++){
        if (aOPb<>(list[i](x), list[max1](x), max))
        {
            max1 = i;
            if (aOPb<>(list[max1](x), list[max0](x), max))
                std::swap(max0, max1);
        }
    }

    return std::make_pair(max0, max1);
}

int main() {
    rgbImageList list = loadLightField("../../data/9x9/matrioska.jpg", 9);

    auto L0 = getLineH(list, 4);
    if (L0.spectrum() == 3){
        L0.RGBtoYCbCr();
        L0.resize(L0.width(), L0.height(), 1, 1);
    }

    //float alpha_min = 0.3, alpha_max = 3.5, alpha_step = 0.1;
    float alpha_min = 0.2, alpha_max = 2, alpha_step = 0.007;

    int i = 0;

    rgbImageList LLa, LDa, LCa;
    for (float alpha = alpha_min; alpha <= alpha_max; alpha += alpha_step){
        i++;

        auto La = shear(L0, alpha);
        auto Laint = integrate(La);
        auto Da = defo(Laint);
        auto Ca = corr(La, Laint);

        LLa.push_back(La);
        LDa.push_back(Da);
        LCa.push_back(Ca);

        printf("%d/%d\n", i, int((alpha_max - alpha_min)/alpha_step) + 1);
    }


    rgbImage output(L0.width(), 500, 1, 1);

    cimg_forX(output, x){
        auto extD = getExtreme2(LDa, x, true);
        auto extC = getExtreme2(LCa, x, false);
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

        for (int i = 0; i < 500; i++)
            output(x, i) = index;
    }

    CImgDisplay dspl(output);
    while (!dspl.is_closed()) { dspl.wait(); }

    return 0;
}
