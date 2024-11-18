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


void Frames::initializeCameraDevices()
{
    QStringList cameraDeviceList;
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();

    for (const QCameraDevice &cDevice : cameras)
    {
        if (cDevice.description().contains("Front", Qt::CaseInsensitive))
        {
            cameraDeviceList.append(cDevice.description());
            break;
        }
    }

    emit cameraListUpdated(cameraDeviceList);
}

void Frames::setCamera(const QString &cameraDescription)
{
    QCamera *selectedCamera = nullptr;
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    // Find the QCameraDevice with the selected description
    for (const QCameraDevice &cDevice : cameras)
    {
        if (cDevice.description() == cameraDescription)
        {
            selectedCamera = new QCamera(cDevice);
            break;
        }
    }

    if (selectedCamera == nullptr)
    {
        emit sendInfo("Selected camera not found!");
        return;
    }

    m_cam.reset(selectedCamera);

    if (m_cam->cameraFormat().isNull())
    {
        auto formats = m_cam->cameraDevice().videoFormats();
        if (!formats.isEmpty())
        {
            QCameraFormat bestFormat;
            int minDistance = std::numeric_limits<int>::max(); // Initialize to a large value

            for (const auto &fmt : formats)
            {
                //                qDebug() << fmt.resolution().width() << "x" << fmt.resolution().height();
                if (fmt.pixelFormat() == QVideoFrameFormat::Format_NV12)
                {
                    int distance = calculateDistance(1280, 720, fmt.resolution().width(), fmt.resolution().height());

                    if (distance < minDistance)
                    {
                        minDistance = distance;
                        bestFormat = fmt;
                    }
                }
            }

            m_cam->setCameraFormat(bestFormat);
        }
    }

    m_cam->setFocusMode(QCamera::FocusModeAuto);

    auto camFormat = m_cam->cameraFormat();
    auto m_formatString = QString("%1x%2 at %3 fps, %4, %5").arg(
        QString::number(camFormat.resolution().width()),
        QString::number(camFormat.resolution().height()),
        QString::number(static_cast<int>(camFormat.maxFrameRate())),
        QVideoFrameFormat::pixelFormatToString(camFormat.pixelFormat()),
        m_cam->cameraDevice().description());

    emit sendInfo(m_formatString);

    m_capture.setCamera(m_cam.get());
    m_capture.setVideoSink(this);
    m_cam->start();
}

void
Frames::stopCam()
{
    m_cam->stop();
    disconnect( m_cam.get(), 0, 0, 0 );
    m_cam->setParent( nullptr );
}
