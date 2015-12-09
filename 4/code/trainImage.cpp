#include"trainImage.h"


trainImagesData trainImagesData::set(const std::vector<rgbImage*>& in) {
    trainImagesData data;
    data.count = in.size();
    data.width = in[1]->width();
    data.height = in[1]->height();
    data.spectrum = in[1]->spectrum();

    uint32_t img_size = data.width*data.height*data.spectrum;
    data.data = (uint8_t*)malloc(data.count*img_size);

    for (uint32_t i = 0; i < data.count; i++){
        memcpy(data.data+i*img_size, in[i]->data(), img_size);
    }

    return data;
}

void trainImagesData::dump(const char* fname) {
    FILE* f;
    f = fopen(fname, "wb");

    fwrite(&count, sizeof(count), 1, f);
    fwrite(&width, sizeof(width), 1, f);
    fwrite(&height, sizeof(height), 1, f);
    fwrite(&spectrum, sizeof(spectrum), 1, f);
    fwrite(data, width*height*spectrum*count, 1, f);

    fclose(f);
}

trainImagesData trainImagesData::load(const char* fname) {
    trainImagesData data;
    FILE* f;
    f = fopen(fname, "rb");

    fread(&data.count, sizeof(count), 1, f);
    fread(&data.width, sizeof(width), 1, f);
    fread(&data.height, sizeof(height), 1, f);
    fread(&data.spectrum, sizeof(spectrum), 1, f);
    uint32_t img_size = data.width*data.height*data.spectrum;
    data.data = (unsigned char*)malloc(data.count*img_size);
    fread(data.data, img_size*data.count, 1, f);

    fclose(f);
    return data;
}

trainImagesData::~trainImagesData() {
    if (data != nullptr)
        free(data);
}

double trainImagesData::getMeanOfComponent(int componentId) const{
    uint64_t sum = 0;
    auto imgSize = spectrum*width*height;
    for (unsigned int c = componentId; c < count*imgSize; c += imgSize){
        sum += data[c];
    }
    return sum / double(imgSize);
}

void trainImagesPCA::generateEigenVectors(const trainImagesData& data) {
    mat pointsMatrix(imageSize, data.count);
    //std::vector<double> componentsMean(imageSize);
    mean = (double*)malloc(sizeof(double)*imageSize);
    for (unsigned int c = 0; c < imageSize; c++){
        mean[c] = data.getMeanOfComponent(c);
    }
    for (int i = 0; i < (int)data.count; i++){
        for (unsigned int c = 0; c < imageSize; c++){
            pointsMatrix(c, i) = data.data[i*imageSize + c] - mean[c];
        }
    }

    //mat eig;
    //vec s;
    //mat V;

    //svd(eig, s, V, pointsMatrix);

    //std::cout << eig << std::endl;
    //std::cout << s << std::endl;

    vec eigval;
    mat eig;
    eig_sym(eigval, eig, pointsMatrix*pointsMatrix.t());

    //std::cout << eig << std::endl;
    //std::cout << eigval << std::endl;

    eigenVectors = (double*) malloc(sizeof(double)*imageSize*eigenCount);
    printf("%d\n", imageSize);
    for (unsigned int j = 0; j < eigenCount; j++){
        for (unsigned int i = 0; i < imageSize; i++){
            eigenVectors[j*imageSize + i] = eig(i, j);
        }
    }
}


trainImagesPCA trainImagesPCA::set(const trainImagesData& data, int pcCount) {
    trainImagesPCA pca;
    pca.count = data.count;
    pca.imageSize = data.spectrum * data.width * data.height;
    pca.eigenCount = pcCount;
    pca.generateEigenVectors(data);
    pca.data = (double*)malloc(sizeof(double)*pca.eigenCount*pca.count);

    for (unsigned int i = 0; i < data.count; i++){
        pca.transformImage(data.data + i*pca.imageSize, pca.data + i*pca.eigenCount);
    }

    return pca;
}

double* trainImagesPCA::transformImage(const unsigned char* data, double* mem) const{
    if (mem == nullptr)
        mem = (double*)malloc(sizeof(double)*eigenCount);

    for (unsigned int c = 0; c < eigenCount; c++){
        double sum = 0;
        for (unsigned int d = 0; d < imageSize; d++){
            sum += (data[d]-mean[d])*eigenVectors[c*imageSize + d];
        }
        mem[c] = sum;
    }
    return mem;
}

trainImagesPCA trainImagesPCA::load(const char* fname) {
    trainImagesPCA data;
    FILE* f;
    f = fopen(fname, "rb");

    fread(&data.count, sizeof(count), 1, f);
    fread(&data.imageSize, sizeof(imageSize), 1, f);
    fread(&data.eigenCount, sizeof(eigenCount), 1, f);

    data.mean = (double*) malloc(sizeof(double)*data.imageSize);
    data.eigenVectors = (double*) malloc(sizeof(double)*data.imageSize*data.eigenCount);
    data.data = (double*) malloc(sizeof(double)*data.count*data.eigenCount);

    fread(data.mean, sizeof(double)*data.imageSize, 1, f);
    fread(data.eigenVectors, sizeof(double)*data.imageSize*data.eigenCount, 1, f);
    fread(data.data, sizeof(double)*data.count*data.eigenCount, 1, f);

    fclose(f);
    return data;
}

void trainImagesPCA::dump(const char* fname) {
    FILE* f;
    f = fopen(fname, "wb");

    fwrite(&count, sizeof(count), 1, f);
    fwrite(&imageSize, sizeof(imageSize), 1, f);
    fwrite(&eigenCount, sizeof(eigenCount), 1, f);
    fwrite(mean, sizeof(double)*imageSize, 1, f);
    fwrite(eigenVectors, sizeof(double)*imageSize*eigenCount, 1, f);
    fwrite(data, sizeof(double)*count*eigenCount, 1, f);

    fclose(f);
}

trainImagesPCA::~trainImagesPCA() {
    free(eigenVectors);
    free(data);
}
