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
