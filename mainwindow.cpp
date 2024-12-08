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

        if(!frameRGB.empty()) {
            Mat frameGray;
            double bpm = 0.0;

            cvtColor((InputArray)frameRGB, (OutputArray)frameGray, COLOR_BGR2GRAY);
            equalizeHist((InputArray)frameGray, (OutputArray)frameGray);

            bpm = rppg->processFrame(frameRGB, frameGray);
            if(bpm < MAX_BPM) {
                std::stringstream ss;
                ss << std::fixed << std::setprecision(1) << bpm;
                printBpm(ss.str().c_str());
            }

            QImage img_face((uchar*)frameRGB.data, frameRGB.cols, frameRGB.rows, frameRGB.step, QImage::Format_RGB888);
            pixmap.setPixmap(QPixmap::fromImage(img_face));
            ui->graphicsView->fitInView(&pixmap, Qt::KeepAspectRatioByExpanding);
        }
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

void MainWindow::printBpm(QString bpm)
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
    QString selectedText = ui->cameraComboBox->itemText(index);
    m_frames->setCamera(selectedText);
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
