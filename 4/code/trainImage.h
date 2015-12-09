#include"common.h"
#include<vector>
#include<stdint.h>

#pragma once
#include<armadillo>

using namespace arma;

struct trainImagesData{
    uint32_t count;
    uint32_t width;
    uint32_t height;
    uint32_t spectrum;
    unsigned char* data = nullptr;

    static trainImagesData set(const std::vector<rgbImage*>& in);
    static trainImagesData load(const char* fname);

    void dump(const char* fname);

    ~trainImagesData();

    double getMeanOfComponent(int componentId) const;
};

struct trainImagesPCA {
    uint32_t count;
    uint32_t imageSize;
    uint32_t eigenCount;
    double*  mean;
    double*  eigenVectors;
    double*  data;

    static trainImagesPCA set(const trainImagesData& data, int pcCount);
    static trainImagesPCA load(const char* fname);
    double* transformImage(const unsigned char* data, double* mem = nullptr) const;

    void dump(const char* fname);

    ~trainImagesPCA();

    private:
    void generateEigenVectors(const trainImagesData& data);
};
