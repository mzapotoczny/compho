#pragma once

#include<complex>
#include<vector>
#include<fftw3.h>
#include"common.h"
#include<armadillo>

using namespace std;
using namespace arma;

class FFT {
    private:
        double norm;
        fftw_plan plan, plan_inverse;
        fftw_complex* inArray;
        fftw_complex* outArray;

        cx_mat returnOutArray(bool normalize);
    public:
        int width, height;

        FFT() = delete;
        FFT(const FFT& other) = delete;
        FFT& operator=(const FFT& other) = delete;

        FFT(int width, int height);
        ~FFT();
        cx_mat computeDFT(const mat& input, bool chessBoard = false);
        cx_mat computeIDFT(const cx_mat& source, bool normalize = true);
};
