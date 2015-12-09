#include"common.h"
#include"trainImage.h"
#include<vector>
#include<string>
#include<assert.h>
#include<algorithm>

template<typename T>
std::vector<int> getNearestData(const T* inputData, unsigned int inputDataCount, const T* imageData, unsigned int imageDataSize) {
    std::vector<std::pair<unsigned int, double>> diffs;
    int translation = 0;
    for (unsigned int testCase = 0; testCase < inputDataCount; testCase++){
        uint64_t sum = 0;
        int diff = 0;
        for (unsigned int i = 0; i < imageDataSize; i++){
            diff = inputData[translation + i] - imageData[i];
            sum += diff*diff;
        }
        diffs.push_back(std::make_pair(testCase, sqrt(sum)));
        translation += imageDataSize;
    }

    std::sort( diffs.begin(),
               diffs.end(),
               [](const std::pair<unsigned int, double>& a, const std::pair<unsigned int, double>& b){
                   return a.second < b.second;
               } );

    std::vector<int> nearestImages(inputDataCount);
    for (unsigned int i = 0; i < inputDataCount; i++)
        nearestImages[i] = diffs[i].first;
    return nearestImages;
}

std::vector<int> getNearestImgesPCA(const trainImagesPCA& data, const rgbImage& input) {
    assert(input.width()*input.height()*input.spectrum() == data.imageSize);
    auto input_data = input.data();
    double* input_pca = data.transformImage(input_data);

    return getNearestData<double>(data.data, data.count, input_pca, data.eigenCount);
}

std::vector<int> getNearestImges(const trainImagesData& data, const rgbImage& input) {
    assert(input.width() - data.width == 0
       && input.height() - data.height == 0
       && input.spectrum() - data.spectrum == 0);
    uint32_t image_size = data.spectrum * data.height * data.width;
    auto input_data = input.data();

    return getNearestData<unsigned char>(data.data, data.count, input_data, image_size);
}

void RGBToOther(rgbImage& image){
    image.RGBtoYCbCr();
}

void OtherToRGB(rgbImage& image){
    image.YCbCrtoRGB();
}

rgbImage transferColor(const rgbImage& originalImage,
                       const rgbImage& originalTestCase,
                       const trainImagesData& tidRGB,
                       const std::vector<int>& nearestImages,
                       const int averageCount = 5,
                       const int channelsCount = 2){
    const int channelSizeRGB = tidRGB.width*tidRGB.height;
    rgbImage testCase(originalTestCase);
    std::vector<unsigned char*> imagesColors(averageCount);

    for (int j = 0; j < averageCount; j++){
        int i = nearestImages[j] * tidRGB.spectrum*channelSizeRGB;
        rgbImage img(tidRGB.data + i, tidRGB.width, tidRGB.height, 1, tidRGB.spectrum);
        RGBToOther(img);
        imagesColors[j] = (unsigned char*)malloc(channelsCount*channelSizeRGB);
        memcpy(imagesColors[j], img.data() + (3 - channelsCount)*channelSizeRGB, channelsCount*channelSizeRGB);
    }

    unsigned char* meanColor = (unsigned char*)malloc(channelsCount*channelSizeRGB);

    for (int v = 0; v < channelsCount*channelSizeRGB; v++){
        int mean = 0;
        for (int j = 0; j < averageCount; j++){
            mean += imagesColors[j][v];
        }
        meanColor[v] = clip<unsigned char>(mean/float(averageCount), 0, 255);
    }

    memcpy(testCase.data() + (3 - channelsCount)*channelSizeRGB, meanColor, channelsCount*channelSizeRGB);
    testCase.resize(originalImage.width(), originalImage.height(), 1, 3, 6);

    rgbImage outputImage(originalImage);
    RGBToOther(outputImage);
    memcpy(outputImage.data() + (3 - channelsCount)*originalImage.width()*originalImage.height(),
           testCase.data()      + (3 - channelsCount)*originalImage.width()*originalImage.height(),
           channelsCount*originalImage.width()*originalImage.height());
    OtherToRGB(outputImage);

    free(meanColor);
    for (int j = 0; j < averageCount; j++){
        free(imagesColors[j]);
    }
    return outputImage;
}

int main(){
    auto tidY   = trainImagesData::load("../../faces/trainsetY.bin");
    auto tidRGB = trainImagesData::load("../../faces/trainsetRGB.bin");
    auto pca    =  trainImagesPCA::load("../../faces/trainsetPCA.bin");
    std::string path = "../../faces/testset/";
    std::string path_out = "../../faces/testset_out-pca/";
    for (int testImage = 1; testImage <= 100; testImage++){
        rgbImage originalImage((path+std::to_string(testImage)+".jpg").c_str());
        rgbImage originalTestCase(originalImage);
        rgbImage testCaseSSD(originalImage);

        originalImage.resize(originalImage.width(), originalImage.height(), 1, 3); // make room for colors!
        originalTestCase.resize(86, 86, 1, 3, 6);
        testCaseSSD.resize(32, 32, 1, 1);

        auto v = getNearestImgesPCA(pca, testCaseSSD);
        //auto v = getNearestImges(tidY, testCaseSSD);
        for (int count = 1; count <= 5; count++){
            std::string filename = path_out + std::to_string(testImage) + "-" + std::to_string(count) + ".png";
            rgbImage outputImage = transferColor(originalImage, originalTestCase, tidRGB, v, count, 2);
            outputImage.save_png(filename.c_str());
            printf("%s\n", filename.c_str());
        }
    }

    return 0;
}

/*
 * Time for PCA:
 * ./colortransfer  2,36s user 1,00s system 55% cpu 6,050 total
 *
 * Time for normal:
 * ./colortransfer  4,92s user 0,98s system 69% cpu 8,533 total
 *
 */
