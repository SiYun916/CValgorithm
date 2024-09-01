#include "AlignmentAlgorithm.h"

ECCAlignment::ECCAlignment(string ref_path, string ori_path):ref_image_path_(ref_path), ori_image_path_(ori_path) {};
ECCAlignment::~ECCAlignment() { }

// Function:
// 返回值:基于ref配准后的ori图
Mat ECCAlignment::ECCProcess(int warp_mode, int iterations, double terminations_eps) {
    Mat ref_image = imread(ref_image_path_);
    Mat ori_image = imread(ori_image_path_);

    Mat ref_gray, ori_gray;
    cvtColor(ref_image, ref_gray, CV_BGR2GRAY);
    cvtColor(ori_image, ori_gray, CV_BGR2GRAY);

    Mat warp_matrix;
    if (warp_mode == MOTION_HOMOGRAPHY)
        warp_matrix = Mat::eye(3, 3, CV_32F);
    else
        warp_matrix = Mat::eye(2, 3, CV_32F);

    TermCriteria criteria (TermCriteria::COUNT+TermCriteria::EPS, iterations, terminations_eps);

    findTransformECC(
                ref_gray,
                ori_gray,
                warp_matrix,
                warp_mode,
                criteria
                );

    Mat ori_aligned;
    if (warp_mode != MOTION_HOMOGRAPHY)
        warpAffine(ori_image, ori_aligned, warp_matrix, ori_image.size(), INTER_LINEAR + WARP_INVERSE_MAP);
    else
        warpPerspective(ori_image, ori_aligned, warp_matrix, ori_image.size(), INTER_LINEAR + WARP_INVERSE_MAP);

    return ori_aligned;

}
