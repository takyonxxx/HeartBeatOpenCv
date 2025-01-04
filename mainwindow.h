#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScreen>
#include <QFile>
#include <QStandardPaths>
#include <QGraphicsPixmapItem>
#include <QMediaDevices>
#include "frames.h"
#include "RPPG.hpp"

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#endif

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

using namespace cv;
Q_DECLARE_METATYPE(cv::Mat);

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    static MainWindow* instance() { return m_instance; }
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    const int BUFFER_SIZE = 300;  // 10 seconds at 30 fps
    std::vector<double> intensityBuffer;
    std::chrono::steady_clock::time_point lastFrameTime;
    double frameInterval;  // Time between frames in seconds

    void setupUI();
    void initializeRPPG();
    void setupCamera();
    void createFile(const QString &fileName);
    void updateIntensityBuffer(double intensity) {
        if (intensityBuffer.size() >= BUFFER_SIZE) {
            intensityBuffer.erase(intensityBuffer.begin());
        }
        intensityBuffer.push_back(intensity);
    }

    double calculateGlucoseLevel(double redIntensity) {
        const double MIN_INTENSITY = 50.0;
        const double MAX_INTENSITY = 200.0;
        const double MIN_GLUCOSE = 120.0;
        const double MAX_GLUCOSE = 350.0;

        redIntensity = std::max(MIN_INTENSITY, std::min(MAX_INTENSITY, redIntensity));
        double normalizedIntensity = (redIntensity - MIN_INTENSITY) / (MAX_INTENSITY - MIN_INTENSITY);
        return MIN_GLUCOSE + normalizedIntensity * (MAX_GLUCOSE - MIN_GLUCOSE);
    }

    double calculateHeartRate() {
        if (intensityBuffer.size() < BUFFER_SIZE) {
            return 0.0;  // Not enough data yet
        }

        // Step 1: Normalize the signal
        std::vector<double> signal = intensityBuffer;
        double mean = std::accumulate(signal.begin(), signal.end(), 0.0) / signal.size();
        double stddev = 0.0;
        for (const auto& value : signal) {
            stddev += (value - mean) * (value - mean);
        }
        stddev = sqrt(stddev / signal.size());

        for (auto& value : signal) {
            value = (value - mean) / stddev;
        }

        // Step 2: Apply bandpass filter for heart rate range (60-120 BPM = 1-2 Hz)
        std::vector<double> filtered(signal.size());
        const double samplingRate = 1.0 / frameInterval;
        const double f1 = 1.0;  // 60 BPM
        const double f2 = 2.0;  // 120 BPM

        // Simple FIR bandpass filter
        const int filterOrder = 32;
        std::vector<double> filter(filterOrder);
        for (int i = 0; i < filterOrder; i++) {
            double n = i - (filterOrder - 1) / 2.0;
            if (n != 0) {
                filter[i] = (sin(2 * M_PI * f2 * n / samplingRate) -
                             sin(2 * M_PI * f1 * n / samplingRate)) / (M_PI * n);
            } else {
                filter[i] = 2 * (f2 - f1) / samplingRate;
            }
        }

        // Apply filter
        for (size_t i = filterOrder; i < signal.size(); i++) {
            filtered[i] = 0;
            for (int j = 0; j < filterOrder; j++) {
                filtered[i] += signal[i - j] * filter[j];
            }
        }

        // Step 3: Apply Hanning window
        for (size_t i = 0; i < filtered.size(); i++) {
            double window = 0.5 * (1 - cos(2 * M_PI * i / (filtered.size() - 1)));
            filtered[i] *= window;
        }

        // Step 4: Perform FFT
        cv::Mat signalMat(filtered);
        cv::Mat fftInput;
        signalMat.convertTo(fftInput, CV_32F);
        cv::Mat fftResult;
        cv::dft(fftInput, fftResult, cv::DFT_COMPLEX_OUTPUT);

        // Step 5: Calculate power spectrum
        std::vector<double> magnitudes;
        for (int i = 0; i < fftResult.rows / 2; i++) {
            double real = fftResult.at<float>(i, 0);
            double imag = fftResult.at<float>(i, 1);
            magnitudes.push_back(real * real + imag * imag);
        }

        // Step 6: Find peaks in the expected heart rate range
        const double minFreq = 1.0;  // 60 BPM
        const double maxFreq = 2.0;  // 120 BPM
        int minIndex = (minFreq * BUFFER_SIZE) / samplingRate;
        int maxIndex = (maxFreq * BUFFER_SIZE) / samplingRate;

        // Find all peaks
        std::vector<std::pair<double, int>> peaks;
        for (int i = minIndex + 1; i < maxIndex - 1; i++) {
            if (magnitudes[i] > magnitudes[i-1] && magnitudes[i] > magnitudes[i+1]) {
                // Quadratic interpolation for better frequency estimation
                double alpha = magnitudes[i-1];
                double beta = magnitudes[i];
                double gamma = magnitudes[i+1];
                double p = 0.5 * (alpha - gamma) / (alpha - 2*beta + gamma);
                double interpolatedIndex = i + p;

                peaks.push_back({magnitudes[i], static_cast<int>(interpolatedIndex)});
            }
        }

        // Sort peaks by magnitude
        std::sort(peaks.begin(), peaks.end(), std::greater<>());

        if (peaks.empty()) {
            return 0.0;
        }

        // Use the strongest peak
        double peakIndex = peaks[0].second;

        // Convert to BPM with interpolated frequency
        double frequency = (peakIndex * samplingRate) / BUFFER_SIZE;
        double bpm = frequency * 60.0;

        // Apply temporal smoothing
        static double lastValidBPM = 0.0;
        if (lastValidBPM == 0.0) {
            lastValidBPM = bpm;
        } else {
            // Smoothing with outlier rejection
            if (abs(bpm - lastValidBPM) > 15.0) {
                // Large jump - probably noise
                bpm = lastValidBPM;
            } else {
                bpm = 0.7 * lastValidBPM + 0.3 * bpm;
                lastValidBPM = bpm;
            }
        }

        return bpm;
    }

#if defined(Q_OS_ANDROID)
    void requestAndroidPermissions();
#endif

    QGraphicsPixmapItem pixmap;
    QMediaCaptureSession m_captureSession;
    QScopedPointer<QCamera> m_camera;
    Frames *m_frames{nullptr};
    RPPG *rppg{nullptr};
    bool frontCamEnabled = false;
    Ui::MainWindow *ui;
    static inline MainWindow* m_instance = nullptr;

signals:
    void cameraPermissionGranted();

private slots:
    void processFrame(QVideoFrame&);
    void processImage(QImage&);
    void printInfo(QString);
    void printValue(QString);
    void onCameraListUpdated(const QStringList &);
    void on_pushExit_clicked();
    void on_cameraComboBox_currentIndexChanged(int index);
#if defined(Q_OS_ANDROID)
    void onCameraPermissionGranted();
#endif
};

#endif // MAINWINDOW_H
