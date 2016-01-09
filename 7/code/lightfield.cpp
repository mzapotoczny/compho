#include"lightfield.hpp"

rgbImageList loadLightField(const char* filename, int size){
    rgbImage image(filename);
    int size_w = image.width() / size;
    int size_h = image.height() / size;
    rgbImageList outList;

    for (int y = 0; y < size; y++){
        for (int x = 0; x < size; x++){
            outList.push_back(image.get_crop( x*size_w, y*size_h, (x+1)*size_w-1, (y+1)*size_h-1 ));
        }
    }
    return outList;
}
