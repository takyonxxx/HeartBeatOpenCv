#ifndef opencv_hpp
#define opencv_hpp

#include <stdio.h>
#include <iostream>
#ifdef __cplusplus
#import <opencv2/opencv.hpp>
#endif

namespace cv {

    const Scalar BLACK    (  0,   0,   0);
    const Scalar RED     (255,   0,   0);
    const Scalar GREEN    (  0, 255,   0);
    const Scalar BLUE      (  0,   0, 255);
    const Scalar YELLOW   (  255, 255, 0);
    const Scalar WHITE    (255, 255, 255);
    const Scalar BLUE_1    (0, 128, 255);
    const Scalar ZERO     (0);
    const Scalar ONE      (1);

    /* COMMON FUNCTIONS */

    double getFps(cv::Mat &t, const double timeBase);
    void push(cv::Mat &m);
    void plot(cv::Mat &mat);

    /* FILTERS */

    void normalization(cv::InputArray _a, cv::OutputArray _b);
    void denoise(cv::InputArray _a, cv::InputArray _jumps, cv::OutputArray _b);
    void detrend(cv::InputArray _a, cv::OutputArray _b, int lambda);
    void movingAverage(cv::InputArray _a, cv::OutputArray _b, int n, int s);
    void bandpass(cv::InputArray _a, cv::OutputArray _b, double low, double high);
    void butterworth_bandpass_filter(cv::Mat &filter, double cutin, double cutoff, int n);
    void butterworth_lowpass_filter(cv::Mat &filter, double cutoff, int n);
    void frequencyToTime(cv::InputArray _a, cv::OutputArray _b);
    void timeToFrequency(cv::InputArray _a, cv::OutputArray _b, bool magnitude);
    void pcaComponent(cv::InputArray _a, cv::OutputArray _b, cv::OutputArray _pc, int low, int high);

    /* LOGGING */

    void printMatInfo(const std::string &name, InputArray _a);

    template<typename T>
    void printMat(const std::string &name, InputArray _a,
                  int rows = -1,
                  int cols = -1,
                  int channels = -1)
    {
        printMatInfo(name, _a);

        Mat a = _a.getMat();
        if (-1 == rows) rows = a.rows;
        if (-1 == cols) cols = a.cols;
        if (-1 == channels) channels = a.channels();

        for (int y = 0; y < rows; y++) {
            std::cout << "[";
            for (int x = 0; x < cols; x++) {
                T* e = &a.at<T>(y, x);
                std::cout << "(" << e[0];
                for (int c = 1; c < channels; c++) {
                    std::cout << ", " << e[c];
                }
                std::cout << ")";
            }
            std::cout << "]" << std::endl;
        }
        std::cout << std::endl;
    }

    /*QString convert_to_rgb(QImage &img)
    {
        float avgR = 0.0;
        float avgG = 0.0;
        float avgB = 0.0;
        float avgH = 0.0;

        int totalPixels = img.height() * img.width();

        for ( int row = 0; row < img.height() ; row++)
            for ( int col = 0; col < img.width() ; col++ )
            {
                QColor clrCurrent( img.pixel( col, row ));
                avgR += clrCurrent.red();
                avgG += clrCurrent.green();
                avgB += clrCurrent.blue();
                avgH += clrCurrent.hue();
            }

        avgR = avgR / totalPixels;
        avgG = avgG / totalPixels;
        avgB = avgB / totalPixels;
        avgH = avgH / totalPixels;

        QString rgb =  "R: " + QString::number(int(avgR))
                + "\nG: " + QString::number(int(avgG))
                + "\nB: "  + QString::number(int(avgB))
                + "\nH: "  + QString::number(int(avgH));

        return rgb;
    }*/

}

#endif /* opencv_hpp */
