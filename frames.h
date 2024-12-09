#ifndef FRAMES_H
#define FRAMES_H
// Qt include.
#include <QVideoSink>
#include <QCamera>
#include <QMediaDevices>
#include <QMediaCaptureSession>
#include <QPointer>

class Frames
    :	public QVideoSink
{
    Q_OBJECT
    //    Q_PROPERTY( QVideoSink* videoSink READ videoSink WRITE setVideoSink NOTIFY videoSinkChanged )

signals:
    void imageCaptured(QImage&);
    void frameCaptured(QVideoFrame&);
    void sendInfo(QString);
    void cameraListUpdated(const QStringList &cameraDevices);

public:
    // Function to calculate the Euclidean distance between two resolutions
    int calculateDistance(int targetWidth, int targetHeight, int width, int height) {
        return std::sqrt((targetWidth - width) * (targetWidth - width) +
                         (targetHeight - height) * (targetHeight - height));
    }

    static void registerQmlType();
    explicit Frames( QObject * parent = nullptr );
    ~Frames() override;

    void initializeCameraDevices();
    void setCamera(const QString &);

private slots:
    void stopCam();
    void newFrame( const QVideoFrame&);

private:
    Q_DISABLE_COPY( Frames )
    QString getFormatString();
    QScopedPointer<QCamera> m_cam;
    QMediaCaptureSession m_capture;
};

#endif // FRAMES_H
