#include"common.h"
#include"trainImage.h"
#include<dirent.h>
#include<string>
#include<vector>

std::vector<std::string> lsDir(const std::string &directory)
{
    std::vector<std::string> out;
    DIR *dir;
    struct dirent *ent;
    struct stat st;

    dir = opendir(directory.c_str());
    while ((ent = readdir(dir)) != NULL) {
        const std::string file_name = ent->d_name;
        const std::string full_file_name = directory + "/" + file_name;

        if (file_name[0] == '.')
            continue;

        if (stat(full_file_name.c_str(), &st) == -1)
            continue;

        const bool is_directory = (st.st_mode & S_IFDIR) != 0;

        if (is_directory)
            continue;

        out.push_back(full_file_name);
    }
    closedir(dir);
    return out;
}

int main() {
    std::vector<rgbImage*> trainImagesOriginals;
    std::vector<rgbImage*> trainImagesGrayscale32;
    printf("Loading images ");
    fflush(stdout);
    auto files = lsDir("../../faces/trainset");
    int i = 0;
    for (auto& s : files){
        rgbImage* img = new rgbImage(s.c_str());
        img->resize(86, 86, 1, 3, 1);
        rgbImage* grayImg = new rgbImage(*img);
        trainImagesOriginals.push_back(img);
        grayImg->RGBtoYCbCr();
        grayImg->resize(32, 32, 1, 1); // Now we only have Y
        //grayImg->resize(4, 4, 1, 1); // Now we only have Y
        trainImagesGrayscale32.push_back(grayImg);
        i++;
        printf("\rLoading images %d/%d", i, (int)files.size());
    }
    printf("\rLoaded images\n");

    auto tid1 = trainImagesData::set(trainImagesOriginals);
    tid1.dump("../../faces/trainsetRGB.bin");

    auto tid2 = trainImagesData::set(trainImagesGrayscale32);
    tid2.dump("../../faces/trainsetY.bin");

    trainImagesPCA pca = trainImagesPCA::set(tid2, 19);
    //trainImagesPCA pca = trainImagesPCA::set(tid2, 5);
    pca.dump("../../faces/trainsetPCA.bin");
}
