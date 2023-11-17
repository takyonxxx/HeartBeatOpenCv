QT += core gui
QT += multimedia
QT += multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    RPPG.cpp \
    frames.cpp \
    main.cpp \
    mainwindow.cpp \
    opencv.cpp

HEADERS += \
    RPPG.hpp \
    frames.h \
    mainwindow.h \
    opencv.hpp

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

unix:!macx:!ios:!android{
    message("linux enabled")

    INCLUDEPATH += /usr/lib
    INCLUDEPATH += /usr/local/lib
    INCLUDEPATH += /usr/local/include/opencv4
    INCLUDEPATH += /usr/include/opencv4

    LIBS += -lopencv_core -lopencv_dnn -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_objdetect -lopencv_video -lopencv_videoio
}

macx{
    message("macx enabled")

    INCLUDEPATH += /opt/homebrew/Cellar/opencv/4.8.1_3/include/opencv4
    LIBS += -L/opt/homebrew/Cellar/opencv/4.8.1_3/lib -lopencv_core -lopencv_dnn -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_objdetect -lopencv_video -lopencv_videoio
    QMAKE_INFO_PLIST = ./macos/Info.plist
#    QMAKE_ASSET_CATALOGS = $$PWD/macos/Assets.xcassets
#    QMAKE_ASSET_CATALOGS_APP_ICON = "AppIcon"

#    INCLUDEPATH += /usr/local/opt/opencv/include/opencv4
#    LIBS += -L/usr/local/opt/opencv/lib -lopencv_core -lopencv_dnn -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_objdetect -lopencv_video -lopencv_videoio
}

win32{
    message("Win32 enabled")
    LIBS += -L$$(OPENCV_DIR)/lib -lopencv_world452
    INCLUDEPATH += C:/opencv/build/include
}

ios {
    message("ios enabled")
    QMAKE_INFO_PLIST = ./ios/Info.plist
    INCLUDEPATH += /Users/turkaybiliyor/opencvios/ios/opencv2.framework/Headers
    LIBS += \
             -F /Users/turkaybiliyor/opencvios/ios \
             -framework opencv2
}

android {

    INCLUDEPATH += $$PWD/OpenCV-android-sdk/sdk/native/jni/include
    DEPENDPATH += $$PWD/OpenCV-android-sdk/sdk/native/jni/include

    ANDROID_TARGET_ARCH = armeabi-v7a

    contains(ANDROID_TARGET_ARCH,armeabi-v7a) {

        LIBS += -L$$PWD/OpenCV-android-sdk/sdk/native/libs/armeabi-v7a -lopencv_java4
        LIBS += \
            -L$$PWD/OpenCV-android-sdk/sdk/native/staticlibs/armeabi-v7a \
            -lopencv_dnn\
            -lopencv_ml\
            -lopencv_objdetect\
            -lopencv_photo\
            -lopencv_stitching\
            -lopencv_video\
            -lopencv_calib3d\
            -lopencv_features2d\
            -lopencv_highgui\
            -lopencv_flann\
            -lopencv_videoio\
            -lopencv_imgcodecs\
            -lopencv_imgproc\
            -lopencv_core
        ANDROID_EXTRA_LIBS = $$PWD/OpenCV-android-sdk/sdk/native/libs/armeabi-v7a/libopencv_java4.so       
    }

    contains(ANDROID_TARGET_ARCH, arm64-v8a) {

        LIBS += -L$$PWD/OpenCV-android-sdk/sdk/native/libs/arm64-v8a -lopencv_java4
        LIBS += \
            -L$$PWD/OpenCV-android-sdk/sdk/native/staticlibs/arm64-v8a \
            -lopencv_dnn\
            -lopencv_ml\
            -lopencv_objdetect\
            -lopencv_photo\
            -lopencv_stitching\
            -lopencv_video\
            -lopencv_calib3d\
            -lopencv_features2d\
            -lopencv_highgui\
            -lopencv_flann\
            -lopencv_videoio\
            -lopencv_imgcodecs\
            -lopencv_imgproc\
            -lopencv_core
        ANDROID_EXTRA_LIBS = $$PWD/OpenCV-android-sdk/sdk/native/libs/arm64-v8a/libopencv_java4.so       
    }

    RESOURCES +=

    DISTFILES += \
        android/AndroidManifest.xml \
        android/build.gradle \
        android/gradle.properties \
        android/gradle/wrapper/gradle-wrapper.jar \
        android/gradle/wrapper/gradle-wrapper.properties \
        android/gradlew \
        android/gradlew.bat \
        android/res/values/libs.xml

    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

}
#sudo apt install libopencv-dev python3-opencv
#brew install opencv

RESOURCES += \
    resources.qrc
