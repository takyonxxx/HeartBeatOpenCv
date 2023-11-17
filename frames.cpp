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

    m_cam.reset(cameraDevice);

    if (m_cam->cameraFormat().isNull()) {
        auto formats = cameraDevice->cameraDevice().videoFormats();
        if (!formats.isEmpty()) {
            // Choose a decent camera format: Maximum resolution at at least 30 FPS
            // we use 29 FPS to compare against as some cameras report 29.97 FPS...
            QCameraFormat bestFormat;
            for (const auto &fmt : formats) {
                if (bestFormat.maxFrameRate() < 29 && fmt.maxFrameRate() > bestFormat.maxFrameRate())
                    bestFormat = fmt;
                else if (bestFormat.maxFrameRate() == fmt.maxFrameRate() &&
                         bestFormat.resolution().width()*bestFormat.resolution().height() <
                             fmt.resolution().width()*fmt.resolution().height())
                    bestFormat = fmt;
            }
            m_cam->setFocusMode( QCamera::FocusModeAuto );
            m_cam->setCameraFormat(bestFormat);
            auto m_formatString = QString( "%1x%2 at %3 fps, %4" ).arg( QString::number( bestFormat.resolution().width() ),
                                                                     QString::number( bestFormat.resolution().height() ), QString::number( (int) bestFormat.maxFrameRate() ),
                                                                     QVideoFrameFormat::pixelFormatToString( bestFormat.pixelFormat() ));
            emit sendInfo(m_formatString);
        }
    }
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
