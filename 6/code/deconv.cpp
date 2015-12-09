#include"common.h"
#include<matio.h>
#include<vector>
#include<assert.h>
#include<stdexcept>
#include<armadillo>
#include<cstdlib>
#include"fft.hpp"
#include"conv.hpp"

Convolver* convo;

enum Direction { Right, Down, Left };

rgbImage imageFromMat(const arma::mat& mat){
    rgbImage img(mat.n_cols, mat.n_rows);
    cimg_forXY(img, x, y){
        img(x, y, 0) = mat(y, x);
    }
    return img;
}

std::vector<arma::mat> getFilter(const char* filename) {
    std::vector<arma::mat> out;
    mat_t *matfp;
    matvar_t *matvar;
    matvar_t *cell;
    matfp = Mat_Open(filename, MAT_ACC_RDONLY);

    if ( NULL == matfp ) {
        throw std::runtime_error(std::string("Error opening MAT file") + std::string(filename));
    }

    matvar = Mat_VarRead(matfp, "filts");
    assert( matvar != NULL);
    assert( matvar->rank == 2 );
    assert( matvar->dims[0] == 1 );
    assert( matvar->data_type == MAT_T_CELL );
    for (unsigned int i = 0; i < matvar->dims[1]; i++){
        cell = Mat_VarGetCell(matvar, i);
        assert( cell->rank == 2 );
        assert( cell->data_type == MAT_T_DOUBLE );
        double* data = (double*)cell->data;
        arma::mat mat(cell->dims[0], cell->dims[1]);
        for (size_t c = 0; c < cell->dims[1]; c++){
            for (size_t r = 0; r < cell->dims[0]; r++){
                mat(r, c) = data[c*cell->dims[0] + r];
            }
        }
        out.push_back(mat);
    }
    Mat_Close(matfp);
    return out;
}

void dumpImage(const rgbImage& im, std::string prefix, int n, bool normalize = false){
    std::string path = "/tmp/deconvolved/";
    if (prefix.length() > 0){
        path += prefix + "/";
        std::string cmd = "mkdir -p "+path;
        system(cmd.c_str());
    }

    std::string out_name = path+prefix+to_string(n)+".bmp";
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

void   recomputeSum(const rgbImage& error,
                    double& currentSum,
                    int radius,
                    int x, int y,
                    Direction direction ){
    if (direction != Down){
        int leftMultiplier = -1;
        int nxL = clip<>(x - radius - 1, 0, error.width() - 1);
        int nxR = clip<>(x + radius, 0, error.width() - 1);
        if (direction == Left){
            leftMultiplier = 1;
            nxL = clip<>(x - radius, 0, error.width() - 1);
            nxR = clip<>(x + radius + 1, 0, error.width() - 1);
        }
        for (int ry = -radius; ry <= radius; ry++){
            int ny  = clip<>(y + ry, 0, error.height() - 1);

            currentSum += leftMultiplier*pow(error(nxL, ny, 0), 2) + (-leftMultiplier)*pow(error(nxR, ny, 0), 2);
        }
    }else{
        int nyU = clip<>(y - radius - 1, 0, error.height() - 1);
        int nyD = clip<>(y + radius, 0, error.height() - 1);
        for (int rx = -radius; rx <= radius; rx++){
            int nx  = clip<>(x + rx, 0, error.width() - 1);

            currentSum += -1*pow(error(nx, nyU, 0), 2) + pow(error(nx, nyD, 0), 2);
        }
    }
}

rgbImage computeSumInWindows(const rgbImage& error, const int radius){
    rgbImage error_value(error);
    Direction dir = Right;
    double sum = 0;

    // compute sum
    for (int rx = -radius; rx <= radius; rx++){
        for (int ry = -radius; ry <= radius; ry++){
            int nx = clip<int>(rx, 0, error.width() - 1);
            int ny = clip<int>(ry, 0, error.height() - 1);
            sum += pow(error(nx, ny, 0), 2);
        }
    }

    for (int y = 0; y < error.height(); y++){
        int xs;
        if (dir == Right){
            xs  = 0;
        }else{
            xs = error.width() - 1;
        }

        int df = (dir == Right) ? 1 : -1;

        //for (int x = xs; (dir == Right) ? x < error.width() : x >= 0; (dir == Right) ? x++ : x--){
        for (int x = xs; x != error.width() - xs - 1 + df; x += df){
            if (x != xs)
                recomputeSum(error, sum, radius, x, y, dir);
            error_value(x,y) = sum;
        }

        if (y + 1 < error.height()){
            recomputeSum(error, sum, radius, error.width() - xs - 1, y+1, Down);
        }

        if (dir == Right)
            dir = Left;
        else
            dir = Right;
    }

    return error_value;
}

std::pair<rgbImage, rgbImage> estimateDepth(const rgbImage& input, const rgbImageList& deconvolved, const int radius = 20){
    rgbImage out_image(input), out_depthest(input);

    rgbImageList errorsList;
    for (unsigned int f = 0; f < deconvolved.size(); f++){
        auto conv = convo->convolve(deconvolved[f], f);
        //auto conv = deconvolved[f].get_convolve(imageFromMat(allFilters[f][4]));
        auto error = input - conv;
        dumpImage(conv, "reconvolved", f, false);
        dumpImage(error, "inputMINUSreconvolved", f, true);

        rgbImage error_value = computeSumInWindows(error, radius);
        dumpImage(error_value, "errorEstimates", f, true);

        errorsList.push_back(error_value);
        printf("Depth estimation at lvl %d\n", f+1);
    }

    std::vector<int> stats(deconvolved.size(), 0);

    cimg_forXY(input, x, y){
        int argmin = 1;
        for (unsigned int f = 2; f < deconvolved.size(); f++){
            if (errorsList[argmin](x,y,0) - errorsList[argmin-1](x,y,0) < errorsList[f](x,y,0) - errorsList[f-1](x,y,0)){
                argmin = f;
            }
        }
        out_depthest(x,y,0) = argmin;
    }

    cimg_forXY(input, x, y){
        int argmin = out_depthest(x,y,0);
        stats[argmin]++;
        out_image(x,y,0) = deconvolved[argmin](x,y,0);
    }

    for (unsigned int i = 0; i < stats.size(); i++)
        printf("%d -> %d\n", i, stats[i]);

    out_depthest.normalize(0.0, 1.0);

    return std::make_pair(out_image, out_depthest);
}


int main(int argc, char** argv){

    if (argc != 4){
        printf("Wrong number of parameters.\n");
        printf("USAGE: %s INPUT_FILE DECONVOLUTION_WEIGHT DEPTH_ESTIMATION_WINDOW\n", argv[0]);
        printf("Try: %s ../../CodedApertureData/cups_board_inp.png 0.05 20\n", argv[0]);
        return 1;
    }
    const char* inputFile = argv[1];
    double weight = std::stod(argv[2]);
    int window = std::stoi(argv[3]);

    rgbImage img(inputFile);
    rgbImage originalYCbCr(img);
    if (img.spectrum() == 3){
        //img.sRGBtoRGB();
        img.RGBtoYCbCr();
        originalYCbCr = img;
        img.resize(img.width(), img.height(), 1, 1);
    }
    img /= 255.0;
    printf("Image: %dx%d\n", img.width(), img.height());

    std::vector<std::vector<arma::mat>> all_filters;
    rgbImageList outputs;

    for (int i = 1; i <= 9; i++){
        std::string num = std::to_string(i);
        std::string mat_file = std::string("/home/michal/studia/comph/6/CodedApertureData/filts/filt_scl0") + num + ".mat";
        auto filters = getFilter(mat_file.c_str());
        filters = {filters[filters.size()/2]};
        all_filters.push_back(filters);
    }

    printf("Now will FFT image, filters...\n"); 
    fflush(stdout);
    convo = new Convolver(img, all_filters, weight);
    printf("DONE\n");


    for (int i = 1; i <= 9; i++){
        rgbImage output = convo->deconvolve(i-1);
        dumpImage(output, "deconvolved", i-1);

        outputs.push_back(output);
        printf("Deconvolved with filter %d/9\n", i);
    }

    auto pair = estimateDepth(img, outputs, window);

    pair.first *= 255.0;
    dumpImage(pair.first, "output", 0);
    dumpHeightmap(pair.second, "output", 1);

    if (originalYCbCr.spectrum() == 3){
        cimg_forXY(pair.first, x, y){
            originalYCbCr(x, y, 0) = pair.first(x,y,0);
        }
        originalYCbCr.YCbCrtoRGB();
        dumpImage(originalYCbCr, "output", 3);
    }

    return 0;
}
