#include<cstdio>
#include<vector>
#include<string>
#include"common.h"

void timesMask(rgbImage& in, const rgbImage& mask) {
    cimg_forXYC(in, x, y, c){
        in(x,y,c) *= mask(x,y);
    }
}

rgbImage naiveComposite(const rgbImage& bg,
                        const rgbImage& fg,
                        const rgbImage& mask,
                        int y, int x){
    rgbImage out(bg);
    cimg_forXY(mask, _x, _y){
        if (mask(_x,_y) != 0){
            for (int c = 0; c < out.spectrum(); c++)
                out(x + _x, y + _y, c) = fg(_x, _y, std::min(c, fg.spectrum()));
        }
    }
    return out;
}

rgbImage paste(const rgbImage& bg,
               const rgbImage& fg,
               int y, int x){
    rgbImage out(bg);
    cimg_forXY(fg, _x, _y){
        for (int c = 0; c < out.spectrum(); c++)
            out(x + _x, y + _y, c) = fg(_x, _y, std::min(c, fg.spectrum()));
    }
    return out;
}

rgbImageList Poisson(const rgbImage& bg,
                     const rgbImage& fg,
                     const rgbImage& mask,
                     const std::vector<int> niter, int y0, int x0) {
    rgbImageList out;
    rgbImage b = fg.get_laplacian();
    rgbImage x (fg);
    rgbImage r;

    cimg_forXYC(fg, _x, _y, c){
        x(_x, _y, c) = (-1*(mask(_x,_y)-1))*bg(x0+_x,y0+_y,c);
    }

    int niter_cur = 0;

    for (int i = 0; i <= niter[niter.size()-1]; i++){
        r = b - x.get_laplacian();
        timesMask(r, mask);
        float alpha = r.dot(r) / r.dot(r.get_laplacian());

        x += alpha*r;

        if (i == niter[niter_cur]){
            out.push_back(x);
            niter_cur++;
        }
    }
    return out;
}

rgbImageList PoissonCG(const rgbImage& bg,
                       const rgbImage& fg,
                       const rgbImage& mask,
                       const std::vector<int> niter, int y0, int x0) {
    rgbImageList out;
    rgbImage b = fg.get_laplacian();
    rgbImage x (fg);
    rgbImage r, d;

    cimg_forXYC(fg, _x, _y, c){
        x(_x, _y, c) = (-1*(mask(_x,_y)-1))*bg(x0+_x,y0+_y,c);
    }

    r = b - x.get_laplacian();
    timesMask(r, mask);
    d = r;

    int niter_cur = 0;

    for (int i = 0; i <= niter[niter.size()-1]; i++){
        if (i > 0){
            auto d_laplacian = d.get_laplacian();
            float alpha = r.dot(r) / d.dot(d_laplacian);
            x += alpha*d;

            rgbImage r_new = r - alpha*d_laplacian;
            timesMask(r_new, mask);

            float beta = r_new.dot(r_new) / r.dot(r);
            r = r_new;
            d = r + beta*d;
        }

        if (i == niter[niter_cur]){
            out.push_back(x);
            niter_cur++;
        }
    }
    return out;
}

rgbImageList PoissonMomentum(const rgbImage& bg,
                             const rgbImage& fg,
                             const rgbImage& mask,
                             const std::vector<int> niter, int y0, int x0, const float momentum = 0.8) {
    rgbImageList out;
    rgbImage b = fg.get_laplacian();
    rgbImage x (fg);
    rgbImage r;
    rgbImage velocity(fg); velocity.fill(0);

    cimg_forXYC(fg, _x, _y, c){
        x(_x, _y, c) = (-1*(mask(_x,_y)-1))*bg(x0+_x,y0+_y,c);
    }

    int niter_cur = 0;

    for (int i = 0; i <= niter[niter.size()-1]; i++){
        r = b - x.get_laplacian();
        timesMask(r, mask);
        float alpha = r.dot(r) / r.dot(r.get_laplacian());

        velocity = momentum*velocity + alpha*r;
        x += velocity;

        if (i == niter[niter_cur]){
            out.push_back(x);
            niter_cur++;
        }
    }
    return out;
}

void dumpList(const std::string prefix, std::vector<int> nums, rgbImageList& list){
    for (unsigned int i = 0; i < list.size(); i++){
        clipImage<float>(list[i], 0, 255);
        std::string fname = prefix + std::to_string(nums[i]) + ".png";
        list[i].save_png(fname.c_str());
    }
}

void gdExample() {
    rgbImage bg = rgbImage("../../data/ramp.png");
    rgbImage fg = rgbImage("../../data/fg.png");
    rgbImage mask = rgbImage("../../data/mask3.png");

    mask /= 255.0;

    std::vector<int> iters = {0,50,100,200};

    rgbImageList lst = Poisson(bg, fg, mask, iters, 0, 0);

    dumpList("/tmp/gd_", iters, lst);
}

void cgdExample() {
    rgbImage bg = rgbImage("../../data/ramp.png");
    rgbImage fg = rgbImage("../../data/fg.png");
    rgbImage mask = rgbImage("../../data/mask3.png");

    mask /= 255.0;
    
    std::vector<int> iters = {0,5,10,20,50, 200, 1000};

    rgbImageList lst = PoissonCG(bg, fg, mask, iters, 0, 0);

    dumpList("/tmp/cgd_", iters, lst);
}

void mgdExample() {
    rgbImage bg = rgbImage("../../data/ramp.png");
    rgbImage fg = rgbImage("../../data/fg.png");
    rgbImage mask = rgbImage("../../data/mask3.png");

    mask /= 255.0;

    std::vector<int> iters = {0,5,10,20,50,100,200};

    rgbImageList lst = PoissonMomentum(bg, fg, mask, iters, 0, 0);

    dumpList("/tmp/mgd_", iters, lst);
}

void prepareMask(rgbImage& img){
    cimg_forXYC(img, x, y, c){
        img(x, y, c) = (img(x,y,c) == 0) ? 0.0 : 1.0;
    }
}

int main(int argc, char** argv){
    if (argc != 9 && argc != 2){
        printf("USAGE: %s BG_IMAGE FG_IMAGE MASK X0 Y0 ALGORITHM[1,2,3,4] IterationsNum OUT_NAME\n", argv[0]);
        return 1;
    }else if (argc == 2){
        gdExample();
        mgdExample();
        cgdExample();
        return 0;
    }
    const char* bg_path = argv[1];
    const char* fg_path = argv[2];
    const char* mask_path = argv[3];
    const char* out_path = argv[8];
    int x0 = std::stoi(argv[4]);
    int y0 = std::stoi(argv[5]);
    int algo = std::stoi(argv[6]);
    int iterationsNum = std::stoi(argv[7]);

    rgbImage bg = rgbImage(bg_path);
    rgbImage fg = rgbImage(fg_path);
    rgbImage mask = rgbImage(mask_path);
    mask /= 255.0;
    prepareMask(mask);

    rgbImageList lst;
    rgbImage last;

    switch (algo){
        case 2:
            lst = Poisson(bg, fg, mask, {iterationsNum}, y0, x0);
            last = paste(bg, lst[lst.size()-1], y0, x0);
            break;
        case 3:
            lst = PoissonCG(bg, fg, mask, {iterationsNum}, y0, x0);
            last = paste(bg, lst[lst.size()-1], y0, x0);
            break;
        case 4:
            lst = PoissonMomentum(bg, fg, mask, {iterationsNum}, y0, x0);
            last = paste(bg, lst[lst.size()-1], y0, x0);
            break;
        default:
            last = naiveComposite(bg, fg, mask, y0, x0);
            break;
    }

    //last.normalize(0, 255);
    clipImage<float>(last, 0, 255);

    last.save_png(out_path);
    CImgDisplay dspl(last);
    while (!dspl.is_closed()) { dspl.wait(); }

    return 0;
}
