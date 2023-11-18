#include "frames.h"

Frames::Frames( QObject * parent )
    :	QVideoSink( parent )
    ,	m_cam( nullptr )
{
    connect( this, &QVideoSink::videoFrameChanged, this, &Frames::newFrame );
}

Frames::~Frames()
{
    stopCam();
}

void
Frames::newFrame( const QVideoFrame & frame )
{    

    QVideoFrame f = frame;
    f.map( QVideoFrame::ReadOnly );

    if( f.isValid() )
    {
        f.unmap();
        emit frameCaptured(f);
    }
}

void
Frames::initCam()
{
    auto cameraDevice = new QCamera(QMediaDevices::defaultVideoInput());
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    for (const QCameraDevice &cDevice : cameras)
    {       
        if (cDevice.position() == QCameraDevice::FrontFace)
        {
            cameraDevice = new QCamera(cDevice);
        }
        if (cDevice.description().contains("Front Camera"))
        {
            cameraDevice = new QCamera(cDevice);
        }
    }

    if(cameraDevice == nullptr)
    {
        emit sendInfo("Camera Not Found!");
        return;
    }

    m_cam.reset(cameraDevice);

    if (m_cam->cameraFormat().isNull()) {
        auto formats = cameraDevice->cameraDevice().videoFormats();
        if (!formats.isEmpty()) {
            QCameraFormat bestFormat;
            for (const auto &fmt : formats) {
                if (fmt.pixelFormat() == QVideoFrameFormat::Format_NV12)
                {
                    #if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
                    if (fmt.resolution().width()== 640 &&  fmt.resolution().height() == 480)
                    {
                        bestFormat = fmt;
                    }
                    #else
                    if(fmt.resolution().width() > fmt.resolution().height()
                        && bestFormat.resolution().width() < fmt.resolution().width()
                        && bestFormat.resolution().height() < fmt.resolution().height()) {
                        bestFormat = fmt;
                    }
                    #endif
                }
            }
            m_cam->setCameraFormat(bestFormat);
        }
    }

    m_cam->setFocusMode( QCamera::FocusModeAuto );

    auto camFormat = m_cam->cameraFormat();
    auto m_formatString = QString( "%1x%2 at %3 fps, %4" ).arg( QString::number( camFormat.resolution().width() ),
                                                             QString::number( camFormat.resolution().height() ), QString::number( (int) camFormat.maxFrameRate() ),
                                                             QVideoFrameFormat::pixelFormatToString( camFormat.pixelFormat() ));
    emit sendInfo(m_formatString);

    m_capture.setCamera( m_cam.get());
    m_capture.setVideoSink( this );

    m_cam->start();
}

void
Frames::stopCam()
{
    m_cam->stop();
    disconnect( m_cam.get(), 0, 0, 0 );
    m_cam->setParent( nullptr );
}
