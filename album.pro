#-------------------------------------------------
#
# Project created by QtCreator 2019-08-14T09:37:24
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = album
TEMPLATE = app
PKGCONFIG += dtkwidget

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

DEFINES += QMAKE_TARGET=\\\"$$TARGET\\\" QMAKE_VERSION=\\\"$$VERSION\\\"

isEmpty(QMAKE_ORGANIZATION_NAME) {
    DEFINES += QMAKE_ORGANIZATION_NAME=\\\"deepin\\\"
}

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11 link_pkgconfig

SOURCES += \
        main.cpp \
        mainwindow.cpp \
        albumview/albumview.cpp \
        allpicview/allpicview.cpp \
        timelineview/timelineview.cpp \
        dbmanager/dbmanager.cpp \
        utils/baseutils.cpp \
    application.cpp \
    controller/globaleventfilter.cpp \
    controller/configsetter.cpp \
    controller/viewerthememanager.cpp \
    controller/signalmanager.cpp \
    utils/imageutils.cpp \
    widgets/thumbnaillistview.cpp \
    widgets/scrollbar.cpp \
    widgets/thumbnaildelegate.cpp

HEADERS += \
        mainwindow.h \
        albumview/albumview.h \
        allpicview/allpicview.h \
        timelineview/timelineview.h \
        dbmanager/dbmanager.h \
        utils/baseutils.h \
    application.h \
    controller/globaleventfilter.h \
    controller/signalmanager.h \
    controller/viewerthememanager.h \
    controller/configsetter.h \
    utils/imageutils.h \
    widgets/thumbnaillistview.h \
    widgets/scrollbar.h \
    widgets/thumbnaildelegate.h

#RESOURCES += \
#    resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
