#include "demosaicing.h"
#include<vector>

using namespace std;

BasicColors mosaic[2][2] = { {R, G}, {G, B} };

inline BasicColors getType(int x, int y) { return mosaic[x%2][y%2]; }

inline void tryAddPoint(const rgbImage& img, vector<point>& dest, int x, int y) {
    if (x >= 0 && x < img.width() && y >= 0 && y < img.height())
        dest.push_back(make_pair(x,y));
}

vector<point> getNeighboors8(const rgbImage& img, int x, int y, BasicColors color) {
    vector<point> neighboors;

    for (int ny = -1; ny <= 1; ny++) {
        for (int nx = -1; nx <= 1; nx++) {
            if (nx == 0 && ny == 0)
                continue;
            if (nx + x < 0 || ny + y < 0)
                continue;
            if (getType(x+nx, y+ny) == color)
                tryAddPoint(img, neighboors, x+nx, y+ny);
        }
    }
    return neighboors;
}

bool set_mode(const char mode[4]) {
    BasicColors colors[4];
    for (int i = 0; i < 4; i++){
        BasicColors currentColor;
        switch(mode[i]){
            case 'r':
            case 'R':
                currentColor = R;
                break;
            case 'g':
            case 'G':
                currentColor = G;
                break;
            case 'b':
            case 'B':
                currentColor = B;
                break;
            default:
                return false;
        }
        colors[i] = currentColor;
    }
    for (int i = 0; i < 4; i++)
        mosaic[i/2][i%2] = colors[i];
    return true;
}

rgbImage demosaicing_linear(const rgbImage& source){

    rgbImage image(source.width(), source.height(), 1, 3);

    for (int y = 0; y < image.height(); y++){
        for (int x = 0; x < image.width(); x++) {
            for (int channel = R; channel <= B; channel++){
                BasicColors truColor = static_cast<BasicColors>(channel);
                int value = 0;
                if (getType(x, y) == truColor){
                    value = source.atXY(x, y, 0, 0);
                } else {
                    auto neigh = getNeighboors8(source, x, y, truColor);
                    if (neigh.size() == 0)
                        continue;

                    for (point p : neigh){
                        value += source.atXY(p.first, p.second, 0, 0);
                    }
                    value /= neigh.size();
                }

                image.atXY(x, y, 0, channel) = clip<>(value, 0, 255);
            }
        }
    }
    return image;
}

rgbImage demosaicing_edge(const rgbImage& source) {
    rgbImage image(source.width(), source.height(), 1, 3);
    image.fill(0);

    for (int y = 0; y < image.height(); y++){
        for (int x = 0; x < image.width(); x++) {
            float value = 0;
            if (getType(x,y) == G){
                value = source.atXY(x, y, 0, 0);
            }else{
                auto neigh_points = getNeighboors8(source, x, y, G);
                vector<int> neigh;
                for (point p : neigh_points){
                    neigh.push_back(source.atXY(p.first, p.second, 0, 0));
                }
                if (neigh.size() != 4){ // for edges
                    for (int p : neigh){
                        value += p;
                    }
                    value /= 1.0*neigh.size();
                }else{
                    int dv = abs(neigh[0] - neigh[3]);
                    int dh = abs(neigh[1] - neigh[2]);
                    if (dh > dv) {
                        value = (neigh[0] + neigh[3])/2.0;
                    }else if (dh < dv){
                        value = (neigh[1] + neigh[2])/2.0;
                    }else{
                        value = (neigh[0] + neigh[1] + neigh[2] + neigh[3])/4.0;
                    }
                }
            }
            image.atXY(x, y, 0, G) = clip<>(value, 0.0f, 255.0f);
        }
    }

    for (int y = 0; y < image.height(); y++){
        for (int x = 0; x < image.width(); x++) {
            for (int channel = R; channel <= B; channel++){
                if (channel == G)
                    continue;
                BasicColors truColor = static_cast<BasicColors>(channel);

                float value = 0;
                if (getType(x, y) == truColor){
                    value = source.atXY(x, y, 0, 0);
                }else{
                    auto neigh = getNeighboors8(source, x, y, truColor);
                    for (point p : neigh){
                        value += source.atXY(p.first, p.second, 0, 0) -
                                 image.atXY(p.first, p.second, 0, G);
                    }
                    value /= 1.0*neigh.size();
                    value += image.atXY(x, y, 0, G);
                    if (value >  255){
                        value = 255;
                    }
                    else if (value < 0){
                        value = 0;
                    }
                }
                image.atXY(x, y, 0, channel) = clip<>(value, 0.0f, 255.0f);
            }
        }
    }
    return image;
}
