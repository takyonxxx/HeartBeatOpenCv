#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_instance = this;

    setWindowTitle("HeartRate Monitor Pro");
    setStyleSheet("background-color: #1E1E2E;");

    setupUI();
    initializeRPPG();

#if defined(Q_OS_ANDROID)
    requestAndroidPermissions();
#else
    setupCamera();
#endif
}

void MainWindow::setupUI()
{
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    ui->textBpm->setStyleSheet("font-size: 36pt; font: bold; color: #3498db; background-color: #2D2D44; border-radius: 10px; padding: 10px;");
    ui->m_textStatus->setStyleSheet("font-size: 16pt; color: #E0E0E0; background-color: #2D2D44; border-radius: 8px;");
#else
    ui->textBpm->setStyleSheet("font-size: 48pt; font: bold; color: #3498db; background-color: #2D2D44; border-radius: 15px; padding: 15px;");
    ui->m_textStatus->setStyleSheet("font-size: 20pt; color: #E0E0E0; background-color: #2D2D44; border-radius: 10px;");
#endif

    ui->graphicsView->setStyleSheet("border: 2px solid #3D3D59; border-radius: 15px; background-color: #2D2D44;");
    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->cameraComboBox->setStyleSheet("font-size: 16pt; color: #FFFFFF; background-color: #4B4B6E; border-radius: 8px; padding: 5px;");
    ui->pushExit->setStyleSheet("font-size: 18pt; font: bold; color: #FFFFFF; background-color: #FF4B4B; border-radius: 8px; padding: 8px;");

    ui->graphicsView->setScene(new QGraphicsScene(this));
    QScreen *primaryScreen = QGuiApplication::primaryScreen();
    QRect screenGeometry = primaryScreen->availableGeometry();
    setGeometry(screenGeometry);
    ui->graphicsView->scene()->addItem(&pixmap);

    QStringList files = {
        ":/opencv/deploy.prototxt",
        ":/opencv/haarcascade_frontalface_alt.xml",
        ":/opencv/res10_300x300_ssd_iter_140000.caffemodel"
    };
    for (const QString &fileName : files) {
        createFile(fileName);
    }
}

void MainWindow::initializeRPPG()
{
#if defined(Q_OS_ANDROID)
    connect(this, &MainWindow::cameraPermissionGranted, this, &MainWindow::onCameraPermissionGranted);
#endif

    rppg = new RPPG();
    connect(rppg, &RPPG::sendInfo, this, &MainWindow::printInfo);
    rppg->load(0, HAAR_CLASSIFIER_PATH, DNN_PROTO_PATH, DNN_MODEL_PATH);
}

void MainWindow::setupCamera()
{
    if (!m_frames) {
        m_frames = new Frames();
        m_frames->setRunning(false);
        connect(m_frames, &Frames::sendInfo, this, &MainWindow::printInfo);
        connect(m_frames, &Frames::frameCaptured, this, &MainWindow::processFrame);
        connect(m_frames, &Frames::cameraListUpdated, this, &MainWindow::onCameraListUpdated);
        m_frames->initializeCameraDevices();
    }
}

#if defined(Q_OS_ANDROID)
void MainWindow::requestAndroidPermissions()
{
    QJniObject activity = QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative",
        "activity",
        "()Landroid/app/Activity;"
        );

    if (!activity.isValid())
        return;

    jint permission = activity.callMethod<jint>(
        "checkSelfPermission",
        "(Ljava/lang/String;)I",
        QJniObject::fromString("android.permission.CAMERA").object()
        );

    if (permission != 0) { // PERMISSION_GRANTED = 0
        QJniObject::callStaticMethod<void>(
            "org/tbiliyor/heartbeat/MainActivity",
            "requestPermission",
            "(Ljava/lang/String;)V",
            QJniObject::fromString("android.permission.CAMERA").object()
            );
    } else {
        setupCamera();
    }
}

void MainWindow::onCameraPermissionGranted()
{
    setupCamera();
}
#endif

void MainWindow::createFile(const QString &fileName)
{
    QString data;
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Error: Could not open file for reading: " << fileName;
        return;
    } else {
        data = file.readAll();
        file.close();
    }

    QString filename = fileName.mid(fileName.lastIndexOf("/") + 1);
    QString tempFilePath;

#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    QString iosWritablePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    tempFilePath = iosWritablePath + "/" + filename;
#else
    tempFilePath = filename;
#endif

    QFile temp(tempFilePath);

    if (temp.exists()) {
        qDebug() << "Warning: File already exists: " << tempFilePath;
        return;
    }

    if (temp.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&temp);
        stream << data;
        temp.close();
        qDebug() << "Data written to file: " << tempFilePath;
    } else {
        qDebug() << "Error: Could not open file for writing: " << tempFilePath;
    }
}

void MainWindow::processFrame(QVideoFrame &frame)
{
    frontCamEnabled=false;

    if (frame.isValid()) {
        double heartRate = 0.0;
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

        if(!frontCamEnabled)
        {
            if(!frameRGB.empty()) {

                // Extract red channel
                std::vector<Mat> channels;
                split(frameRGB, channels);
                Mat redChannel = channels[2];  // BGR format, so red is index 2

                // Create mask for ROI (center region)
                Mat mask = Mat::zeros(redChannel.size(), CV_8UC1);
                int centerX = redChannel.cols / 2;
                int centerY = redChannel.rows / 2;
                int roiSize = std::min(redChannel.cols, redChannel.rows) / 3;
                cv::circle(mask, Point(centerX, centerY), roiSize, Scalar(255), -1);

                // Apply the mask to the red channel
                Mat maskedRed;
                redChannel.copyTo(maskedRed, mask);

                // Calculate heart rate from the red channel
                heartRate = calculateInstantHeartRate(maskedRed, mask);

                // Visualize the masked red channel
                Mat displayFrame;
                cvtColor(maskedRed, displayFrame, COLOR_GRAY2BGR);
                displayFrame.copyTo(frameRGB, mask);
            }
        }
        else
        {
            Mat frameGray;
            cvtColor((InputArray)frameRGB, (OutputArray)frameGray, COLOR_BGR2GRAY);
            equalizeHist((InputArray)frameGray, (OutputArray)frameGray);

            heartRate = rppg->processFrame(frameRGB, frameGray);
        }

        std::stringstream ss;
        ss << std::fixed << std::setprecision(0)
           << heartRate << " BPM";

        printValue(ss.str().c_str());

        QImage img_processed((uchar*)frameRGB.data, frameRGB.cols, frameRGB.rows,
                             frameRGB.step, QImage::Format_RGB888);
        pixmap.setPixmap(QPixmap::fromImage(img_processed));
        ui->graphicsView->fitInView(&pixmap, Qt::KeepAspectRatioByExpanding);
    }
}

void MainWindow::processImage(QImage &img_face)
{
    pixmap.setPixmap(QPixmap::fromImage(img_face));
    ui->graphicsView->fitInView(&pixmap, Qt::KeepAspectRatio);
}

void MainWindow::printInfo(QString info)
{
    ui->m_textStatus->setText(info);
}

void MainWindow::printValue(QString bpm)
{
    ui->textBpm->setText(bpm);
}

void MainWindow::on_pushExit_clicked()
{
    qApp->exit();
}

void MainWindow::onCameraListUpdated(const QStringList &cameraDevices)
{
    ui->cameraComboBox->clear();
    ui->cameraComboBox->addItems(cameraDevices);
}

void MainWindow::on_cameraComboBox_currentIndexChanged(int index)
{
    frontCamEnabled = false;
    m_frames->setRunning(false);

    QString selectedText = ui->cameraComboBox->itemText(index);
    m_frames->setCamera(selectedText);
    if(selectedText.contains("Back"))
        m_frames->setFlash(true);
    else
    {
        m_frames->setFlash(false);
        frontCamEnabled = true;
    }
}

MainWindow::~MainWindow()
{
    if(m_frames)
        delete m_frames;

    if(rppg)
        delete rppg;

    delete ui;
}

#if defined(Q_OS_ANDROID)
extern "C" {
JNIEXPORT void JNICALL
Java_org_tbiliyor_heartbeat_MainActivity_notifyCameraPermissionGranted(JNIEnv *env, jobject obj)
{
    if (MainWindow::instance()) {
        QMetaObject::invokeMethod(MainWindow::instance(), "cameraPermissionGranted");
    }
}
}
#endif

double MainWindow::getFilteredBpm(double newBpm) {
    static std::deque<double> recentBpms;
    static const int BPM_HISTORY_SIZE = 10;

    // Update BPM history
    if (recentBpms.size() >= BPM_HISTORY_SIZE) {
        recentBpms.pop_front();
    }
    recentBpms.push_back(newBpm);

    // Calculate median BPM with bounds checking
    if (recentBpms.empty()) {
        return newBpm;
    }

    std::vector<double> sortedBpms(recentBpms.begin(), recentBpms.end());
    std::sort(sortedBpms.begin(), sortedBpms.end());
    double medianBpm;
    if (sortedBpms.size() % 2 == 0 && sortedBpms.size() > 1) {
        size_t mid = sortedBpms.size() / 2;
        medianBpm = (sortedBpms[mid-1] + sortedBpms[mid]) / 2.0;
    } else if (!sortedBpms.empty()) {
        medianBpm = sortedBpms[sortedBpms.size() / 2];
    } else {
        medianBpm = newBpm;
    }

    // Apply exponential smoothing
    static double smoothedBpm = medianBpm;
    smoothedBpm = 0.9 * smoothedBpm + 0.1 * medianBpm;

    return smoothedBpm;
}

double MainWindow::calculateInstantHeartRate(const cv::Mat& currentFrame, const cv::Mat& mask) {
    static std::deque<double> intensities;
    static std::deque<double> recentBpms;
    static const int BUFFER_SIZE = 45;
    static const int BPM_HISTORY_SIZE = 30;  // Much longer history
    static double lastValidBpm = 75.0;

    // Get current intensity
    Scalar mean = cv::mean(currentFrame, mask);
    double intensity = mean[0] * 1000000.0;
    qDebug() << "Current intensity (scaled):" << intensity;

    // Update buffer
    if (intensities.size() >= BUFFER_SIZE) {
        intensities.pop_front();
    }
    intensities.push_back(intensity);

    if (intensities.size() < BUFFER_SIZE) {
        return lastValidBpm;
    }

    // Calculate moving average of signal
    std::vector<double> smoothedIntensities;
    const int smoothingWindow = 3;
    for (size_t i = smoothingWindow; i < intensities.size() - smoothingWindow; i++) {
        double sum = 0;
        for (int j = -smoothingWindow; j <= smoothingWindow; j++) {
            sum += intensities[i + j];
        }
        smoothedIntensities.push_back(sum / (2 * smoothingWindow + 1));
    }

    // Calculate signal statistics on smoothed signal
    double maxIntensity = *std::max_element(smoothedIntensities.begin(), smoothedIntensities.end());
    double minIntensity = *std::min_element(smoothedIntensities.begin(), smoothedIntensities.end());
    double range = maxIntensity - minIntensity;

    const int minDistance = 18;  // Increased for more stable rate ~80 BPM
    std::vector<int> peakIndices;

    // Peak detection on smoothed signal
    for (size_t i = 3; i < smoothedIntensities.size() - 3; i++) {
        if (!peakIndices.empty() && i - peakIndices.back() < minDistance) {
            continue;
        }

        bool isPeak = true;
        for (int j = -3; j <= 3; j++) {
            if (j != 0) {
                if (i + j < smoothedIntensities.size() &&
                    smoothedIntensities[i] <= smoothedIntensities[i + j]) {
                    isPeak = false;
                    break;
                }
            }
        }

        if (isPeak) {
            // Calculate peak prominence
            double prominence = 0;
            if (i > 0 && i < smoothedIntensities.size() - 1) {
                double leftMin = *std::min_element(smoothedIntensities.begin() + std::max(0, (int)i - minDistance),
                                                   smoothedIntensities.begin() + i);
                double rightMin = *std::min_element(smoothedIntensities.begin() + i + 1,
                                                    smoothedIntensities.begin() + std::min((int)smoothedIntensities.size(), (int)i + minDistance));
                prominence = smoothedIntensities[i] - std::max(leftMin, rightMin);
            }

            if (prominence > range * 0.1) {  // Reduced threshold for smoothed signal
                peakIndices.push_back(i);
                qDebug() << "Peak found at index" << i << "height:" << smoothedIntensities[i];
            }
        }
    }

    // Calculate BPM
    double bpm = 0;
    if (peakIndices.size() >= 2) {
        std::vector<double> intervals;
        for (size_t i = 1; i < peakIndices.size(); i++) {
            int interval = peakIndices[i] - peakIndices[i-1];
            if (interval >= minDistance) {
                intervals.push_back(interval);
            }
        }

        if (!intervals.empty()) {
            // Calculate trimmed mean of intervals (exclude outliers)
            std::sort(intervals.begin(), intervals.end());
            size_t trimCount = intervals.size() / 4;  // Trim 25% from each end
            double sum = 0;
            int count = 0;
            for (size_t i = trimCount; i < intervals.size() - trimCount; i++) {
                sum += intervals[i];
                count++;
            }
            if (count > 0) {
                double avgInterval = sum / count;
                bpm = (30.0 * 60.0) / avgInterval;
            }
        }
    }

    qDebug() << "Raw BPM:" << bpm;

    // Strict BPM validation and stronger smoothing
    if (bpm >= 70 && bpm <= 90) {  // Narrower range
        if (recentBpms.size() >= BPM_HISTORY_SIZE) {
            recentBpms.pop_front();
        }
        recentBpms.push_back(bpm);

        if (recentBpms.size() >= 5) {  // Need minimum history
            std::vector<double> sortedBpms(recentBpms.begin(), recentBpms.end());
            std::sort(sortedBpms.begin(), sortedBpms.end());

            // Use trimmed mean of recent BPMs
            size_t trimCount = sortedBpms.size() / 4;
            double sum = 0;
            int count = 0;
            for (size_t i = trimCount; i < sortedBpms.size() - trimCount; i++) {
                sum += sortedBpms[i];
                count++;
            }

            if (count > 0) {
                double meanBpm = sum / count;
                // Very strong smoothing
                lastValidBpm = 0.975 * lastValidBpm + 0.025 * meanBpm;
            }
        }
    }

    return std::round(lastValidBpm);  // Round to nearest whole number
}
