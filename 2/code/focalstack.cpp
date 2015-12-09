#include<sys/stat.h>
#include<cstdio>
#include<string>
#include<vector>
#include"common.h"
#include"sobel.h"

inline bool file_exists(const std::string& name) {
    struct stat buffer;   
    return (stat (name.c_str(), &buffer) == 0); 
}

int main(int argc, char** argv) {
    std::vector<rgbImage*> images;
    std::vector<std::vector<int>> gradients;
    for (int i = 1; i < argc; i++){
        if (file_exists(argv[i])){
            images.push_back(new rgbImage(argv[i]));
            if (images[images.size()-1]->width()  != images[0]->width() ||
                images[images.size()-1]->height() != images[0]->height())
            {
                printf("Images must have the same dimensions!\n");
                exit(1);
            }
            gradients.push_back(get_gradient(*images[images.size()-1]));
        }else{
            printf("File %s does not exist\n", argv[i]);
            exit(1);
        }
    }
    if (images.size() == 0){
        printf("No images\n");
        exit(1);
    }

    const int width = images[0]->width();
    const int height = images[0]->height();

    rgbImage output(width, height, 1, 1);
    rgbImage outputCC(width, height, 1, 3);

    for (int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){
            int max = 0, max_id = 0;
            for (size_t i = 0; i < gradients.size(); i++)
                if (gradients[i][AT(x,y,width)] > max) {
                    max_id = i;
                    max = gradients[i][AT(x,y,width)];
                }
            outputCC.atXY(x, y, 0, max_id) = images[max_id]->atXY(x,y,0,0);
            output.atXY(x, y, 0, 0) = images[max_id]->atXY(x, y, 0, 0);
        }
    }
    outputCC.save_png("output_cc.png");
    output.save_png("output.png");

    return 0;
}
