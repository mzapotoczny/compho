#pragma once

#include<armadillo>
#include "common.h"
#include "fft.hpp"

class Convolver {
    public:
        Convolver(const rgbImage& image, const std::vector<std::vector<arma::mat>>& allFilters, double weight) ;

        rgbImage convolve(int scale_num); 
        rgbImage convolve(const rgbImage& image, int scale_num); 

        rgbImage deconvolve(int scale_num); 
        rgbImage deconvolve(const rgbImage& image, int scale_num); 
        void set_contrast(rgbImage& img, double c);

        double weight = 0.03;
        
        ~Convolver();
    private:
        arma::cx_mat imageFFT(const rgbImage& image) const;
        arma::cx_mat convolve_one(const arma::cx_mat& imageFFT, const arma::cx_mat& filterFFT);
        arma::cx_mat deconvolve_one(const arma::cx_mat& imageFFT, const arma::cx_mat& filterFFT);

        rgbImage apply_operator(const arma::cx_mat& imageFFT, const std::vector<arma::cx_mat>& filtersFFT, bool deconvolution = true);


        FFT* fft2;
        arma::cx_mat _imageFFT;
        arma::cx_mat _GxFFT;
        arma::cx_mat _GyFFT;
        std::vector<std::vector<arma::cx_mat>> _allFiltersFFT;

};
