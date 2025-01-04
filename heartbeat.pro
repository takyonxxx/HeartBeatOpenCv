QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TEMPLATE = app
TARGET = HeartBeat

QT += multimedia
QT += multimediawidgets

QMAKE_TARGET_BUNDLE_PREFIX = tbiliyor.com
PRODUCT_BUNDLE_IDENTIFIER = $${QMAKE_TARGET_BUNDLE_PREFIX}.heartrate


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

win32 {
    message("Win32 enabled")
    RC_ICONS += $$\PWD\icons\heart-rate.png
    LIBS += -L$$(OPENCV_DIR)/lib -lopencv_world452
    INCLUDEPATH += C:/opencv/build/include
}

unix:!macx:!ios:!android {
    message("linux enabled")
    #sudo apt install libopencv-dev
    INCLUDEPATH += /usr/lib
    INCLUDEPATH += /usr/local/lib
    INCLUDEPATH += /usr/local/include/opencv4
    INCLUDEPATH += /usr/include/opencv4

    LIBS += -lopencv_core -lopencv_dnn -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_objdetect -lopencv_video -lopencv_videoio
}

macos {
    message("macx enabled")    
    QMAKE_INFO_PLIST = ./macos/Info.plist
    QMAKE_ASSET_CATALOGS = $$PWD/macos/Assets.xcassets
    QMAKE_ASSET_CATALOGS_APP_ICON = "AppIcon"

    INCLUDEPATH += /usr/local/Cellar/opencv/4.10.0_12/include/opencv4
    LIBS += -L/usr/local/Cellar/opencv/4.10.0_12/lib -lopencv_core -lopencv_dnn -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_objdetect -lopencv_video -lopencv_videoio
 }

INCLUDEPATH += /Users/turkaybiliyor/Desktop/buildHeartBeat/opencv2.framework/Versions/A/Headers

ios {

    message("ios enabled")
    QMAKE_INFO_PLIST = ./ios/Info.plist
    QMAKE_ASSET_CATALOGS = $$PWD/ios/Assets.xcassets
    QMAKE_ASSET_CATALOGS_APP_ICON = "AppIcon"

    # OPENCV_DIR = /Users/turkaybiliyor/Desktop/buildHeartBeat

    # INCLUDEPATH += $${OPENCV_DIR}/opencv2.framework/Headers
    # LIBS += -F$${OPENCV_DIR} -framework opencv2
    # QMAKE_LFLAGS += -F$${OPENCV_DIR}

    # # Copy framework to app bundle
    # opencv2.files = $${OPENCV_DIR}/opencv2.framework
    # opencv2.path = Frameworks
    # QMAKE_BUNDLE_DATA += opencv2

    # # Post-processing command without concatenation
    # QMAKE_POST_LINK = cp -f $$PWD/ios/opencv2_Info.plist $${OPENCV_DIR}/opencv2.framework/Info.plist
}

android {   

    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

    # Your existing OpenCV configurations
    INCLUDEPATH += $$PWD/OpenCV-android-sdk/sdk/native/jni/include
    DEPENDPATH += $$PWD/OpenCV-android-sdk/sdk/native/jni/include

    contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
        LIBS += -L$$PWD/OpenCV-android-sdk/sdk/native/libs/armeabi-v7a -lopencv_java4
        LIBS += \
            -L$$PWD/OpenCV-android-sdk/sdk/native/staticlibs/armeabi-v7a \
            -lopencv_ml\
            -lopencv_objdetect\
            -lopencv_photo\
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
}

#sudo apt install libopencv-dev python3-opencv
#brew install opencv

RESOURCES += \
    resources.qrc

DISTFILES += \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle.properties \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/res/drawable-hdpi/icon.png \
    android/res/drawable-ldpi/icon.png \
    android/res/drawable-mdpi/icon.png \
    android/res/drawable-xhdpi/icon.png \
    android/res/drawable-xxhdpi/icon.png \
    android/res/drawable-xxxhdpi/icon.png \
    android/src/org/tbiliyor/heartbeat/MainActivity.java \
    android/res/values/libs.xml \
    android/res/xml/qtprovider_paths.xml \
    ios/Assets.xcassets/AppIcon.appiconset/1024.png \
    ios/Assets.xcassets/AppIcon.appiconset/114.png \
    ios/Assets.xcassets/AppIcon.appiconset/120.png \
    ios/Assets.xcassets/AppIcon.appiconset/128.png \
    ios/Assets.xcassets/AppIcon.appiconset/16.png \
    ios/Assets.xcassets/AppIcon.appiconset/180.png \
    ios/Assets.xcassets/AppIcon.appiconset/256.png \
    ios/Assets.xcassets/AppIcon.appiconset/29.png \
    ios/Assets.xcassets/AppIcon.appiconset/32.png \
    ios/Assets.xcassets/AppIcon.appiconset/40.png \
    ios/Assets.xcassets/AppIcon.appiconset/512.png \
    ios/Assets.xcassets/AppIcon.appiconset/57.png \
    ios/Assets.xcassets/AppIcon.appiconset/58.png \
    ios/Assets.xcassets/AppIcon.appiconset/60.png \
    ios/Assets.xcassets/AppIcon.appiconset/64.png \
    ios/Assets.xcassets/AppIcon.appiconset/80.png \
    ios/Assets.xcassets/AppIcon.appiconset/87.png \
    ios/Assets.xcassets/AppIcon.appiconset/Contents.json \
    ios/Info.plist \
    ios/opencv2_Info.plist \
    macos/Assets.xcassets/AppIcon.appiconset/1024.png \
    macos/Assets.xcassets/AppIcon.appiconset/114.png \
    macos/Assets.xcassets/AppIcon.appiconset/120.png \
    macos/Assets.xcassets/AppIcon.appiconset/128.png \
    macos/Assets.xcassets/AppIcon.appiconset/16.png \
    macos/Assets.xcassets/AppIcon.appiconset/180.png \
    macos/Assets.xcassets/AppIcon.appiconset/256.png \
    macos/Assets.xcassets/AppIcon.appiconset/29.png \
    macos/Assets.xcassets/AppIcon.appiconset/32.png \
    macos/Assets.xcassets/AppIcon.appiconset/40.png \
    macos/Assets.xcassets/AppIcon.appiconset/512.png \
    macos/Assets.xcassets/AppIcon.appiconset/57.png \
    macos/Assets.xcassets/AppIcon.appiconset/58.png \
    macos/Assets.xcassets/AppIcon.appiconset/60.png \
    macos/Assets.xcassets/AppIcon.appiconset/64.png \
    macos/Assets.xcassets/AppIcon.appiconset/80.png \
    macos/Assets.xcassets/AppIcon.appiconset/87.png \
    macos/Assets.xcassets/AppIcon.appiconset/Contents.json \
    macos/Info.plist


