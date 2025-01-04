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

    intensityBuffer.reserve(BUFFER_SIZE);
    lastFrameTime = std::chrono::steady_clock::now();

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
        setupCamera(); // Call setupCamera directly if permission already granted
    }
}

void MainWindow::onCameraPermissionGranted()
{
    setupCamera(); // Remove timer delay, call setupCamera directly
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
    if (frame.isValid()) {

        // Calculate frame interval
        auto currentTime = std::chrono::steady_clock::now();
        frameInterval = std::chrono::duration<double>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;
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
                // Convert to HSV color space for better color analysis
                Mat frameHSV;
                cvtColor(frameRGB, frameHSV, COLOR_BGR2HSV);

                // Define red color range in HSV
                Scalar lowerRed1(0, 70, 50);
                Scalar upperRed1(10, 255, 255);
                Scalar lowerRed2(170, 70, 50);
                Scalar upperRed2(180, 255, 255);

                // Create masks for red regions
                Mat mask1, mask2, redMask;
                inRange(frameHSV, lowerRed1, upperRed1, mask1);
                inRange(frameHSV, lowerRed2, upperRed2, mask2);
                bitwise_or(mask1, mask2, redMask);

                // Apply morphological operations
                Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
                morphologyEx(redMask, redMask, MORPH_OPEN, kernel);
                morphologyEx(redMask, redMask, MORPH_CLOSE, kernel);

                // Calculate intensity
                Scalar mean = cv::mean(frameRGB, redMask);
                double redIntensity = mean[2] * 0.7 + mean[1] * 0.2 + mean[0] * 0.1;

                // Store intensity for heart rate calculation
                updateIntensityBuffer(redIntensity);

                // Calculate measurements
                //double glucoseLevel = calculateGlucoseLevel(redIntensity);
                heartRate = calculateHeartRate();
                std::stringstream ss;
                ss << std::fixed << std::setprecision(0)
                   << "Heart Rate: " << heartRate << " BPM";

                printValue(ss.str().c_str());
            }
        }
        else
        {
            Mat frameGray;
            cvtColor((InputArray)frameRGB, (OutputArray)frameGray, COLOR_BGR2GRAY);
            equalizeHist((InputArray)frameGray, (OutputArray)frameGray);

            heartRate = rppg->processFrame(frameRGB, frameGray);
                std::stringstream ss;
                ss << std::fixed << std::setprecision(0)
                   << "Heart Rate: " << heartRate << " BPM";
                printValue(ss.str().c_str());
        }

        // Display processed image
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
