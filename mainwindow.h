#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScreen>
#include <QFile>
#include "capturethread.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

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
            stream << data << endl;
        }
    }

#ifdef Q_OS_ANDROID

#endif

private:
    CaptureThread* cpThread{};
    QGraphicsPixmapItem pixmap;

private slots:    
    void processFrame(Mat&, double, bool);
    void printInfo(QString);
    void orientationChanged(Qt::ScreenOrientation orientation);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
