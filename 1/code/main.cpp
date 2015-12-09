#include"common.h"
#include"demosaicing.h"
#include"gamma.h"
#include"colorspace.h"
#include"median_filter.h"
#include"flags.hh"

#include<ctime>
#include<string>
#include<unistd.h>

inline bool file_exists(const std::string& name) {
    struct stat buffer;   
    return (stat (name.c_str(), &buffer) == 0); 
}

int main(int argc, char** argv){

    Flags flags;
    unsigned int median_filter_size;
    float gamma_correction;
    std::string demosaicing_mode;
    std::string file_name;
    bool help;
    bool save;

    flags.Var(file_name, 'f', "file-name", std::string(""), "File in which we want to do magic");
    flags.Var(median_filter_size, 'm', "median-size", (unsigned int)(9), "Size of the kernel of median filter (*2 + 1)");
    flags.Var(gamma_correction, 'g', "gamma-correction", float(2.2), "Gamma correction");
    flags.Var(demosaicing_mode, 'd', "demosaicing-mode", std::string("RGGB"), "Set which pixel codes what rgb value (must be 4 values from {r,g,b}).");
    flags.Bol(help, 'h', "help", "Show help");
    flags.Bol(save, 's', "save", "Save results as filename-{1,2,3}.jpg");

    if (!flags.Parse(argc, argv)) {
        flags.PrintHelp(argv[0]);
        return 1;
    } else if (help) {
        flags.PrintHelp(argv[0]);
        return 0;
    } else if (!file_exists(file_name)) {
        fprintf(stderr, "ERROR: File %s does not exist\n", file_name.c_str());
        flags.PrintHelp(argv[0]);
        return 1;
    }

    if (demosaicing_mode.size() == 4){
        char mode[4];
        for (int i = 0; i < 4; i++)
            mode[i] = demosaicing_mode[i];
        if (!set_mode(mode)){
            fprintf(stderr, "ERROR: Coould not set %s demosaicing mode\n", demosaicing_mode.c_str());
            flags.PrintHelp(argv[0]);
            return 1;
        }
    }

    const clock_t begin_time = clock();

    CImg<unsigned char> image(file_name.c_str());

    auto simplest = demosaicing_linear(image);
    apply_gamma(simplest, 1/gamma_correction);

    auto better = demosaicing_edge(image);
    apply_gamma(better, 1/gamma_correction);
    
    auto best = demosaicing_edge(image);
    convert_colorspace(best, rgb_to_ycbcr);
    best = median_filter(best, median_filter_size, 1);
    best = median_filter(best, median_filter_size, 2);
    convert_colorspace(best, ycbcr_to_rgb);
    apply_gamma(best, 1/gamma_correction);

    printf("Time: %fs\n", float( clock () - begin_time ) /  CLOCKS_PER_SEC);

    if (!save){
        CImgList<unsigned char> list(simplest, better, best);
        CImgDisplay dspl(list, "Simplest, Better, Best");

        while (!dspl.is_closed()) { dspl.wait(); }
    }else{
        simplest.save_jpeg((file_name+"-1.jpg").c_str());
        better.save_jpeg((file_name+"-2.jpg").c_str());
        best.save_jpeg((file_name+"-3.jpg").c_str());
    }

    return 0;
}
