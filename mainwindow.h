#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScreen>
#include <QFile>
#include <QStandardPaths>
#include <QGraphicsPixmapItem>
#include <QMediaDevices>
#include <QVideoFrame>
#include <QImage>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <iomanip>
#include "frames.h"
#include "RPPG.hpp"

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#endif

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

using namespace cv;

Q_DECLARE_METATYPE(cv::Mat);

class BPMKalmanFilter {
private:
    cv::KalmanFilter kf;
    bool initialized;

public:
    BPMKalmanFilter() : kf(2, 1, 0) {  // 2 state variables (value and velocity), 1 measurement
        initialized = false;

        // Initialize transition matrix (state update matrix)
        kf.transitionMatrix = (cv::Mat_<float>(2, 2) <<
                                   1, 1,   // Position update: x(t) = x(t-1) + v(t-1)
                               0, 1    // Velocity update: v(t) = v(t-1)
                               );

        // Measurement matrix (what we can measure)
        kf.measurementMatrix = (cv::Mat_<float>(1, 2) << 1, 0);  // We only measure position

        // Increase filtering effect
        setIdentity(kf.processNoiseCov, cv::Scalar::all(0.0001));  // Lower process noise for smoother predictions
        setIdentity(kf.measurementNoiseCov, cv::Scalar::all(0.5));  // Higher measurement noise to trust predictions more

        // Initial state covariance matrix
        setIdentity(kf.errorCovPost, cv::Scalar::all(1));
    }

    float update(float measurement) {
        if (!initialized) {
            kf.statePost.at<float>(0) = measurement;
            kf.statePost.at<float>(1) = 0;
            initialized = true;
            return measurement;
        }

        cv::Mat prediction = kf.predict();

        cv::Mat_<float> measurement_matrix(1, 1);
        measurement_matrix(0) = measurement;

        cv::Mat estimated = kf.correct(measurement_matrix);

        return estimated.at<float>(0);
    }
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    static MainWindow* instance() { return m_instance; }
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupUI();
    void initializeRPPG();
    void setupCamera();
    void createFile(const QString &fileName);
    double calculateInstantHeartRate(const cv::Mat& currentFrame, const cv::Mat& mask);
    double getFilteredBpm(double newBpm);
    float getPulseValue(const Mat& maskedRed);

#if defined(Q_OS_ANDROID)
    void requestAndroidPermissions();
#endif

    QGraphicsPixmapItem pixmap;
    QMediaCaptureSession m_captureSession;
    QScopedPointer<QCamera> m_camera;
    Frames *m_frames{nullptr};
    RPPG *rppg{nullptr};
    bool frontCamEnabled = false;
    Ui::MainWindow *ui;
    static inline MainWindow* m_instance = nullptr;

    // Variables for instant heart rate calculation
    cv::Mat prevFrameGray;
    double prevAvgIntensity = 0.0;
    BPMKalmanFilter bpmKalman;
    double previousEma = 0.0;


signals:
    void cameraPermissionGranted();

private slots:
    void processFrame(QVideoFrame&);
    void processImage(QImage&);
    void printInfo(QString);
    void printValue(QString);
    void onCameraListUpdated(const QStringList &);
    void on_pushExit_clicked();
    void on_cameraComboBox_currentIndexChanged(int index);

#if defined(Q_OS_ANDROID)
    void onCameraPermissionGranted();
#endif
};

#endif // MAINWINDOW_H
