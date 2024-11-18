#include "RPPG.hpp"
#include "opencv.hpp"
#include <future>

using namespace cv;
using namespace dnn;
using namespace std;

#define LOW_BPM 42
#define HIGH_BPM 240
#define REL_MIN_FACE_SIZE 0.4
#define SEC_PER_MIN 60
#define MAX_CORNERS 12
#define MIN_CORNERS 3
#define QUALITY_LEVEL 0.01
#define MIN_DISTANCE 20

RPPG::RPPG(QObject* parent)
    :QObject(parent)
{
}

bool RPPG::load(int camIndex, const string &haarPath, const string &dnnProtoPath, const string &dnnModelPath) {

    // algorithm setting
    rPPGAlgorithm rPPGAlg;
    rPPGAlg = to_rppgAlgorithm(DEFAULT_RPPG_ALGORITHM);

    // face detection algorithm setting
    faceDetAlgorithm faceDetAlg;
    faceDetAlg = to_faceDetAlgorithm(DEFAULT_FACEDET_ALGORITHM);

    // rescanFrequency setting
    double rescanFrequency;
    rescanFrequency = DEFAULT_RESCAN_FREQUENCY;

    // samplingFrequency setting
    double samplingFrequency;
    samplingFrequency = DEFAULT_SAMPLING_FREQUENCY;

    // max signal size setting
    int maxSignalSize;
    maxSignalSize = DEFAULT_MAX_SIGNAL_SIZE;

    // min signal size setting
    int minSignalSize;
    minSignalSize = DEFAULT_MIN_SIGNAL_SIZE;

    // Reading downsample setting
    int downsample;
    downsample = DEFAULT_DOWNSAMPLE;
    QString _haarPath, _dnnProtoPath, _dnnModelPath;

#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    _haarPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + haarPath.c_str();
    std::ifstream test1(_haarPath.toStdString().c_str());
    if (!test1.is_open()) {
        std::cout << "Face classifier xml not found!" << std::endl;
        info = "Face classifier xml not found!";
        emit sendInfo(info);
        return false;
    }

    _dnnProtoPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + dnnProtoPath.c_str();
    std::ifstream test2(_dnnProtoPath.toStdString().c_str());
    if (!test2.is_open()) {
        std::cout << "DNN proto file not found!" << std::endl;
        info = "DNN proto file not found!";
        emit sendInfo(info);
        return false;
    }

    _dnnModelPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + dnnModelPath.c_str();
    std::ifstream test3(_dnnModelPath.toStdString().c_str());
    if (!test3.is_open()) {
        std::cout << "DNN model file not found!" << std::endl;
        info = "DNN model file not found!";
        emit sendInfo(info);
        return false;
    }    
#else
    std::ifstream test1(HAAR_CLASSIFIER_PATH);
    if (!test1) {
        std::cout << "Face classifier xml not found!" << std::endl;
        info = "Face classifier xml not found!";
        emit sendInfo(info);
        return false;
    }

    std::ifstream test2(DNN_PROTO_PATH);
    if (!test2) {
        std::cout << "DNN proto file not found!" << std::endl;
        info = "DNN proto file not found!";
        emit sendInfo(info);
        return false;
    }

    std::ifstream test3(DNN_MODEL_PATH);
    if (!test3) {
        std::cout << "DNN model file not found!" << std::endl;
        info = "DNN model file not found!";
        emit sendInfo(info);
        return false;
    }

    _haarPath = haarPath.c_str();
    _dnnProtoPath = dnnProtoPath.c_str();
    _dnnModelPath = dnnModelPath.c_str();

#endif    

    bool offlineMode = false;
    int width = 0;
    int height = 0;
    double timeBase = 0.0;
    VideoCapture localCap;

    //    std::future<void> asyncInitialization = std::async(std::launch::async, [&]() {
    //    });

    //    asyncInitialization.wait();

    try {
        localCap.open(camIndex);
    } catch (cv::Exception& e) {
        std::cerr << "OpenCV Exception: " << e.what() << std::endl;
        std::cerr << "Could not open camera with index: " << camIndex << std::endl;
    }

    if (!localCap.isOpened()) {
        width = 480;
        height = 800;
        timeBase = 0.001;
    } else {
        // Load video information
        width = localCap.get(cv::CAP_PROP_FRAME_WIDTH);
        height = localCap.get(cv::CAP_PROP_FRAME_HEIGHT);
        timeBase = 0.001;

        if (localCap.isOpened())
            localCap.release();
    }

    std::string title = offlineMode ? "rPPG offline" : "rPPG online";

    this->rPPGAlg = rPPGAlg;
    this->faceDetAlg = faceDetAlg;
    this->guiMode = true;
    this->lastSamplingTime = 0;
    this->minFaceSize = Size(min(width, height) * REL_MIN_FACE_SIZE, min(width, height) * REL_MIN_FACE_SIZE);
    this->maxSignalSize = maxSignalSize;
    this->minSignalSize = minSignalSize;
    this->rescanFlag = false;
    this->rescanFrequency = rescanFrequency;
    this->samplingFrequency = samplingFrequency;
    this->timeBase = timeBase;
#if defined(Q_OS_IOS)
    this->time_correction = 10;
#endif

    // Load classifier
    switch (faceDetAlg) {
    case haar:
        haarClassifier.load(_haarPath.toStdString());
        break;
    case deep:
        dnnClassifier = readNetFromCaffe(_dnnProtoPath.toStdString(), _dnnModelPath.toStdString());
        break;
    }

    return true;
}

void RPPG::exit() {   
}

double RPPG::processFrame(Mat &frameRGB, Mat &frameGray) {

    process_time = get_current_time();

    if (!faceValid)
    {
        lastScanTime = process_time;
        detectFace(frameRGB, frameGray);

    }
    else if ((process_time - lastScanTime) * timeBase * time_correction >= 1/rescanFrequency) {
        lastScanTime = process_time;
        detectFace(frameRGB, frameGray);
        rescanFlag = true;
    }
    else
    {
        trackFace(frameGray);
    }    

    if (faceValid)
    {
        // Update fps
        fps = getFps(t, timeBase);

        // Remove old values from raw signal buffer
        while (s.rows > fps * maxSignalSize) {
            push(s);
            push(t);
            push(re);
        }

        assert(s.rows == t.rows && s.rows == re.rows);

        // New values
        Scalar means = mean(frameRGB, mask);
        // Add new values to raw signal buffer
        double values[] = {means(0), means(1), means(2)};
        s.push_back(Mat(1, 3, CV_64F, values));

        //        QDateTime currentTime = QDateTime::currentDateTimeUtc();
        //        qint64 currentTimeInMilliseconds = currentTime.toMSecsSinceEpoch();
        //        cv::Mat matTime = cv::Mat(1, 1, CV_64F);  // Use CV_64F for long long
        //        matTime.at<double>(0, 0) = static_cast<double>(currentTimeInMilliseconds);

        t.push_back(process_time* time_correction);

        // Save rescan flag
        re.push_back(rescanFlag);

        // Update fps
        fps = getFps(t, timeBase);

        // Update band spectrum limits
        low = (int)(s.rows * LOW_BPM / SEC_PER_MIN / fps);
        high = (int)(s.rows * HIGH_BPM / SEC_PER_MIN / fps) + 1;

        int valid_signal = fps * minSignalSize;      

        // If valid signal is large enough: estimate
        if (s.rows >= valid_signal) {

            // Filtering
            switch (rPPGAlg) {
            case g:
                extractSignal_g();
                break;
            case pca:
                extractSignal_pca();
                break;
            case xminay:
                extractSignal_xminay();
                break;
            }

            // HR estimation
            estimateHeartrate();
        }

        if (guiMode) {
            draw(frameRGB);
        }
    }

    rescanFlag = false;
    frameGray.copyTo(lastFrameGray);
    return meanBpm;
}

void RPPG::detectFace(Mat &frameRGB, Mat &frameGray) {

    //    cout << "Scanning for faces…" << faceDetAlg << " " << endl;
    vector<Rect> boxes = {};

    switch (faceDetAlg) {
    case haar:
        // Detect faces with Haar classifier
        if (!frameGray.empty()) {
            haarClassifier.detectMultiScale(frameGray, boxes, 1.1, 2, CASCADE_SCALE_IMAGE, minFaceSize);
        } else {
            // Handle the case when frameGray is empty
            cerr << "Error: Input grayscale frame is empty." << endl;
        }
        break;
    case deep:
        // Detect faces with DNN
        Mat resize300;
        cv::resize(frameRGB, resize300, Size(300, 300));
        Mat blob = blobFromImage(resize300, 1.0, Size(300, 300), Scalar(104.0, 177.0, 123.0));
        dnnClassifier.setInput(blob);
        Mat detection = dnnClassifier.forward();
        Mat detectionMat(detection.size[2], detection.size[3], CV_32F, detection.ptr<float>());
        float confidenceThreshold = 0.5;

        for (int i = 0; i < detectionMat.rows; i++) {
            float confidence = detectionMat.at<float>(i, 2);
            if (confidence > confidenceThreshold) {
                int xLeftBottom = static_cast<int>(detectionMat.at<float>(i, 3) * frameRGB.cols);
                int yLeftBottom = static_cast<int>(detectionMat.at<float>(i, 4) * frameRGB.rows);
                int xRightTop = static_cast<int>(detectionMat.at<float>(i, 5) * frameRGB.cols);
                int yRightTop = static_cast<int>(detectionMat.at<float>(i, 6) * frameRGB.rows);
                Rect object((int)xLeftBottom, (int)yLeftBottom,
                            (int)(xRightTop - xLeftBottom),
                            (int)(yRightTop - yLeftBottom));
                boxes.push_back(object);
            }
        }
        break;
    }

    if (boxes.size() > 0) {

        //        cout << "Found a face" << endl;

        setNearestBox(boxes);
        detectCorners(frameGray);
        updateROI();
        updateMask(frameGray);
        faceValid = true;

    } else {
        invalidateFace();
    }
}

void RPPG::setNearestBox(vector<Rect> boxes) {
    int index = 0;
    Point p = box.tl() - boxes.at(0).tl();
    int min = p.x * p.x + p.y * p.y;
    for (int i = 1; i < boxes.size(); i++) {
        p = box.tl() - boxes.at(i).tl();
        int d = p.x * p.x + p.y * p.y;
        if (d < min) {
            min = d;
            index = i;
        }
    }
    box = boxes.at(index);
}

void RPPG::detectCorners(Mat &frameGray) {

    // Define tracking region
    Mat trackingRegion = Mat::zeros(frameGray.rows, frameGray.cols, CV_8UC1);
    Point points[1][4];
    points[0][0] = Point(box.tl().x + 0.22 * box.width,
                         box.tl().y + 0.21 * box.height);
    points[0][1] = Point(box.tl().x + 0.78 * box.width,
                         box.tl().y + 0.21 * box.height);
    points[0][2] = Point(box.tl().x + 0.70 * box.width,
                         box.tl().y + 0.65 * box.height);
    points[0][3] = Point(box.tl().x + 0.30 * box.width,
                         box.tl().y + 0.65 * box.height);
    const Point *pts[1] = {points[0]};
    int npts[] = {4};
    fillPoly(trackingRegion, pts, npts, 1, WHITE);

    // Apply corner detection
    goodFeaturesToTrack(frameGray,
                        corners,
                        MAX_CORNERS,
                        QUALITY_LEVEL,
                        MIN_DISTANCE,
                        trackingRegion,
                        3,
                        false,
                        0.04);
}

void RPPG::trackFace(Mat &frameGray) {

    // Make sure enough corners are available
    if (corners.size() < MIN_CORNERS) {
        detectCorners(frameGray);
    }

    Contour2f corners_1;
    Contour2f corners_0;
    vector<uchar> cornersFound_1;
    vector<uchar> cornersFound_0;
    Mat err;

    if(corners.size() > 0)
    {
        // Track face features with Kanade-Lucas-Tomasi (KLT) algorithm
        calcOpticalFlowPyrLK(lastFrameGray, frameGray, corners, corners_1, cornersFound_1, err);

        // Backtrack once to make it more robust
        calcOpticalFlowPyrLK(frameGray, lastFrameGray, corners_1, corners_0, cornersFound_0, err);
    }

    // Exclude no-good corners
    Contour2f corners_1v;
    Contour2f corners_0v;
    for (size_t j = 0; j < corners.size(); j++) {
        if (cornersFound_1[j] && cornersFound_0[j]
            && norm(corners[j]-corners_0[j]) < 2) {
            corners_0v.push_back(corners_0[j]);
            corners_1v.push_back(corners_1[j]);
        } else {
            qDebug() << "Mis! ";
        }
    }

    if (corners_1v.size() >= MIN_CORNERS) {

        // Save updated features
        corners = corners_1v;

        // Estimate affine transform
        Mat transform = estimateRigidTransform(corners_0v, corners_1v, false);

        if (transform.total() > 0) {

            // Update box
            Contour2f boxCoords;
            boxCoords.push_back(box.tl());
            boxCoords.push_back(box.br());
            Contour2f transformedBoxCoords;

            cv::transform(boxCoords, transformedBoxCoords, transform);
            box = Rect(transformedBoxCoords[0], transformedBoxCoords[1]);

            // Update roi
            Contour2f roiCoords;
            roiCoords.push_back(roi.tl());
            roiCoords.push_back(roi.br());
            Contour2f transformedRoiCoords;
            cv::transform(roiCoords, transformedRoiCoords, transform);
            roi = Rect(transformedRoiCoords[0], transformedRoiCoords[1]);

            updateMask(frameGray);
        }

    } else {
        qDebug() << "Tracking failed! Not enough corners left.";
        invalidateFace();
    }
}

void RPPG::updateROI() {
    this->roi = Rect(Point(box.tl().x + 0.3 * box.width, box.tl().y + 0.1 * box.height),
                     Point(box.tl().x + 0.7 * box.width, box.tl().y + 0.25 * box.height));
}

void RPPG::updateMask(Mat &frameGray) {
    mask = Mat::zeros(frameGray.size(), frameGray.type());
    rectangle(mask, this->roi, WHITE, FILLED);
}

void RPPG::invalidateFace() {

    s = Mat1d();
    s_f = Mat1d();
    t = Mat1d();
    re = Mat1b();
    powerSpectrum = Mat1d();
    faceValid = false;
}

void RPPG::extractSignal_g() {

    // Denoise
    Mat s_den = Mat(s.rows, 1, CV_64F);
    denoise(s.col(1), re, s_den);

    // Normalise
    normalization(s_den, s_den);

    // Detrend
    Mat s_det = Mat(s_den.rows, s_den.cols, CV_64F);
    detrend(s_den, s_det, fps);

    // Moving average
    Mat s_mav = Mat(s_det.rows, s_det.cols, CV_64F);
    movingAverage(s_det, s_mav, 3, fmax(floor(fps/6), 2));

    s_mav.copyTo(s_f);

}

void RPPG::extractSignal_pca() {

    // Denoise signals
    Mat s_den = Mat(s.rows, s.cols, CV_64F);
    denoise(s, re, s_den);

    // Normalize signals
    normalization(s_den, s_den);

    // Detrend
    Mat s_det = Mat(s.rows, s.cols, CV_64F);
    detrend(s_den, s_det, fps);

    // PCA to reduce dimensionality
    Mat s_pca = Mat(s.rows, 1, CV_32F);
    Mat pc = Mat(s.rows, s.cols, CV_32F);
    pcaComponent(s_det, s_pca, pc, low, high);

    // Moving average
    Mat s_mav = Mat(s.rows, 1, CV_32F);
    movingAverage(s_pca, s_mav, 3, fmax(floor(fps/6), 2));

    s_mav.copyTo(s_f);    
}

void RPPG::extractSignal_xminay() {

    // Denoise signals
    Mat s_den = Mat(s.rows, s.cols, CV_64F);
    denoise(s, re, s_den);

    // Normalize raw signals
    Mat s_n = Mat(s_den.rows, s_den.cols, CV_64F);
    normalization(s_den, s_n);

    // Calculate X_s signal
    Mat x_s = Mat(s.rows, s.cols, CV_64F);
    addWeighted(s_n.col(0), 3, s_n.col(1), -2, 0, x_s);

    // Calculate Y_s signal
    Mat y_s = Mat(s.rows, s.cols, CV_64F);
    addWeighted(s_n.col(0), 1.5, s_n.col(1), 1, 0, y_s);
    addWeighted(y_s, 1, s_n.col(2), -1.5, 0, y_s);

    // Bandpass
    Mat x_f = Mat(s.rows, s.cols, CV_32F);
    bandpass(x_s, x_f, low, high);
    x_f.convertTo(x_f, CV_64F);
    Mat y_f = Mat(s.rows, s.cols, CV_32F);
    bandpass(y_s, y_f, low, high);
    y_f.convertTo(y_f, CV_64F);

    // Calculate alpha
    Scalar mean_x_f;
    Scalar stddev_x_f;
    meanStdDev(x_f, mean_x_f, stddev_x_f);
    Scalar mean_y_f;
    Scalar stddev_y_f;
    meanStdDev(y_f, mean_y_f, stddev_y_f);
    double alpha = stddev_x_f.val[0]/stddev_y_f.val[0];

    // Calculate signal
    Mat xminay = Mat(s.rows, 1, CV_64F);
    addWeighted(x_f, 1, y_f, -alpha, 0, xminay);

    // Moving average
    movingAverage(xminay, s_f, 3, fmax(floor(fps/6), 2));

}

void RPPG::estimateHeartrate() {

    powerSpectrum = cv::Mat(s_f.size(), CV_32F);
    timeToFrequency(s_f, powerSpectrum, true);

    // band mask
    const int total = s_f.rows;
    Mat bandMask = Mat::zeros(s_f.size(), CV_8U);
    bandMask.rowRange(min(low, total), min(high, total) + 1).setTo(ONE);

    if (!powerSpectrum.empty()) {

        // grab index of max power spectrum
        double min, max;
        Point pmin, pmax;
        minMaxLoc(powerSpectrum, &min, &max, &pmin, &pmax, bandMask);

        // calculate BPM
        bpm = pmax.y * fps / total * SEC_PER_MIN;
        bpms.push_back(bpm);
    }

    if ((process_time - lastSamplingTime) * timeBase * time_correction >= 1/samplingFrequency) {
        lastSamplingTime = get_current_time();
        cv::sort(bpms, bpms, SORT_EVERY_COLUMN);
        // average calculated BPMs since last sampling time
        meanBpm = mean(bpms)(0);
        minBpm = bpms.at<double>(0, 0);
        maxBpm = bpms.at<double>(bpms.rows-1, 0);
        bpms.pop_back(bpms.rows);
    }
}

/*void RPPG::draw(cv::Mat &frameRGB) {

    // Draw roi
    rectangle(frameRGB, roi, GREEN, 2);

    // Draw bounding box
    rectangle(frameRGB, box, RED, 2);

    // Draw signal
    if (!s_f.empty() && !powerSpectrum.empty()) {

        // Display of signals with fixed dimensions
        double displayHeight = box.height/2.0;
        double displayWidth = box.width*0.8;

        // Draw signal
        double vmin, vmax;
        Point pmin, pmax;
        minMaxLoc(s_f, &vmin, &vmax, &pmin, &pmax);
        double heightMult = displayHeight/(vmax - vmin);
        double widthMult = displayWidth/(s_f.rows - 1);
        double drawAreaTlX = box.tl().x + box.width*0.1;
        double drawAreaTlY = box.tl().y - box.height/2 - 10;
        Point p1(drawAreaTlX, drawAreaTlY + (vmax - s_f.at<double>(0, 0))*heightMult);
        Point p2;
        for (int i = 1; i < s_f.rows; i++) {
            p2 = Point(drawAreaTlX + i * widthMult, drawAreaTlY + (vmax - s_f.at<double>(i, 0))*heightMult);
            line(frameRGB, p1, p2, BLUE_1, 4);
            p1 = p2;
        }

        //         Draw powerSpectrum
        //        const int total = s_f.rows;
        //        Mat bandMask = Mat::zeros(s_f.size(), CV_8U);
        //        bandMask.rowRange(min(low, total), min(high, total) + 1).setTo(ONE);
        //        minMaxLoc(powerSpectrum, &vmin, &vmax, &pmin, &pmax, bandMask);
        //        heightMult = displayHeight/(vmax - vmin);
        //        widthMult = displayWidth/(high - low);
        //        drawAreaTlX = box.tl().x + box.width + 20;
        //        drawAreaTlY = box.tl().y + box.height/2.0;
        //        p1 = Point(drawAreaTlX, drawAreaTlY + (vmax - powerSpectrum.at<double>(low, 0))*heightMult);
        //        for (int i = low + 1; i <= high; i++) {
        //            p2 = Point(drawAreaTlX + (i - low) * widthMult, drawAreaTlY + (vmax - powerSpectrum.at<double>(i, 0)) * heightMult);
        //            line(frameRGB, p1, p2, RED, 2);
        //            p1 = p2;
        //        }
    }

    std::stringstream ss;

    // Draw BPM text
    if (faceValid) {
        ss << std::fixed << std::setprecision(0) << meanBpm;
        if(meanBpm < MAX_BPM)
        {
#if defined (Q_OS_ANDROID)
            putText(frameRGB, ss.str(), Point(box.tl().x + 10, box.tl().y + box.height - 10), FONT_HERSHEY_PLAIN, 12, BLUE_1, 12);
#else
            putText(frameRGB, ss.str(), Point(box.tl().x + 10, box.tl().y + box.height - 10), FONT_HERSHEY_PLAIN, 8, BLUE_1,8);
#endif
        }
    }

    // Draw FPS text
    ss.str("");
    ss << fps << " fps";
    putText(frameRGB, ss.str(), Point(box.tl().x, box.br().y + 60), FONT_HERSHEY_PLAIN, 4, GREEN, 4);

    // Draw corners
    for (unsigned int i = 0; i < corners.size(); i++) {
        //circle(frameRGB, corners[i], r, WHITE, -1, 8, 0);
        line(frameRGB, Point(corners[i].x-5,corners[i].y), Point(corners[i].x+5,corners[i].y), GREEN, 2);
        line(frameRGB, Point(corners[i].x,corners[i].y-5), Point(corners[i].x,corners[i].y+5), GREEN, 2);
    }
}*/

void RPPG::draw(cv::Mat &frameRGB) {
    // Draw roi
    rectangle(frameRGB, roi, cv::Scalar(0, 255, 0), 2);

    // Draw bounding box
    rectangle(frameRGB, box, cv::Scalar(0, 0, 255), 2);

    // Draw signal
    if (!s_f.empty()) {
        // Display of signals with fixed dimensions
        double displayHeight = box.height/2.0;
        double displayWidth = box.width*0.8;

        // Draw signal with improved visibility
        double vmin, vmax;
        Point pmin, pmax;
        minMaxLoc(s_f, &vmin, &vmax, &pmin, &pmax);

        if (vmax > vmin) {  // Check to avoid division by zero
            double heightMult = displayHeight/(vmax - vmin);
            double widthMult = displayWidth/(s_f.rows - 1);
            double drawAreaTlX = box.tl().x + box.width*0.1;
            double drawAreaTlY = box.tl().y - box.height/2 - 10;

            // Draw background for better visibility
            Point bgTL(drawAreaTlX - 5, drawAreaTlY - 5);
            Point bgBR(drawAreaTlX + displayWidth + 5, drawAreaTlY + displayHeight + 5);
            rectangle(frameRGB, bgTL, bgBR, cv::Scalar(32, 32, 32), -1);

            // Draw zero line
            double zeroY = drawAreaTlY + displayHeight/2;
            line(frameRGB,
                 Point(drawAreaTlX, zeroY),
                 Point(drawAreaTlX + displayWidth, zeroY),
                 cv::Scalar(0, 128, 0), 1);

            // Draw signal
            Point p1(drawAreaTlX, drawAreaTlY + (vmax - s_f.at<double>(0, 0))*heightMult);
            Point p2;

            for (int i = 1; i < s_f.rows; i++) {
                p2 = Point(drawAreaTlX + i * widthMult,
                           drawAreaTlY + (vmax - s_f.at<double>(i, 0))*heightMult);
                line(frameRGB, p1, p2, cv::Scalar(255, 0, 100), 2);
                p1 = p2;
            }

            // Draw border around signal area
            rectangle(frameRGB, bgTL, bgBR, cv::Scalar(128, 128, 128), 1);
        }
    }

    // Draw BPM text with improved visibility
    if (faceValid) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << meanBpm;

        if(meanBpm < MAX_BPM && meanBpm > MIN_BPM) {
            // Draw background for text
            cv::Size textSize = getTextSize(ss.str(), FONT_HERSHEY_PLAIN, 8, 8, nullptr);
            Point textBgTL(box.tl().x + 5, box.tl().y + box.height - textSize.height - 15);
            Point textBgBR(textBgTL.x + textSize.width + 10, textBgTL.y + textSize.height + 10);
            rectangle(frameRGB, textBgTL, textBgBR, cv::Scalar(0, 0, 0), -1);

#if defined (Q_OS_ANDROID)
            putText(frameRGB, ss.str(),
                    Point(box.tl().x + 10, box.tl().y + box.height - 10),
                    FONT_HERSHEY_PLAIN, 12, cv::Scalar(255, 0, 100), 12);
#else
            putText(frameRGB, ss.str(),
                    Point(box.tl().x + 10, box.tl().y + box.height - 10),
                    FONT_HERSHEY_PLAIN, 8, cv::Scalar(255, 0, 100), 8);
#endif
        }
    }

    // Draw FPS text
    std::stringstream ss;
    ss.str("");
    ss << std::fixed << std::setprecision(1) << fps << " fps";
    putText(frameRGB, ss.str(),
            Point(box.tl().x, box.br().y + 60),
            FONT_HERSHEY_PLAIN, 4, cv::Scalar(0, 255, 0), 4);

    // Draw corners
    for (const auto& corner : corners) {
        line(frameRGB,
             Point(corner.x-5, corner.y),
             Point(corner.x+5, corner.y),
             cv::Scalar(0, 255, 0), 2);
        line(frameRGB,
             Point(corner.x, corner.y-5),
             Point(corner.x, corner.y+5),
             cv::Scalar(0, 255, 0), 2);
    }
}

