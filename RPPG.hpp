#ifndef RPPG_hpp
#define RPPG_hpp

#include <fstream>
#include <string>
#include <stdio.h>
#include <iostream>
#include <chrono>
#include <vector>
#include <map>
#include <limits>
#include <string>
#include <algorithm>
#include <QDebug>
#include <QCamera>
#include <QDateTime>
#include <QStandardPaths>
#include <opencv2/opencv.hpp>

#define DEFAULT_RPPG_ALGORITHM "g"
#define DEFAULT_FACEDET_ALGORITHM "haar"
#define DEFAULT_RESCAN_FREQUENCY 1
#define DEFAULT_SAMPLING_FREQUENCY 1
#define DEFAULT_DOWNSAMPLE 1 // x means only every xth frame is used

#define MIN_BPM 40
#define MAX_BPM 240
#define DEFAULT_MIN_SIGNAL_SIZE 5
#define DEFAULT_MAX_SIGNAL_SIZE 15


#define HAAR_CLASSIFIER_PATH "haarcascade_frontalface_alt.xml"
#define DNN_PROTO_PATH "deploy.prototxt"
#define DNN_MODEL_PATH "res10_300x300_ssd_iter_140000.caffemodel"


using namespace cv;
using namespace dnn;
using namespace std;

enum rPPGAlgorithm { g, pca, xminay };
enum faceDetAlgorithm { haar, deep };

class RPPG : public QObject
{
    Q_OBJECT

public:
    explicit RPPG(QObject *parent = nullptr);
    // Load Settings
    bool load(int camIndex, const string &haarPath, const string &dnnProtoPath, const string &dnnModelPath);
    double processFrame(Mat &frameRGB, Mat &frameGray);
    void exit();

private:

    typedef vector<Point2f> Contour2f;

    void detectFace(Mat &frameRGB, Mat &frameGray);
    void setNearestBox(vector<Rect> boxes);
    void detectCorners(Mat &frameGray);
    void trackFace(Mat &frameGray);
    void updateMask(Mat &frameGray);
    void updateROI();
    void extractSignal_g();
    void extractSignal_pca();
    void extractSignal_xminay();
    void estimateHeartrate();
    void draw(Mat &frameRGB);
    void invalidateFace();

    int get_current_time()
    {
        int64 tickCount = cv::getTickCount();
        double timeInMilliseconds = (tickCount * 1000.0 / time_correction) / cv::getTickFrequency();

        // Check for overflow before casting to int
        if (timeInMilliseconds < std::numeric_limits<int>::min() || timeInMilliseconds > std::numeric_limits<int>::max())
        {
            qDebug() << "Error : overflow int.";
            return 0;
        }
        return static_cast<int>(timeInMilliseconds);
    }

    static bool to_bool(string s) {
        bool result;
        transform(s.begin(), s.end(), s.begin(), ::tolower);
        istringstream is(s);
        is >> boolalpha >> result;
        return result;
    }

    static rPPGAlgorithm to_rppgAlgorithm(string s) {
        rPPGAlgorithm result;
        if (s == "g") result = g;
        else if (s == "pca") result = pca;
        else if (s == "xminay") result = xminay;
        else {
            std::cout << "Please specify valid rPPG algorithm (g, pca, xminay)!" << std::endl;
        }
        return result;
    }

    static faceDetAlgorithm to_faceDetAlgorithm(string s) {
        faceDetAlgorithm result;
        if (s == "haar") result = haar;
        else if (s == "deep") result = deep;
        else {
            std::cout << "Please specify valid face detection algorithm (haar, deep)!" << std::endl;
        }
        return result;
    }

    // The algorithm
    rPPGAlgorithm rPPGAlg;

    // The classifier
    faceDetAlgorithm faceDetAlg;
    CascadeClassifier haarClassifier;
    Net dnnClassifier;

    // Settings
    Size minFaceSize;
    int maxSignalSize;
    int minSignalSize;
    double rescanFrequency;
    double samplingFrequency;
    double timeBase;
    bool guiMode;

    // State variables
    int time_correction = 1;
    int process_time= 0;
    int lastSamplingTime= 0;
    int lastScanTime= 0;
    double fps;
    int high;
    int low;
    int64_t now;
    bool faceValid;
    bool rescanFlag;

    // Tracking
    Mat lastFrameGray;
    Contour2f corners;

    // Mask
    Rect box;
    Mat1b mask;
    Rect roi;

    // Raw signal
    Mat1d s;
    Mat1d t;
    Mat1b re;

    // Estimation
    Mat1d s_f;
    Mat1d bpms;
    Mat1d powerSpectrum;
    double bpm = 0.0;
    double meanBpm;
    double minBpm;
    double maxBpm;    

    QString info{};

signals:
    void sendInfo(QString);
};


#endif /* RPPG_hpp */
