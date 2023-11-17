#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScreen>
#include <QFile>
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

    void createFile(QString &fileName)
    {
        QString data;

        QFile file(fileName);
        if(!file.open(QIODevice::ReadOnly)) {
           return;
        }
        else
        {
            data = file.readAll();
        }

        file.close();

        QString filename = fileName.remove(":/opencv/");
        QFile temp(filename);


        if(temp.exists())
            return;

        if (temp.open(QIODevice::ReadWrite)) {
            QTextStream stream(&temp);
            stream << data ;
        }
    }

#ifdef Q_OS_ANDROID

#endif

private:
//    CaptureThread* cpThread{};
    int count{0};
    QGraphicsPixmapItem pixmap;
    QMediaCaptureSession m_captureSession;
    QScopedPointer<QCamera> m_camera;
    Frames *m_frames;   
    RPPG *rppg{};

private slots:
    void processFrame(QVideoFrame&);
    void processImage(QImage&);
    void printInfo(QString);

    void on_pushExit_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
