#-------------------------------------------------
#
# Project created by QtCreator 2019-08-14T09:37:24
#
#-------------------------------------------------

QT       += core gui sql

QT += core gui sql dbus concurrent svg x11extras printsupport
qtHaveModule(opengl): QT += opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = deepin-album
TEMPLATE = app
QT += dtkwidget
PKGCONFIG += dtkwidget
LIBS += -lfreeimage

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

DEFINES += QMAKE_TARGET=\\\"$$TARGET\\\" QMAKE_VERSION=\\\"$$VERSION\\\"
DEFINES += LITE_DIV
isEmpty(QMAKE_ORGANIZATION_NAME) {
    DEFINES += QMAKE_ORGANIZATION_NAME=\\\"deepin\\\"
}

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11 link_pkgconfig

include (frame/frame.pri)
include (module/modules.pri)
include (widgets/widgets.pri)
include (utils/utils.pri)
include (controller/controller.pri)

SOURCES += \
        main.cpp \
        mainwindow.cpp \
        albumview/albumview.cpp \
        allpicview/allpicview.cpp \
        timelineview/timelineview.cpp \
        dbmanager/dbmanager.cpp \
    application.cpp \
    dialogs/albumcreatedialog.cpp \
    dialogs/dialog.cpp \
    searchview/searchview.cpp \
    widgets/albumlefttabitem.cpp \
    importview/importview.cpp

HEADERS += \
        mainwindow.h \
        albumview/albumview.h \
        allpicview/allpicview.h \
        timelineview/timelineview.h \
        dbmanager/dbmanager.h \
    application.h \
    dialogs/albumcreatedialog.h \
    dialogs/dialog.h \
    searchview/searchview.h \
    widgets/albumlefttabitem.h \
    importview/importview.h

isEmpty(BINDIR):BINDIR=/usr/bin
isEmpty(APPDIR):APPDIR=/usr/share/applications
isEmpty(DSRDIR):DSRDIR=/usr/share/album

target.path = $$INSTROOT$$BINDIR

icon_files.path = /usr/share/icons/hicolor/scalable/apps
icon_files.files = $$PWD/resources/images/other/deepin-photo-album.svg


INSTALLS += target translations icon_files

RESOURCES += \
    resources.qrc
