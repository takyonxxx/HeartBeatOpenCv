#include "capturethread.h"

CaptureThread::CaptureThread(QObject* parent)
    :QThread(parent)
{
    qRegisterMetaType<Mat>("Mat&");

    rppg = new RPPG();
    connect(rppg, &RPPG::sendInfo, this, &CaptureThread::printInfo);
    rppg->load(0, HAAR_CLASSIFIER_PATH, DNN_PROTO_PATH, DNN_MODEL_PATH);

    if(!videoCapturer.open(0))
    {
        initQCamera();
    }
}

void CaptureThread::abort()
{    
    m_abort = true;
    if( m_camera )
        m_camera->stop();
    if(rppg)
        delete rppg;
}

void CaptureThread::initQCamera()
{
    QCameraInfo cam_face;
    const QList<QCameraInfo> availableCameras = QCameraInfo::availableCameras();
    for (const QCameraInfo &cameraInfo : availableCameras) {
        cam_face = cameraInfo;
        info = cameraInfo.description();
        emit printInfo(info);
    }

    m_camera.reset(new QCamera(cam_face));
    connect(m_camera.data(), &QCamera::stateChanged, this, &CaptureThread::updateCameraState);
    connect(m_camera.data(), QOverload<QCamera::Error>::of(&QCamera::error), this, &CaptureThread::displayCameraError);

    m_cameraCapture = new QtCameraCapture;
    connect(m_cameraCapture, &QtCameraCapture::frameAvailable, this, &CaptureThread::processImage);
    m_camera->setViewfinder(m_cameraCapture);

    if (m_camera->state() == QCamera::ActiveState) {
        m_camera->stop();
    }

    m_camera->start();
}

void CaptureThread::setPause(bool pause)
{
    m_pause = pause;
}

void CaptureThread::setOrientation(const Qt::ScreenOrientation &orientation)
{
    m_orientation = orientation;
}

void CaptureThread::updateCameraState(QCamera::State state)
{
    qDebug() << "Camera status: " << m_camera->status() << state;
}

void CaptureThread::displayCameraError()
{
    qDebug() << "Error : " <<  m_camera->errorString();
}

void CaptureThread::processImage(QVideoFrame frame)
{
    if(m_pause)
        return;

    if (frame.isValid())
    {
        QVideoFrame cloneFrame(frame);
        cloneFrame.map(QAbstractVideoBuffer::ReadOnly);

        QImage img = cloneFrame.image();
        /*QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(cloneFrame.pixelFormat());
        QImage img( cloneFrame.bits(),
                     cloneFrame.width(),
                     cloneFrame.height(),
                     cloneFrame.bytesPerLine(),
                     imageFormat);*/

        cloneFrame.unmap();

        if(m_orientation == Qt::ScreenOrientation::PortraitOrientation)
        {
            if(img.width() > img.height())
            {
                QTransform transf;
                transf.rotate(270);
                img = img.transformed(transf);
                img = img.mirrored(true, false);
            }
        }

        img = img.convertToFormat(QImage::Format_RGB888);

        //info = QString::number(img.width()) + "x" + QString::number(img.height());
        //emit sendInfo(info);

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

            if (count % DEFAULT_DOWNSAMPLE == 0)
            {
                bpm = rppg->processFrame(frameRGB, frameGray, time);
            }
            else
            {
                cout << "SKIPPING FRAME TO DOWNSAMPLE!" << endl;
            }
            emit frameCaptured(frameRGB, bpm, false);
            count++;
        }
    }
}

void CaptureThread::printInfo(QString info)
{
    emit sendInfo(info);
}

void CaptureThread::run()
{
    Mat frameRGB, frameGray;
    double bpm = 0.0;

    if(videoCapturer.isOpened())
    {
        info = "opencv cam active";
        emit sendInfo(info);
    }
    else
    {
        info = "qt cam active";
        emit sendInfo(info);
    }

    while(!m_abort)
    {
        if(videoCapturer.isOpened())
        {
            if(m_pause)
                continue;

            videoCapturer.read(frameRGB);

            if(!frameRGB.empty())
            {
                // Generate grayframe
                cvtColor((InputArray)frameRGB, (OutputArray)frameGray, COLOR_BGR2GRAY);
                equalizeHist((InputArray)frameGray, (OutputArray)frameGray);

                int time;
                time = (cv::getTickCount()*1000.0)/cv::getTickFrequency();

                if (count % DEFAULT_DOWNSAMPLE == 0) {
                    bpm = rppg->processFrame(frameRGB, frameGray, time);
                } else {
                    info = "SKIPPING FRAME TO DOWNSAMPLE!";
                    emit sendInfo(info);
                }

                count++;

                emit frameCaptured(frameRGB, bpm, true);

            }
        }
        else{
            QThread::sleep(1);
        }
    }
}
