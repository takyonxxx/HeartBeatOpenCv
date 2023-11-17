#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->textInfo->setStyleSheet("font-size: 24pt; color:#ECF0F1; background-color: #212F3C; padding: 6px; spacing: 6px;");
    ui->graphicsView->setStyleSheet("font-size: 24pt; color:#ECF0F1; background-color: #212F3C; padding: 6px; spacing: 6px;");
    ui->pushExit->setStyleSheet("font-size: 24pt; font: bold; color: #ffffff; background-color: #336699;");

    //QString currentTime = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
    ui->graphicsView->setScene(new QGraphicsScene(this));
    setGeometry(0,0,1024,1024);
    ui->graphicsView->scene()->addItem(&pixmap);

    QString fileName = ":/opencv/deploy.prototxt";
    createFile(fileName);
    fileName = ":/opencv/haarcascade_frontalface_alt.xml";
    createFile(fileName);
    fileName = ":/opencv/res10_300x300_ssd_iter_140000.caffemodel";
    createFile(fileName);

    rppg = new RPPG();
    connect(rppg, &RPPG::sendInfo, this, &MainWindow::printInfo);
    rppg->load(0, HAAR_CLASSIFIER_PATH, DNN_PROTO_PATH, DNN_MODEL_PATH);

    m_frames = new Frames();
    connect(m_frames, &Frames::sendInfo, this, &MainWindow::printInfo);
    connect(m_frames, &Frames::frameCaptured, this, &MainWindow::processFrame);
    m_frames->initCam();
}

MainWindow::~MainWindow()
{
    if(m_frames)
        delete m_frames;

    if(rppg)
        delete rppg;

    delete ui;
}

void MainWindow::processFrame(QVideoFrame &frame)
{
    if (frame.isValid())
    {
        QVideoFrame cloneFrame(frame);
        cloneFrame.map(QVideoFrame::ReadOnly);

        QImage img = cloneFrame.toImage();
        cloneFrame.unmap();

        img = img.convertToFormat(QImage::Format_RGB888);
        Mat frameRGB(img.height(),
                     img.width(),
                     CV_8UC3,
                     img.bits(),
                     img.bytesPerLine());

        if(!frameRGB.empty())
        {
            Mat frameGray;
            double bpm = 0.0;

            // Generate grayframe
            cvtColor((InputArray)frameRGB, (OutputArray)frameGray, COLOR_BGR2GRAY);
            equalizeHist((InputArray)frameGray, (OutputArray)frameGray);

            int time;
            time = (cv::getTickCount()*1000.0)/cv::getTickFrequency();
            bpm = rppg->processFrame(frameRGB, frameGray, time);
            printInfo(QString::number(bpm, 'f', 1));
        }

        QImage img_face((uchar*)frameRGB.data, frameRGB.cols, frameRGB.rows, frameRGB.step, QImage::Format_RGB888);
        pixmap.setPixmap( QPixmap::fromImage(img_face));
        ui->graphicsView->fitInView(&pixmap, Qt::KeepAspectRatioByExpanding);
    }
}

void MainWindow::processImage(QImage &img_face)
{    
    pixmap.setPixmap( QPixmap::fromImage(img_face));
    ui->graphicsView->fitInView(&pixmap, Qt::KeepAspectRatio);
}

void MainWindow::printInfo(QString info)
{
    ui->textInfo->setText(info);
}

void MainWindow::on_pushExit_clicked()
{
    qApp->exit();
}

