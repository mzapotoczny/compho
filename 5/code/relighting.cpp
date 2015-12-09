#include"common.h"
#include<dirent.h>
#include<string>
#include<vector>
#include<iostream>
#include<sstream>
#include<algorithm>
#include<unordered_map>

const std::string data_dir = "../../1_relighting/data";
typedef std::vector<std::vector<std::vector<float>>> lpc;

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

        out.push_back(file_name);
    }
    closedir(dir);
    return out;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::pair<int,int> getAngles(const std::string& data){
    auto t = split(data, '_');
    auto s = split(t[2], '.');
    return std::make_pair(stoi(t[1]), stoi(s[0]));
}

std::pair<float,float> getAnglesStartEnd(const std::vector<float>& angles, unsigned int i){
    float diff_r = (i < angles.size()-1) ? angles[i+1] - angles[i]
                                         : angles[i]   - angles[i-1];
    float diff_l = (i > 0)               ? angles[i] - angles[i-1]
                                         : angles[i+1]   - angles[i];
    
    return std::make_pair(angles[i] - diff_l/2, angles[i] + diff_r/2);
}

lpc getLightProbeColors(const CImg<float>& lightProbe,
                        const std::vector<float>& phis,
                        const std::vector<float>& thetas,
                        const float phi_mod = 0,
                        const float theta_mod = 0,
                        const float phi_step = 1,
                        const float theta_step = 1
                        ){
    std::vector<float> lightProbeMax(lightProbe.spectrum(), 0);

    for (int c = 0; c < lightProbe.spectrum(); c++){
        for (int y = 0; y < lightProbe.height(); y++){
            for (int x = 0; x < lightProbe.width(); x++){
                lightProbeMax[c] = std::max(lightProbeMax[c], lightProbe.atXY(x, y, 0, c));
            }
        }
    }
    float lpmm = std::max({lightProbeMax[0], lightProbeMax[1], lightProbeMax[2]});

    std::vector<std::vector<std::vector<float>>> lightProbeColors
        (thetas.size(), std::vector<std::vector<float>>(phis.size(),
            std::vector<float>(lightProbe.spectrum(), 0.0)));

    for (int c = 0; c < lightProbe.spectrum(); c++){
        for (unsigned int y = 0; y < thetas.size(); y++){
            for (unsigned int x = 0; x < phis.size(); x++){
                double sum = 0;
                int count = 0;
                std::pair<float,float> phi_se = getAnglesStartEnd(phis, x);
                std::pair<float,float> theta_se = getAnglesStartEnd(thetas, y);
                //printf("%f %f %f\n", phis[x], phi_se.first, phi_se.second);

                for (double phi = phi_se.first + phi_mod; phi <= phi_se.second + phi_mod; phi += phi_step){
                    for (double theta = theta_se.first + theta_mod; theta <= theta_se.second + theta_mod; theta += theta_step){
                        count++;
                        float dx = sin(phi*M_PI/180.0)*sin(theta*M_PI/180.0);
                        float dy = cos(phi*M_PI/180.0);
                        float dz = -sin(phi*M_PI/180.0)*cos(theta*M_PI/180.0);
                        float r = (1.0/M_PI)*acos(dz)/sqrt(dx*dx + dy*dy);

                        float u = dx*r;
                        float v = dy*r;

                        float lx = lightProbe.width()/2.0;
                        float ly = lightProbe.height()/2.0;
                        lx += u*lx;
                        ly += v*ly;

                        sum += lightProbe.linear_atXY(lx, ly, 0, c)*std::abs(sin(phi*M_PI/180.0));
                    }
                }
                lightProbeColors[y][phis.size() - x - 1][c] = (count>0) ? ((sum/count)/lpmm) : 0;
            }
        }
    }
    return lightProbeColors;
}


rgbImage relightImages(const std::vector<rgbImage*>& images,
                       const std::vector<std::pair<float,float>>& angles,
                       const CImg<float>& lightProbe,
                       const lpc& lightProbeColors,
                       std::unordered_map<float, unsigned int>& phis_index,
                       std::unordered_map<float, unsigned int>& thetas_index,
                       const std::vector<std::vector<float>>& lightIntensities = {}
                       ) {
    int width = images[0]->width();
    int height = images[0]->height();
    int spectrum = images[0]->spectrum();


    CImg<unsigned char> out(width, height, 1, lightProbe.spectrum());

    printf("Progress: 0%%");
    fflush(stdout);
    int all = lightProbe.spectrum() * height * width;
    int current = 0;
    int current_last = 0;
    for (int c = 0; c < lightProbe.spectrum(); c++){
        int imageCurrentSpectrum =  std::min(spectrum-1, c);
        float* angTimes = new float[angles.size()];
        for (unsigned int a = 0; a < angles.size(); a++){
            float phi = angles[a].first;
            float theta = angles[a].second;

            int phi_i   = phis_index[phi];
            int theta_i = thetas_index[theta];

            float times = lightProbeColors[theta_i][lightProbeColors[0].size() - phi_i - 1][c];
            if (!lightIntensities.empty())
                times *= lightIntensities[a][c];
            angTimes[a] = times;
        }

        for (int y = 0; y < height; y++){
            for (int x = 0 ; x < width; x++){
                current++;
                if (current*100.0/all - current_last >= 2){
                    current_last = current*100.0/all;
                    printf("\rProgress: %d%%", current_last);
                    fflush(stdout);
                }

                double sum = 0;
                for (unsigned int a = 0; a < angles.size(); a++){
                    sum += (images[a]->atXY(x, y, 0, imageCurrentSpectrum)/255.0)*angTimes[a];
                }
                sum /= 2.0;
                out.atXY(x, y, 0, c) = clip<int>(sum * 255.0, 0, 255);
            }
        }
        delete[] angTimes;
    }
    printf("\rDone!\n");
    return out;
}

void generateTeapots() {
    std::vector<std::pair<float,float>> angles;
    std::vector<std::vector<float>> lightIntensities;
    std::vector<rgbImage*> images;
    CImg<float> lightProbe1("../../1_relighting/grace_probe.hdr");
    CImg<float> lightProbe2("../../1_relighting/rnl_probe.hdr");
    CImg<float> lightProbe3("../../1_relighting/uffizi_probe.hdr");

    auto files = lsDir(data_dir);
    for (auto file : files){
        auto angls = getAngles(file);
        lightIntensities.push_back({0.5,0.5,0.5});
        angles.push_back(angls);
        auto fullFileName = data_dir + "/" + file;
        images.push_back(new rgbImage(fullFileName.c_str()));
    }

    std::vector<float> phis;
    std::vector<float> thetas;
    std::unordered_map<float, unsigned int> phis_index;
    std::unordered_map<float, unsigned int> thetas_index;

    for (auto angle : angles){
        phis.push_back(angle.first);
        thetas.push_back(angle.second);
    }

    std::sort(phis.begin(), phis.end());
    phis.erase( std::unique( phis.begin(), phis.end() ), phis.end() );
    std::sort(thetas.begin(), thetas.end());
    thetas.erase( std::unique( thetas.begin(), thetas.end() ), thetas.end() );

    printf("Phi: %.2ff %.2ff\n", phis[0], phis[phis.size()-1]);
    printf("Theta: %.2ff %.2ff\n", thetas[0], thetas[thetas.size()-1]);

    for (unsigned int i = 0; i < phis.size(); i++){
        phis_index[phis[i]] = i;
    }
    for (unsigned int i = 0; i < thetas.size(); i++){
        thetas_index[thetas[i]] = i;
    }

    auto lpc1 = getLightProbeColors(lightProbe1, phis, thetas, 0, 0, 0.3, 0.3);
    auto lpc2 = getLightProbeColors(lightProbe2, phis, thetas, 0, 0, 0.3, 0.3);
    auto lpc3 = getLightProbeColors(lightProbe3, phis, thetas, 0, 0, 0.3, 0.3);

    rgbImage out1 = relightImages(images, angles, lightProbe1, lpc1, phis_index, thetas_index, lightIntensities);
    rgbImage out2 = relightImages(images, angles, lightProbe2, lpc2, phis_index, thetas_index, lightIntensities);
    rgbImage out3 = relightImages(images, angles, lightProbe3, lpc3, phis_index, thetas_index, lightIntensities);
    out1.save_png("/home/michal/grace.png");
    out2.save_png("/home/michal/rnl.png");
    out3.save_png("/home/michal/uffizi.png");

    //CImgDisplay dspl(out, "wat?!");
    //while(!dspl.is_closed()) {dspl.wait();}
}

void dumpLightProbe(int from, int to){
    CImg<float> lightProbe("../../1_relighting/grace_probe.hdr");


    std::vector<float> phis;
    std::vector<float> thetas;
    for (float phi = 0; phi <= 180; phi+=0.5)
        phis.push_back(phi);

    for (float theta = -180; theta <= 180; theta+=0.5)
        thetas.push_back(theta);

    for (int tht = from; tht < to; tht++){
        auto imgVec = getLightProbeColors(lightProbe, phis, thetas, 0, tht, 0.1, 0.1);
        CImg<unsigned char> img(imgVec.size(), imgVec[0].size(), 1, imgVec[0][0].size());
        for (int x = 0; x < (int)imgVec.size(); x++){
            for (int y = 0; y < (int)imgVec[0].size(); y++){
                for (int c = 0; c < (int)imgVec[0][0].size(); c++){
                    img.atXY(x, y, 0, c) = clip<int>(imgVec[x][y][c]*255, 0, 255);
                }
            }
        }

        char buff[255];
        sprintf(buff, "%.3d", tht);
        std::string out_name = "/home/michal/movie-lpr/" + std::string(buff) + ".png";
        img.save_png(out_name.c_str());
        printf("Image %d/%d\n", tht, 360);
    }

}

void gammaCorrection(rgbImage& image, const float correctionVal){
    for (int c = 0; c < image.spectrum(); c++){
        for (int y = 0; y < image.height(); y++){
            for (int x = 0; x < image.width(); x++){
                image.atXY(x,y,0,c) = clip<int>(pow(image.atXY(x,y,0,c)/255.0,correctionVal)*255.0, 0, 255);
            }
        }
    }
}

void showLightProbeColor(const lpc& lightProbeColor, const std::vector<std::pair<float,float>>& angles,
                         std::unordered_map<float, unsigned int>& phis_index,
                         std::unordered_map<float, unsigned int>& thetas_index,
                         const char* fname = nullptr){
    int width = 100, height = 100;
    rgbImage r(width, height, 1, lightProbeColor[0][0].size());
    for (int c = 0; c < r.spectrum(); c++){
        for (int y = 0; y < r.height(); y++){
            for (int x = 0; x < r.width(); x++){
                r.atXY(x,y,0,c) = 0;
            }
        }
    }

    for (int c = 0; c < r.spectrum(); c++){
        for (unsigned int a = 0; a < angles.size(); a++){
            float phi = angles[a].first;
            float theta = angles[a].second;

                        float dx = sin(phi*M_PI/180.0)*sin(theta*M_PI/180.0);
                        float dy = cos(phi*M_PI/180.0);
                        float dz = -sin(phi*M_PI/180.0)*cos(theta*M_PI/180.0);
                        float rr = (1.0/M_PI)*acos(dz)/sqrt(dx*dx + dy*dy);

                        float u = dx*rr;
                        float v = dy*rr;

                        float lx = width/2.0;
                        float ly = height/2.0;
                        lx += u*lx;
                        ly += v*ly;

                        r.atXY(ly,lx,0,c) = clip<int>(a*1.0/angles.size() * 255, 0,255);

                        //sum += lightProbe.linear_atXY(lx, ly, 0, c)*std::abs(sin(phi*M_PI/180.0));


            //int phi_i   = phis_index[phi];
            //int theta_i = thetas_index[theta];

            //float times = lightProbeColor[theta_i][phi_i][c];
            ////r.atXY(theta_i,phi_i,0,c) = clip<int>(times*255.0, 0,255);
            //r.atXY(theta_i,phi_i,0,c) = clip<int>(a*1.0/angles.size() * 255, 0,255);
            //if (!lightIntensities.empty())
                //times *= lightIntensities[a][c];
            //angTimes[a] = times;
        }
    }
    //for (int c = 0; c < r.spectrum(); c++){
        //for (int y = 0; y < r.height(); y++){
            //for (int x = 0; x < r.width(); x++){
                //r.atXY(x,y,0,c) = clip<int>(lightProbeColor[x][y][c]*255.0, 0, 255);
            //}
        //}
    //}
    if (fname != nullptr){
        r.save_png(fname);
    }else{
        CImgDisplay cdisp(r, "Light Probe COlors");
        while(!cdisp.is_closed()) { cdisp.wait(); }
    }
}

int main(int argc, char** argv) {
    //generateTeapots();
    //return 0;

    int start = 0, end = 180;

    if (argc == 3){
        start = std::stoi(argv[1]);
        end   = std::stoi(argv[2]);
    }
    //dumpLightProbe(start, end);
    //return 0;

    std::vector<std::pair<float,float>> angles;
    std::vector<std::vector<float>> lightInt;
    std::vector<rgbImage*> images;
    CImg<float> lightProbe("../../1_relighting/grace_probe.hdr");

    FILE* ldirFile = fopen("/home/michal/Pobrane/light_dir.txt", "r");
    int n;
    fscanf(ldirFile, "%d", &n);
    char buf[255];
    int a;
    float x,y,z;
    for (int i = 0; i < n; i++){
        fscanf(ldirFile, "%d %f %f %f", &a, &x, &y, &z);
        angles.push_back(std::make_pair(round(acos(z)*180.0/M_PI), round(atan2(-y, -x)*180.0/M_PI)));

        sprintf(buf, "/home/michal/Pobrane/knight_standing/knight_standing_%.3d.png", i+1);
        printf("%.2f %.2f\n", angles[angles.size()-1].first, angles[angles.size()-1].second);
        rgbImage* img = new rgbImage(buf);
        gammaCorrection(*img, 2.2);
        images.push_back(img);
    }

    FILE* lintFile = fopen("/home/michal/Pobrane/light_int.txt", "r");
    fscanf(lintFile, "%d", &n);
    for (int i = 0; i < n; i++){
        fscanf(lintFile, "%d %f %f %f", &a, &x, &y, &z);
        lightInt.push_back({x,y,z});
    }

    printf("Loaded images\n");

    std::vector<float> phis;
    std::vector<float> thetas;
    std::unordered_map<float, unsigned int> phis_index;
    std::unordered_map<float, unsigned int> thetas_index;

    for (auto angle : angles){
        phis.push_back(angle.first);
        thetas.push_back(angle.second);
    }

    std::sort(phis.begin(), phis.end());
    phis.erase( std::unique( phis.begin(), phis.end() ), phis.end() );
    std::sort(thetas.begin(), thetas.end());
    thetas.erase( std::unique( thetas.begin(), thetas.end() ), thetas.end() );

    printf("Phi: %.2ff %.2ff\n", phis[0], phis[phis.size()-1]);
    printf("Theta: %.2ff %.2ff\n", thetas[0], thetas[thetas.size()-1]);

    for (unsigned int i = 0; i < phis.size(); i++){
        phis_index[phis[i]] = i;
    }
    for (unsigned int i = 0; i < thetas.size(); i++){
        thetas_index[thetas[i]] = i;
    }

    for (int mod = start; mod <= end; mod += 1){
        printf("Generating lightProbeColors....\r");
        fflush(stdout);
        auto lightProbeColors = getLightProbeColors(lightProbe, phis, thetas, 0, mod, 0.3, 0.3);
        printf("Generated lightProbeColors phis: %d thetas: %d\n", (int)phis.size(), (int)thetas.size());

        rgbImage out = relightImages(images, angles, lightProbe, lightProbeColors, phis_index, thetas_index, lightInt);
        char buff[255];
        sprintf(buff, "%.3d", mod);
        std::string out_name = "/home/michal/movie/" + std::string(buff) + ".png";
        std::string lpr_name = "/tmp/lpr/"+std::string(buff)+".png";
        out.save_png(out_name.c_str());
        showLightProbeColor(lightProbeColors, angles, phis_index, thetas_index, lpr_name.c_str());
        printf("Image %d/%d\n", mod, end);
    }

    return 0;
}

