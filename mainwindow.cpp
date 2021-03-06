#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->textInfo->setStyleSheet("font-size: 36pt; color:#ECF0F1; background-color: #212F3C; padding: 6px; spacing: 6px;");
    ui->graphicsView->setStyleSheet("font-size: 24pt; color:#ECF0F1; background-color: #212F3C; padding: 6px; spacing: 6px;");

    //QString currentTime = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
    ui->graphicsView->setScene(new QGraphicsScene(this));
    setGeometry(0,0,1024,600);
    ui->graphicsView->scene()->addItem(&pixmap);

    QString fileName = ":/opencv/deploy.prototxt";
    createFile(fileName);
    fileName = ":/opencv/haarcascade_frontalface_alt.xml";
    createFile(fileName);
    fileName = ":/opencv/res10_300x300_ssd_iter_140000.caffemodel";
    createFile(fileName);

    QScreen *s = QGuiApplication::primaryScreen();
    connect(s, SIGNAL(orientationChanged(Qt::ScreenOrientation)),
            this, SLOT(orientationChanged(Qt::ScreenOrientation)));

    s->setOrientationUpdateMask(
                Qt::PortraitOrientation
                | Qt::LandscapeOrientation
                | Qt::InvertedPortraitOrientation
                | Qt::InvertedLandscapeOrientation);

    cpThread = new CaptureThread();
    connect(cpThread, &CaptureThread::frameCaptured, this, &MainWindow::processFrame);
    connect(cpThread, &CaptureThread::sendInfo, this, &MainWindow::printInfo);
    connect(cpThread, &CaptureThread::finished, cpThread, &QObject::deleteLater);
    connect(cpThread, &CaptureThread::finished, [&](){ cpThread = nullptr; });
    cpThread->start();
}

MainWindow::~MainWindow()
{
    cpThread->abort();
    cpThread->quit();
    cpThread->wait(1000);
    delete ui;
}

void MainWindow::orientationChanged(Qt::ScreenOrientation orientation)
{
    cpThread->setPause(true);
    cpThread->setOrientation(orientation);
    cpThread->setPause(false);
}

void MainWindow::processFrame(Mat &frameRGB, double bpm, bool swap)
{
    QImage img_face((uchar*)frameRGB.data, frameRGB.cols, frameRGB.rows, frameRGB.step, QImage::Format_RGB888);
    if(swap)
        img_face = img_face.rgbSwapped();

    pixmap.setPixmap( QPixmap::fromImage(img_face));
    ui->graphicsView->fitInView(&pixmap, Qt::KeepAspectRatio);
    printInfo(QString::number(bpm, 'f', 1));
}

void MainWindow::printInfo(QString info)
{
    ui->textInfo->setText(info);
}

