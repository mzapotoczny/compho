#include "conv.hpp"

Convolver::Convolver(const rgbImage& image, const std::vector<std::vector<arma::mat>>& allFilters, double weight) : weight(weight) {
    fft2 = new FFT(image.width(), image.height());
    _imageFFT = imageFFT(image);

    arma::mat Gx(1,2); 
    arma::mat Gy(2,1);
    Gx(0,0) = 1.0; Gx(0,1) = -1.0;
    Gy(0,0) = 1.0; Gy(1,0) = -1.0;

    _GxFFT = fft2->computeDFT(Gx);
    _GyFFT = fft2->computeDFT(Gy);

    for (unsigned int s = 0; s < allFilters.size(); s++){
        _allFiltersFFT.push_back(std::vector<arma::cx_mat>());
        for (unsigned int d = 0; d < allFilters[s].size(); d++){
            _allFiltersFFT[s].push_back(fft2->computeDFT(allFilters[s][d]));
        }
    }
}

rgbImage Convolver::convolve(int scale_num) {
    return apply_operator(_imageFFT, _allFiltersFFT[scale_num], false);
}

rgbImage Convolver::convolve(const rgbImage& image, int scale_num) {
    //printf("Want to convolve image with spectrum %.3f %.3f, scale_num = %d\n", image.min(), image.max(), scale_num);
    auto fft = imageFFT(image);
    return apply_operator(fft, _allFiltersFFT[scale_num], false);
}

rgbImage Convolver::deconvolve(int scale_num) {
    return apply_operator(_imageFFT, _allFiltersFFT[scale_num], true);
}

rgbImage Convolver::deconvolve(const rgbImage& image, int scale_num) {
    auto fft = imageFFT(image);
    return apply_operator(fft, _allFiltersFFT[scale_num], true);
}

arma::cx_mat Convolver::imageFFT(const rgbImage& image) const {
    arma::mat imageMat(image.height(), image.width());
    cimg_forXY(image, x, y){
        imageMat(y, x) = image(x,y,0);
    }
    
    return fft2->computeDFT(imageMat);
}

arma::cx_mat Convolver::convolve_one(const arma::cx_mat& imageFFT, const arma::cx_mat& filterFFT){
    cx_mat conv = imageFFT % filterFFT;
    return fft2->computeIDFT( conv , true );
}

arma::cx_mat Convolver::deconvolve_one(const arma::cx_mat& imageFFT, const arma::cx_mat& filterFFT) {
    arma::cx_mat numer = conj(filterFFT) % imageFFT; // YES, % is element-wise * in armadillo...
    arma::cx_mat denom = conj(filterFFT) % filterFFT
               + weight*(conj(_GxFFT)%_GxFFT + conj(_GyFFT)%_GyFFT);

    arma::cx_mat dv = numer/denom;
    auto X = fft2->computeIDFT(dv);
    return X;
}

rgbImage Convolver::apply_operator(const arma::cx_mat& imageFFT, const std::vector<arma::cx_mat>& filtersFFT, bool deconvolution) {
    std::vector<cx_mat> mats;

    int width = imageFFT.n_cols;
    int fsize = filtersFFT.size();
    int elem_size = width / fsize;

    for (int f = 0; f < fsize; f++){
        if (deconvolution)
            mats.push_back(deconvolve_one(imageFFT, filtersFFT[f]));
        else
            mats.push_back(convolve_one(imageFFT, filtersFFT[f]));
    }

    rgbImage output(mats[0].n_cols, mats[0].n_rows, 1, 1);

    unsigned int basic = 0;
    for (unsigned int i = 0; i < mats.size(); i++){
        int h = ((int)i < fsize-1) ? elem_size : width - i*elem_size;

        for (unsigned int y = 0; y < mats[i].n_rows; y++){
            for (unsigned int x = basic; x < basic + h; x++){
                output(x, y, 0) = mats[i](y, x).real();
            }
        }
        basic += h;
    }
    //printf("Output normals: %.3f %.3f\n", output.min(), output.max());
    clipImage<float>(output, 0.0, 1.0);
    //output.normalize(0.0,1.0);
    return output;
}

void Convolver::set_contrast(rgbImage& img, double c) {
    cimg_forXY(img, x, y){
        img(x, y, 0) = clip<>(img(x,y,0)*c, 0.0, 1.0);
    }
}

Convolver::~Convolver() {
    delete fft2;
}
