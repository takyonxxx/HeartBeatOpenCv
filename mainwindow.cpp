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

float MainWindow::getPulseValue(const Mat& maskedRed) {
    Scalar mean = cv::mean(maskedRed);
    return mean[0];  // Return average intensity of the masked region
}

void MainWindow::processFrame(QVideoFrame &frame)
{
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
            if (!frameRGB.empty()) {
                std::vector<Mat> channels;
                split(frameRGB, channels);
                Mat redChannel = channels[2];

                Mat mask = Mat::zeros(redChannel.size(), CV_8UC1);
                int centerX = redChannel.cols / 2;
                int centerY = redChannel.rows / 2;
                int roiSize = std::min(redChannel.cols, redChannel.rows) / 3;
                cv::circle(mask, Point(centerX, centerY), roiSize, Scalar(255), -1);

                Mat maskedRed;
                redChannel.copyTo(maskedRed, mask);

                double rawHeartRate = calculateInstantHeartRate(maskedRed, mask);
                heartRate = rawHeartRate; // Kalman filtresi kaldırıldı

                static std::vector<float> pulseBuffer;
                const int bufferSize = 200;

                pulseBuffer.push_back(heartRate);
                if (pulseBuffer.size() > bufferSize) {
                    pulseBuffer.erase(pulseBuffer.begin());
                }

                Mat displayFrame;
                cvtColor(maskedRed, displayFrame, COLOR_GRAY2BGR);

                if (pulseBuffer.size() > 1) {
                    auto minmax = std::minmax_element(pulseBuffer.begin(), pulseBuffer.end());
                    float minVal = *minmax.first;
                    float maxVal = *minmax.second;
                    float range = maxVal - minVal;

                    int lineY = centerY;
                    int startX = centerX - roiSize;
                    int endX = centerX + roiSize;

                    // İnce grid çizgileri
                    for(int y = lineY - roiSize/3; y <= lineY + roiSize/3; y += roiSize/10) {
                        line(displayFrame, Point(startX, y), Point(endX, y), Scalar(50, 50, 50), 1);
                    }
                    for(int x = startX; x <= endX; x += roiSize/10) {
                        line(displayFrame, Point(x, lineY - roiSize/3), Point(x, lineY + roiSize/3), Scalar(50, 50, 50), 1);
                    }

                    // Ana referans çizgisi
                    line(displayFrame, Point(startX, lineY), Point(endX, lineY), Scalar(100, 100, 100), 2);

                    std::vector<Point> points;
                    int segmentWidth = (endX - startX) / pulseBuffer.size();

                    // Sinyal yumuşatma için cubic spline noktaları
                    std::vector<Point> splinePoints;
                    const int SMOOTHING_WINDOW = 5;

                    for (size_t i = 0; i < pulseBuffer.size(); ++i) {
                        // Hareketli ortalama ile yumuşatma
                        double smoothedValue = 0;
                        int count = 0;

                        for (int j = -SMOOTHING_WINDOW; j <= SMOOTHING_WINDOW; j++) {
                            if (i + j >= 0 && i + j < pulseBuffer.size()) {
                                smoothedValue += pulseBuffer[i + j];
                                count++;
                            }
                        }
                        smoothedValue /= count;

                        float normalizedValue = (smoothedValue - minVal) / range;
                        int x = startX + i * segmentWidth;
                        int y = lineY - normalizedValue * (roiSize/3);
                        splinePoints.push_back(Point(x, y));
                    }

                    // Cubic spline interpolasyon için ara noktalar
                    for (size_t i = 0; i < splinePoints.size() - 1; ++i) {
                        Point p0 = (i > 0) ? splinePoints[i-1] : splinePoints[i];
                        Point p1 = splinePoints[i];
                        Point p2 = splinePoints[i+1];
                        Point p3 = (i < splinePoints.size()-2) ? splinePoints[i+2] : p2;

                        // Her iki nokta arasında 5 ara nokta oluştur
                        for (int t = 0; t < 5; ++t) {
                            float tt = t / 5.0f;

                            // Catmull-Rom spline formülü
                            float tt2 = tt * tt;
                            float tt3 = tt2 * tt;

                            float x = 0.5f * ((2.0f * p1.x) +
                                              (-p0.x + p2.x) * tt +
                                              (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * tt2 +
                                              (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * tt3);

                            float y = 0.5f * ((2.0f * p1.y) +
                                              (-p0.y + p2.y) * tt +
                                              (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * tt2 +
                                              (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * tt3);

                            points.push_back(Point(round(x), round(y)));
                        }
                        points.push_back(p2);

                        // Peak noktalarında spike ekle
                        if (i > 0 && i < splinePoints.size()-1) {
                            if (splinePoints[i].y < splinePoints[i-1].y &&
                                splinePoints[i].y < splinePoints[i+1].y) {
                                int x = splinePoints[i].x;
                                int y = splinePoints[i].y;
                                points.push_back(Point(x, y - 12));
                                points.push_back(Point(x + 2, y + 8));
                                points.push_back(Point(x + 4, y));
                            }
                        }
                    }

                    // Sinyal çizimi
                    Scalar signalColor(0, 255, 255); // Sabit sarı renk
                    for (size_t i = 0; i < points.size() - 1; ++i) {
                        line(displayFrame, points[i], points[i + 1], signalColor, 2);
                    }
                }

                // Daire çizimi
                cv::circle(displayFrame, Point(centerX, centerY), roiSize, Scalar(0, 255, 255), 2);

                displayFrame.copyTo(frameRGB, mask);

                // BPM text
                std::string bpmText = std::to_string(static_cast<int>(std::round(heartRate))) + " BPM";
                int fontFace = FONT_HERSHEY_DUPLEX;
                double fontScale = 1.5;
                int thickness = 2;
                int baseline = 0;
                Size textSize = getTextSize(bpmText, fontFace, fontScale, thickness, &baseline);
                Point textOrg(centerX - textSize.width / 2, centerY - roiSize/2);

                putText(frameRGB, bpmText, textOrg, fontFace, fontScale, Scalar(0, 0, 0), thickness + 1);
                putText(frameRGB, bpmText, textOrg, fontFace, fontScale, Scalar(255, 255, 255), thickness);
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

        processImage(img_processed);
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
    return bpmKalman.update(newBpm);
}

double MainWindow::calculateInstantHeartRate(const cv::Mat& currentFrame, const cv::Mat& mask) {
    static std::deque<double> intensities;
    static const int BUFFER_SIZE = 100;
    static double lastValidBpm = 0.0;

    cv::Scalar mean = cv::mean(currentFrame, mask);
    double intensity = mean[0];

    // Store intensity in buffer
    if (intensities.size() >= BUFFER_SIZE) {
        intensities.pop_front();
    }

    intensities.push_back(intensity);

    if (intensities.size() < BUFFER_SIZE) {
        return lastValidBpm;
    }

    // Calculate threshold
    double maxVal = *std::max_element(intensities.begin(), intensities.end());
    double minVal = *std::min_element(intensities.begin(), intensities.end());
    double mean_val = std::accumulate(intensities.begin(), intensities.end(), 0.0) / intensities.size();
    double threshold = mean_val + 0.225 * (maxVal - minVal); // Reduced threshold factor for sensitivity

    // Detect peaks
    std::vector<int> peakIndices;
    const int minDistance = 20;
    const int maxDistance = 30;

    for (size_t i = 1; i < intensities.size() - 1; i++) {
        if (!peakIndices.empty() && i - peakIndices.back() < minDistance) {
            continue;
        }
        if (intensities[i] > threshold &&
            intensities[i] > intensities[i - 1] &&
            intensities[i] > intensities[i + 1]) {
            peakIndices.push_back(i);
        }
    }

    // Calculate BPM
    double bpm = 0.0;
    if (peakIndices.size() >= 2) {
        std::vector<double> intervals;
        for (size_t i = 1; i < peakIndices.size(); i++) {
            int interval = peakIndices[i] - peakIndices[i - 1];
            qDebug() << interval;
            if (interval >= minDistance && interval <= maxDistance) {
                intervals.push_back(interval);
            }
        }
        if (!intervals.empty()) {
            double avgInterval = std::accumulate(intervals.begin(), intervals.end(), 0.0) / intervals.size();
            bpm = (30.0 * 57.0) / avgInterval; // Convert frame interval to BPM
        }
    }

    if (bpm >= 70 && bpm <= 90) { // Adjusted valid range
        lastValidBpm = getFilteredBpm(bpm);
    }

    return lastValidBpm;
}
