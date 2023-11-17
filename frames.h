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

public:
    static void registerQmlType();

    explicit Frames( QObject * parent = nullptr );
    ~Frames() override;

    void initCam();

private slots:
    void stopCam();
    void newFrame( const QVideoFrame&);

private:
    Q_DISABLE_COPY( Frames )
    QScopedPointer<QCamera> m_cam;
    QMediaCaptureSession m_capture;
};

#endif // FRAMES_H
