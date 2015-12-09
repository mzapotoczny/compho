#include"median_filter.h"

rgbImage median_filter_naive(rgbImage& input, int w, int channel) {
    int windowHistogram[256];
    rgbImage out(input);
    int hSum;
    for (int y = 0; y < input.height(); y++){
        for (int x = 0; x < input.width(); x++){
            memset(windowHistogram, 0, sizeof(windowHistogram));
            hSum = 0;
            for (int lx = clip(x-w, 0, input.width()); lx <= clip(x+w, 0, input.width()); lx++) {
                for (int ly = clip(y-w, 0, input.height()); ly <= clip(y+w, 0, input.height()); ly++) {
                    windowHistogram[input.atXY(lx, ly, 0, channel)]++;
                    hSum++;
                }
            }
            int sum = 0, median = 255;
            for (int i = 0; i < 256; i++){
                sum += windowHistogram[i];
                if (sum >= hSum/2){
                    median = i;
                    break;
                }
            }

            out.atXY(x, y, 0, channel) = median;
        }
    }
    return out;
}

rgbImage median_filter(rgbImage& input, int w, int channel) {
    if (w == 0)
        return input;
    int windowHistogram[256];
    rgbImage out(input);

    int hSum = 0;
    for (int x = 0; x < input.width(); x++){
        hSum = 0;
        memset(windowHistogram, 0, sizeof(windowHistogram));
        for (int hx = -w; hx <= w; hx++){
            for (int hy = 0; hy <= w; hy++){
                if (hx + x < 0 || hx + x >= input.width() ||
                    hy >= input.height())
                    continue;
                windowHistogram[ input.atXY(x+hx, hy, 0, channel) ]++;
                hSum++;
            }
        }
        for (int y = 0; y < input.height(); y++){
            if (y > w){
                for (int hx = -w; hx <= w; hx++) {
                    if (hx + x < 0 || hx + x >= input.width())
                        continue;
                    windowHistogram[ input.atXY(x + hx, y-w-1, 0, channel) ]--;
                    hSum--;
                }
            }
            if (y > 0 && y < input.height() - w - 1){
                for (int hx = -w; hx <= w; hx++) {
                    if (hx + x < 0 || hx + x >= input.width())
                        continue;
                    windowHistogram[ input.atXY(x + hx, y + w, 0, channel) ]++;
                    hSum++;
                }
            }

            int sum = 0, median = 255;
            for (int i = 0; i < 256; i++){
                sum += windowHistogram[i];
                if (sum >= hSum/2){
                    median = i;
                    break;
                }
            }

            out.atXY(x, y, 0, channel) = clip<>(median, 0, 255);
        }
    }
    return out;
}

