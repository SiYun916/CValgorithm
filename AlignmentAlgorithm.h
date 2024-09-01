#ifndef ALIGNMENTALGORITHM_H
#define ALIGNMENTALGORITHM_H

#include <dirent.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <io.h>

#include <string>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace cv;
using namespace std;


class ECCAlignment {
public:

    ECCAlignment(string ref_path, string ori_path);
    ~ECCAlignment();
    Mat ECCProcess(int warp_mode = MOTION_EUCLIDEAN, int iterations = 5000, double terminations_eps = 1e-10);

private:
    string ref_image_path_, ori_image_path_, save_image_path_;
};






#endif // ALIGNMENTALGORITHM_H
