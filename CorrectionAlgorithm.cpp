#include "CorrectionAlgorithm.h"

//**********************************************
// Function: gamma亮度矫正
GammaCorrection::GammaCorrection(Mat src, float gamma = 1):src_(src), gamma_(gamma) { }

GammaCorrection::~GammaCorrection() { }

Mat GammaCorrection::Process() {
    float k_factor = 1 / gamma_;

    unsigned char LUT[256];
    for (int i = 0; i <= 255; i++) {
        float pixel_f = (i + 0.5) / 256;
        pixel_f = float(pow(pixel_f, k_factor));
        LUT[i] = saturate_cast<uchar> (pixel_f * 256 - 0.5f);
    }

    Mat dst = src_.clone();
    if (src_.channels() == 1) {
        MatIterator_<uchar> iter = dst.begin<uchar>();
        MatIterator_<uchar> iter_end = dst.end<uchar>();
        for (; iter != iter_end; iter++) {
            *iter = LUT[(*iter)];
        }

    } else {
        MatIterator_<Vec3b> iter = dst.begin<Vec3b>();
        MatIterator_<Vec3b> iter_end = dst.end<Vec3b>();
        for (; iter != iter_end; iter++) {
            (*iter)[0] = LUT[(*iter)[0]];
            (*iter)[1] = LUT[(*iter)[1]];
            (*iter)[2] = LUT[(*iter)[2]];
        }
    }
    return dst;
}

//**********************************************
// Function: 自动颜色均衡

AutoColorEqual::AutoColorEqual(Mat src, int ratio = 4, int radius = 7):src_(src), ratio_(ratio), radius_(radius) {}

AutoColorEqual::~AutoColorEqual() { }

Mat AutoColorEqual::StretchInterRes(Mat src) {
    int row = src.rows;
    int col = src.cols;
    Mat dst(row, col, CV_64FC1);
    double max_value = 0.0;
    double min_value = 255.0;
    //找中间结果中最大、最小像素值
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            max_value = max(max_value, src.at<double>(i, j));
            min_value = min(min_value, src.at<double>(i, j));
        }
    }
    //根据最大最小值拉伸
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            dst.at<double>(i, j) = (1.0 * src.at<double>(i, j) - min_value) / (max_value - min_value);
            // 防止溢出
            if(dst.at<double>(i, j) > 1.0) dst.at<double>(i, j) = 1.0;
            if(dst.at<double>(i, j) < 0) dst.at<double>(i, j) = 0;
        }
    }
    return dst;
}

// 计算半径为radius正方形，各点到中心点距离的权重
Mat AutoColorEqual::GetWeightMap(int radius) {
    int size = radius * 2 + 1;
    Mat dst(size, size, CV_64FC1);
    //各点到中心点欧式距离的倒数
    for (int i = -radius; i <= radius; i++) {
        for(int j = -radius; j <= radius; j++) {
            if (i == 0 && j == 0)
                dst.at<double>(i + radius, j + radius) = 0;
            else
                dst.at<double>(i + radius, j + radius) = 1.0 / sqrt(i * i + j * j);
        }
    }
    //倒数之和
    double sum = 0;
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j ++) {
            sum += dst.at<double>(i, j);
        }
    }
    //radius的距离权重矩阵
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j ++) {
            dst.at<double>(i, j) = dst.at<double>(i, j) / sum;
        }
    }
    return dst;
}

Mat AutoColorEqual::NormalACE(Mat src, int ratio, int radius) {
    Mat weight_map = GetWeightMap(radius);
    int row = src.rows;
    int col = src.cols;
    int size = 2 * radius + 1;
    //原图像放中间，外面radius一周补0
    Mat Z(row + 2 * radius, col + 2 * radius, CV_64FC1);
    for (int i = 0; i < Z.rows; i++) {
        for (int j = 0; j < Z.cols; j++) {
            if((i - radius >= 0) && (i - radius < row) && (j - radius >= 0) && (j - radius < col))
                Z.at<double>(i, j) = src.at<double>(i - radius, j - radius);
            else
                Z.at<double>(i, j) = 0;
        }
    }

    //初始化返回值
    Mat dst(row, col, CV_64FC1);
    for (int i = 0; i < row; i ++) {
        for (int j = 0; j < col; j++) {
            dst.at<double>(i, j) = 0.f;
        }
    }
    //对src每个点，每次计算radius中一个分量
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j ++) {
            if (weight_map.at<double>(i, j) == 0) continue;
            for (int x = 0; x < row; x++) {
                for (int y = 0; y < col; y++) {
                    double sub = src.at<double>(x, y) - Z.at<double>(x + i, y + i);
                    double tmp = sub * ratio;
                    if (tmp > 1.0) tmp = 1.0;
                    if (tmp < -1.0) tmp = -1.0;
                    dst.at<double>(x, y) += tmp * weight_map.at<double>(i, j);
                }
            }
        }
    }

    return dst;
}

Mat AutoColorEqual::FastACE(Mat src, int ratio, int radius) {
    int row = src.rows;
    int col = src.cols;
    //当行、列小于等于2时，直接返回0.5
    if (min(row, col) <= 2) {
        Mat dst(row, col, CV_64FC1);
        for (int i = 0; i < row; i++) {
            for (int j = 0; j < col; j++) {
                dst.at<double>(i, j) = 0.5;
            }
        }
        return dst;
    }

    //缩小为原图1/2
    Mat r_s((row + 1) / 2, (col + 1) / 2, CV_64FC1);
    resize(src, r_s, Size((col + 1) / 2, (row + 1) / 2));
    //递归处理
    Mat r_f = FastACE(r_s, ratio, radius);
    //处理结果图放大2倍
    resize(r_f, r_f, Size(col, row));
    //缩小图放大2倍
    resize(r_s, r_s, Size(col, row));
    //对缩小后的图以及缩小图缩小再放大后的图
    Mat dst(row, col, CV_64FC1);
    //处理原图
    Mat dst1 = NormalACE(src, ratio, radius);
    //处理把原图缩小后的直接放大的图
    Mat dst2 = NormalACE(r_s, ratio, radius);
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            dst.at<double>(i, j) = r_f.at<double>(i, j) + dst1.at<double>(i, j) - dst2.at<double>(i, j);
        }
    }
    return dst;
}


Mat AutoColorEqual::Process() {
    int row = src_.rows;
    int col = src_.cols;

    Mat src_c1(row, col, CV_64FC1);
    Mat src_c2(row, col, CV_64FC1);
    Mat src_c3(row, col, CV_64FC1);
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            src_c1.at<double>(i, j) = 1.0 * src_.at<Vec3b>(i, j)[0] / 255.0;
            src_c2.at<double>(i, j) = 1.0 * src_.at<Vec3b>(i, j)[1] / 255.0;
            src_c3.at<double>(i, j) = 1.0 * src_.at<Vec3b>(i, j)[2] / 255.0;
        }
    }

    src_c1 = StretchInterRes(FastACE(src_c1, ratio_, radius_));
    src_c2 = StretchInterRes(FastACE(src_c2, ratio_, radius_));
    src_c3 = StretchInterRes(FastACE(src_c3, ratio_, radius_));

    Mat dst1(row, col, CV_8UC1);
    Mat dst2(row, col, CV_8UC1);
    Mat dst3(row, col, CV_8UC1);

    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            dst1.at<uchar>(i, j) = (int)(src_c1.at<double>(i, j) * 255);
            if (dst1.at<uchar>(i, j) > 255) dst1.at<uchar>(i, j) = 255;
            else if (dst1.at<uchar>(i, j) < 0) dst1.at<uchar>(i, j) = 0;

            dst2.at<uchar>(i, j) = (int)(src_c2.at<double>(i, j) * 255);
            if (dst2.at<uchar>(i, j) > 255) dst2.at<uchar>(i, j) = 255;
            else if (dst2.at<uchar>(i, j) < 0) dst2.at<uchar>(i, j) = 0;

            dst3.at<uchar>(i, j) = (int)(src_c3.at<double>(i, j) * 255);
            if (dst3.at<uchar>(i, j) > 255) dst3.at<uchar>(i, j) = 255;
            else if (dst3.at<uchar>(i, j) < 0) dst3.at<uchar>(i, j) = 0;
        }
    }

    vector<Mat> out;
    out.push_back(dst1);
    out.push_back(dst2);
    out.push_back(dst3);
    Mat dst;
    merge(out, dst);
    return dst;

}

//**********************************************
// Function: GrayWorld白平衡

GrayWorld::GrayWorld(Mat src):src_(src) { }

GrayWorld::~GrayWorld() {}

Mat GrayWorld::Process() {
    double B = 0, G = 0, R = 0;
    int row = src_.rows;
    int col = src_.cols;

    Mat dst(row, col, CV_8UC3);
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            B += 1.0 * src_.at<Vec3b>(i, j)[0];
            G += 1.0 * src_.at<Vec3b>(i, j)[1];
            R += 1.0 * src_.at<Vec3b>(i, j)[2];
        }
    }

    B /= (row * col);
    G /= (row * col);
    R /= (row * col);
    double gray_value = (B + G + R) / 3;
    double kr = gray_value / R;
    double kg = gray_value / G;
    double kb = gray_value / B;

    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            dst.at<Vec3b>(i, j)[0] = (int)(kb * src_.at<Vec3b>(i, j)[0]) > 255 ? 255 : (int)(kb * src_.at<Vec3b>(i, j)[0]);
            dst.at<Vec3b>(i, j)[1] = (int)(kg * src_.at<Vec3b>(i, j)[1]) > 255 ? 255 : (int)(kg * src_.at<Vec3b>(i, j)[1]);
            dst.at<Vec3b>(i, j)[2] = (int)(kr * src_.at<Vec3b>(i, j)[2]) > 255 ? 255 : (int)(kr * src_.at<Vec3b>(i, j)[2]);
        }
     }

    return dst;
}

//**********************************************
// Function: 完美反射理论 白平衡

PerfectReflection::PerfectReflection(Mat src, float ratio = 10):src_(src), ratio_(ratio) { }

PerfectReflection::~PerfectReflection() { }

Mat PerfectReflection::Process() {
    int row = src_.rows;
    int col = src_.cols;
    Mat dst(row, col, CV_8UC3);

    int rgbsum_hist[766] = {0};
    // 这里不直接用255而是统计R G B中最大像素值，原因：本来曝光低图像不能直接调的特别亮
    int max_val = 0;
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            max_val = max(max_val, (int)src_.at<Vec3b>(i, j)[0]);
            max_val = max(max_val, (int)src_.at<Vec3b>(i, j)[1]);
            max_val = max(max_val, (int)src_.at<Vec3b>(i, j)[2]);
            int sum = src_.at<Vec3b>(i, j)[0] + src_.at<Vec3b>(i, j)[1] + src_.at<Vec3b>(i, j)[2];
            rgbsum_hist[sum]++;
        }
    }

    // 计算最亮部分从哪个像素值开始
    int threshold = 0;
    int pixels_count = 0;
    for (int i = 755; i >= 0; i--) {
        pixels_count += rgbsum_hist[i];
        if (pixels_count > row * col * ratio_) {
            threshold = i;
            break;
        }
    }

    // 计算最亮区域中r g b分量的均值（本来是一个最亮点的r g b值，由于用到了一个区域，所以取该区域的均值）
    int avgb = 0, avgg = 0, avgr = 0;
    int cnt = 0;
    for (int i = 0; i < row; i ++) {
        for (int j = 0; j < col; j++) {
            int sum_tmp = src_.at<Vec3b>(i, j)[0] + src_.at<Vec3b>(i, j)[1] + src_.at<Vec3b>(i, j)[2];
            if (sum_tmp > threshold) {
                avgb += src_.at<Vec3b>(i, j)[0];
                avgg += src_.at<Vec3b>(i, j)[1];
                avgr += src_.at<Vec3b>(i, j)[2];
                cnt++;
            }
        }
    }
    avgb /= cnt;
    avgg /= cnt;
    avgr /= cnt;
    // 最亮点调整到max_val，其余像素点也按此比例调整
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            int blue = src_.at<Vec3b>(i, j)[0] * max_val / avgb;
            int green = src_.at<Vec3b>(i, j)[1] * max_val / avgg;
            int red = src_.at<Vec3b>(i, j)[2] * max_val / avgr;
            if (blue > 255) blue = 255;
            else if (blue < 0) blue = 0;
            if (green > 255) green = 255;
            else if (green < 0) green = 0;
            if (red > 255) red = 255;
            else if (red < 0) red = 0;

            dst.at<Vec3b>(i, j)[0] = blue;
            dst.at<Vec3b>(i, j)[1] = green;
            dst.at<Vec3b>(i, j)[2] = red;
        }
    }

    return dst;
}


//**********************************************
// Function: 直方图均衡化
HistogramEqual::HistogramEqual (Mat src):src_(src) { }

HistogramEqual::~HistogramEqual () {}


Mat HistogramEqual::Process() {
    if (src_.channels() == 3) {
        int R[256] = {0};
        int G[256] = {0};
        int B[256] = {0};

        int row = src_.rows;
        int col = src_.cols;
        int total = row * col;

        for (int i = 0; i < row; i++) {
            for (int j = 0; j < col; j++) {
                B[src_.at<Vec3b>(i, j)[0]]++;
                G[src_.at<Vec3b>(i, j)[1]]++;
                R[src_.at<Vec3b>(i, j)[2]]++;
            }
        }

        double val[3] = {0};
        // 计算对应转换关系
        for (int i = 0; i < 256; i++) {
            val[0] += B[i];
            val[1] += G[i];
            val[2] += R[i];
            B[i] = val[0] * 255 / total;
            G[i] = val[1] * 255 / total;
            R[i] = val[2] * 255 / total;
        }
        // 转换
        Mat dst(row, col, CV_8UC3);
        for (int i = 0; i < row; i++) {
            for (int j = 0; j < col; j++) {
                dst.at<Vec3b>(i, j)[0] = B[src_.at<Vec3b>(i, j)[0]];
                dst.at<Vec3b>(i, j)[1] = G[src_.at<Vec3b>(i, j)[1]];
                dst.at<Vec3b>(i, j)[2] = R[src_.at<Vec3b>(i, j)[2]];
            }
        }
        return dst;
    } else if (src_.channels() == 1) {
        int Gray[256] = {0};

        int row = src_.rows;
        int col = src_.cols;
        int total = row * col;

        for (int i = 0; i < row; i++) {
            for (int j = 0; j < col; j++) {
                Gray[src_.at<uchar>(i, j)]++;
            }
        }

        double val = 0;
        // 计算对应转换关系
        for (int i = 0; i < 256; i++) {
            val += Gray[i];
            Gray[i] = val * 255 / total;
        }
        // 转换
        Mat dst(row, col, CV_8UC1);
        for (int i = 0; i < row; i++) {
            for (int j = 0; j < col; j++) {
                dst.at<uchar>(i, j) = Gray[src_.at<uchar>(i, j)];
            }
        }
        return dst;
    }

}
































