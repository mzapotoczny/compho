#include"Eigen/Dense"
#include"common.h"
#include<iostream>
#include<vector>
#include"apply_homography.h"

using namespace Eigen;
using namespace std;

Matrix3f computeHomography(const vector<point>& correspondencePointsFirst,
                           const vector<point>& correspondencePointsSecond){
    MatrixXf A;
    A.resize(correspondencePointsFirst.size()*2, 9);

    for (size_t i = 0; i < correspondencePointsFirst.size(); i++){
        int n = 2*i;
        float x  = correspondencePointsFirst[i].first,
              y  = correspondencePointsFirst[i].second,
              xP = correspondencePointsSecond[i].first,
              yP = correspondencePointsSecond[i].second;
        A(n, 0) = x;
        A(n, 1) = y;
        A(n, 2) = 1;
        A(n, 3) = A(n, 4) = A(n, 5) = 0;
        A(n, 6) = -x * xP;
        A(n, 7) = -y * xP;
        A(n, 8) =    - xP;

        A(n+1, 0) = A(n+1, 1) = A(n+1, 2) = 0;
        A(n+1, 3) = x;
        A(n+1, 4) = y;
        A(n+1, 5) = 1;
        A(n+1, 6) = -x * yP;
        A(n+1, 7) = -y * yP;
        A(n+1, 8) =    - yP;
    }

    auto svd = A.jacobiSvd(ComputeFullU | ComputeFullV);

    auto V = svd.matrixV().col(8);
    Matrix3f h;
    int i = 0;
    for (int y = 0; y < 3; y++){
        for (int x = 0; x < 3; x++){
            h(y, x) = V(i++);
        }
    }
    return h;
}

std::pair<point, point> getDimensions(const Matrix3f& homography, rgbImage& first, rgbImage& second) {
    int lowX = 0, lowY = 0, highX = first.width(), highY = first.height();

    auto hInv = homography.inverse();
    Vector3f p(0,0,1);
    for (int x = 0; x <= second.width(); x += second.width()){
        p(0) = x;
        for (int y = 0; y <= second.height(); y += second.height()){
            p(1) = y;
            int nx, ny;
            Vector3f values = hInv*p;
            nx = values(0)/values(2);
            ny = values(1)/values(2);

            lowX = std::min(lowX, nx);
            lowY = std::min(lowY, ny);
            highX = std::max(highX, nx);
            highY = std::max(highY, ny);
        }
    }
    return make_pair(make_pair(lowX, lowY), make_pair(highX, highY));
}

int main(int argc, char** argv) {
    if (argc != 2){
        printf("USAGE: %s OUTPUT_FILE < INPUT_FILE\n", argv[0]);
        exit(1);
    }
    std::vector<point> correspondencePointsFirst ;
    std::vector<point> correspondencePointsSecond ;

    char buff[255];
    scanf("%s", buff);
    rgbImage first(buff);
    scanf("%s", buff);
    rgbImage second(buff);
    int n ;
    scanf("%d", &n);
    int x,y;
    for (int i = 0; i < n; i++){
        scanf("%d %d", &x, &y);
        correspondencePointsFirst.push_back(std::make_pair(x,y));
    }
    for (int i = 0; i < n; i++){
        scanf("%d %d", &x, &y);
        correspondencePointsSecond.push_back(std::make_pair(x,y));
    }

    auto h = computeHomography(correspondencePointsFirst, correspondencePointsSecond);
    auto newDims = getDimensions(h, first, second);

    rgbImage canvas(newDims.second.first - newDims.first.first,
                    newDims.second.second - newDims.first.second,
                    1,
                    3);
    canvas.fill(0);
    Vector2f translation(std::abs(newDims.first.first), std::abs(newDims.first.second));
    Matrix3f translationMatrix;
    translationMatrix << 1, 0, -translation(0),
                         0, 1, -translation(1),
                         0, 0, 1;
    h *= translationMatrix;
    canvas.draw_image(translation(0), translation(1), 0, 0, first);

    auto out = applyHomography(canvas, second, h, false);
    out.save_png(argv[1]);
    CImgDisplay dspl(out, "img");

    while (!dspl.is_closed()) {}

    return 0;
}

