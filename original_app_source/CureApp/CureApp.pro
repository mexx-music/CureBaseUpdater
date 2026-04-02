TARGET = CureAppPro
QT += qml

#QT += quick
#QT +=widgets
#QT+= charts
QT += bluetooth

android: QT += androidextras

CONFIG += c++11

DEFINES += "uECC_PLATFORM=0"

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

#INCLUDEPATH = ../mcharts/examples
DEFINES += IsCureAppPro=false

SOURCES += \
        Programs.cpp \
        availablecuredevices.cpp \
        availableitemsmodel.cpp \
        bleuart.cpp \
        curebasestatemachine.cpp \
        dataprovider.cpp \
        kmackay-micro-ecc-24c60e2/uECC.c \
        main.cpp \
        miniz.c \
        myownprograms.cpp \
        myprogramsmodel.cpp \
        programmplayer.cpp \
        qmlhelpers.cpp \
        settingsmanager.cpp \
        utility.cpp\
        aes/aes.c

RESOURCES += qml.qrc

TRANSLATIONS += \
    CureAppAndroidQuirks_de.ts \
    CureApp_de.ts
CONFIG += lrelease
CONFIG += embed_translations

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

VERSION = 1.0.13.2
ANDROID_VERSION_CODE = 010013002
ANDROID_VERSION_NAME = "1.0.13.2"

HEADERS += \
    Programs.h \
    aes/aes.h \
    aes/aes.hpp \
    availablecuredevices.h \
    availableitemsmodel.h \
    bleuart.h \
    curebasestatemachine.h \
    dataprovider.h \
    kmackay-micro-ecc-24c60e2/uECC.h \
    miniz.h \
    myownprograms.h \
    myprogramsmodel.h \
    programmplayer.h \
    qmlhelpers.h \
    settingsmanager.h \
    utility.h

DISTFILES += \
    android/AndroidManifest.xml \
    android/AndroidManifest.xml \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/build.gradle \
    android/build.gradle \
    android/gradle.properties \
    android/gradle.properties \
    android/gradle.properties \
    android/gradle.properties \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew \
    android/gradlew \
    android/gradlew.bat \
    android/gradlew.bat \
    android/gradlew.bat \
    android/res/values/libs.xml \
    android/res/values/libs.xml \
    android/res/values/libs.xml

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

include(../mcharts/mcharts.pri)
android: include(/home/hans/Android/Sdk/android_openssl/openssl.pri)
