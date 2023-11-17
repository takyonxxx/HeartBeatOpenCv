#ifndef FRAMES_H
#define FRAMES_H
// Qt include.
#include <QVideoSink>
#include <QCamera>
#include <QMediaDevices>
#include <QMediaCaptureSession>
#include <QPointer>

//! Frames listener.
class Frames
    :	public QVideoSink
{
    Q_OBJECT
    Q_PROPERTY( QVideoSink* videoSink READ videoSink WRITE setVideoSink NOTIFY videoSinkChanged )

signals:
    //! Video sink changed.
    void videoSinkChanged(QVideoSink* videoSink);
    void imageCaptured(QImage&);
    void frameCaptured(QVideoFrame&);
    void sendInfo(QString);

public:
    static void registerQmlType();

    explicit Frames( QObject * parent = nullptr );
    ~Frames() override;

    //! Init camera.
    void initCam();

    //! \return Sink of video output.
    QVideoSink * videoSink() const;
    //! Set sink of video output.
    void setVideoSink( QVideoSink * newVideoSink );

private slots:    
    //! Stop camera.
    void stopCam();
    //! Video frame changed.
    void newFrame( const QVideoFrame & frame );    

private:
    Q_DISABLE_COPY( Frames )
    QScopedPointer<QCamera> m_cam;
    QMediaCaptureSession m_capture;
    QPointer< QVideoSink > m_videoSink;      
}; // class Frames

#endif // FRAMES_H
