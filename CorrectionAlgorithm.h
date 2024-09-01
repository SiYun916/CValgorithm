#ifndef CORRECTIONALGORITHM_H
#define CORRECTIONALGORITHM_H

#include <dirent.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <io.h>
#include <windows.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace cv;
using namespace std;

// 亮度矫正
class GammaCorrection {
public:
    GammaCorrection(Mat src, float gamma);
    ~GammaCorrection();
    Mat Process();
private:
    Mat src_;
    float gamma_;
};

// 色彩均衡
class AutoColorEqual {
public:
    AutoColorEqual(Mat src, int ratio, int radius);
    ~AutoColorEqual();
    Mat Process();
private:
    Mat src_;
    int ratio_, radius_;
    Mat FastACE(Mat src, int ratio, int radius);
    Mat NormalACE(Mat src, int ratio, int radius);
    Mat GetWeightMap(int radius);
    Mat StretchInterRes(Mat src);

};

// 灰度世界
class GrayWorld {
public:
    GrayWorld(Mat src);
    ~GrayWorld();
    Mat Process();
private:
    Mat src_;

};

// 完美反射理论
class PerfectReflection {
public:
    PerfectReflection(Mat src, float ratio);
    ~PerfectReflection();
    Mat Process();
private:
    Mat src_;
    float ratio_;
};

// 直方图均衡化
class HistogramEqual {
public:
    HistogramEqual(Mat src);
    ~HistogramEqual();
    Mat Process();
private:
    Mat src_;


};



#endif // CORRECTIONALGORITHM_H
