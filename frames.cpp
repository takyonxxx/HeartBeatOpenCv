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

    // Connect to camera status changes
    connect(m_cam.data(), &QCamera::activeChanged, this, [this](bool active) {
        if (active) {
            auto format = m_cam.data()->cameraFormat();
            QString formatStr = QString("%1x%2, %3")
                                    .arg(format.resolution().width())
                                    .arg(format.resolution().height())
                                    .arg(m_cam.data()->cameraDevice().description());
            emit sendInfo(formatStr);
        }
    });

    if (m_cam->cameraFormat().isNull())
    {
        auto formats = m_cam->cameraDevice().videoFormats();
        if (!formats.isEmpty())
        {
            QCameraFormat bestFormat;
            int minDistance = std::numeric_limits<int>::max();
            for (const auto &fmt : formats)
            {
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
    m_capture.setCamera(m_cam.get());
    m_capture.setVideoSink(this);
    m_cam->start();
}

QString Frames::getFormatString()
{
    if (!m_cam.data()->isActive()) {
        return "Camera not active";
    }

    QCameraDevice device = m_cam.data()->cameraDevice();
    QList<QSize> resolutions = device.photoResolutions();

    if (resolutions.isEmpty()) {
        return QString("Camera: %1").arg(device.description());
    }

    QSize maxRes = resolutions.first();
    for (const QSize &res : resolutions) {
        if (res.width() * res.height() > maxRes.width() * maxRes.height()) {
            maxRes = res;
        }
    }

    return QString("%1x%2, %3")
        .arg(maxRes.width())
        .arg(maxRes.height())
        .arg(device.description());
}

void Frames::stopCam()
{
    m_cam->stop();
    disconnect( m_cam.get(), 0, 0, 0 );
    m_cam->setParent( nullptr );
}
