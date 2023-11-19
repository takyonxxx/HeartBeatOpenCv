#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScreen>
#include <QFile>
#include <QStandardPaths>
#include <QGraphicsPixmapItem>
#include <QMediaDevices>
#include "frames.h"
//#include "capturethread.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#include "RPPG.hpp"

using namespace cv;

Q_DECLARE_METATYPE(cv::Mat);

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void createFile(const QString &fileName);

private:
    QGraphicsPixmapItem pixmap;
    QMediaCaptureSession m_captureSession;
    QScopedPointer<QCamera> m_camera;
    Frames *m_frames;   
    RPPG *rppg{};

private slots:
    void processFrame(QVideoFrame&);
    void processImage(QImage&);
    void printInfo(QString);
    void printBpm(QString);

    void on_pushExit_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
