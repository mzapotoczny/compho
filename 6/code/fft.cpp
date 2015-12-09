#include "fft.hpp"

FFT::FFT(int width, int height) : width(width), height(height) {
    norm = width*height;
    inArray = (fftw_complex*)fftw_malloc(width*height*sizeof(fftw_complex));
    outArray = (fftw_complex*)fftw_malloc(width*height*sizeof(fftw_complex));

    plan =
        fftw_plan_dft_2d(height, width, inArray, outArray, FFTW_FORWARD, FFTW_ESTIMATE);
    plan_inverse =
        fftw_plan_dft_2d(height, width, inArray, outArray, FFTW_BACKWARD, FFTW_ESTIMATE);
}

cx_mat FFT::returnOutArray(bool normalize){
    cx_mat out(height, width);
    double lnorm = (normalize) ? norm : 1.0;
    for (int x = 0; x < width; x++){
        for (int y = 0; y < height; y++){
            out(y,x) = cx_double(outArray[x + width*y][0]/lnorm, outArray[x + width*y][1]/lnorm);
        }
    }
    return out;
}

cx_mat FFT::computeDFT(const mat& input, bool chessBoard) {
    for (int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){
            double pixelColor;
            if (y < (int)input.n_rows && x < (int)input.n_cols)
                pixelColor = input(y, x);
            else
                pixelColor = 0.0;

            int pow = (chessBoard && (x+y) % 2 == 0) ? -1 : 1;
            inArray[x + width*y][0] = pixelColor;
            inArray[x + width*y][0] *= pow;
            inArray[x + width*y][1] = 0;
        }
    }

    fftw_execute(plan);
    return returnOutArray(false);
}

cx_mat FFT::computeIDFT(const cx_mat& source, bool normalize){
    for (int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){
            inArray[x + width*y][0] = source(y,x).real();
            inArray[x + width*y][1] = source(y,x).imag();
        }
    }

    fftw_execute(plan_inverse);
    return returnOutArray(normalize);
}

FFT::~FFT() {
    fftw_destroy_plan(plan);
    fftw_destroy_plan(plan_inverse);

    fftw_free(inArray);
    fftw_free(outArray);
}
