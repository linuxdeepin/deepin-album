// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include <DFileDialog>
#include <DSearchEdit>

#include <QTestEventList>
#include <QMap>
#include <QObject>
#include <QDialog>
#include <QStringList>
#include <QFileDialog>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QMimeDatabase>
#include <QCommandLineParser>

#define private public
#define protected public

#include "mainwindow.h"
#include "albumcreatedialog.h"
#include "test_qtestDefine.h"
#include "imginfodialog.h"
#include "exporter.h"
#include "utils/imageutils.h"
#include "utils/unionimage.h"
#include "imageengine/imageengineapi.h"
#include "ac-desktop-define.h"

#include <stub-tool/cpp-stub/stub.h>
#include <stub-tool/stub-ext/stubext.h>

struct CMOption {
    QString shortOption;
    QString longOption;
    QString description;
    QString valueName;
};

TEST(Exporter, test_Exporter_init)
{
    TEST_CASE_NAME("test_Exporter_init")
    MainWindow *w = dApp->getMainWindow();
    Q_UNUSED(w)
    Exporter::instance();
}

TEST(Exporter, exportImage_test)
{
    TEST_CASE_NAME("exportImage_test")

    //根据UT报告，补充测试部分分支

    //列表为空
    Exporter::instance()->exportImage({});

    //GIF
    Exporter::instance()->exportImage({ testPath_test + "/9a0a4c97036f61.gif" });

    //多个图片
    {
        int (*dlgexec)() = []() {
            return 1;
        };
        typedef int (*fptr)(QDialog *);
        fptr fptrexec = reinterpret_cast<fptr>(&QDialog::exec);  //obtaining an address
        Stub stub;
        stub.set(fptrexec, dlgexec);
        Exporter::instance()->exportImage({ testPath_test + "/9a0a4c97036f61.gif", testPath_test + "/2e5y8y.jpg" });
    }
}


TEST(Exporter, exportAlbum_test)
{
    TEST_CASE_NAME("exportAlbum_test")
    MainWindow *w = dApp->getMainWindow();
    QStringList paths;
    if (DBManager::instance()->getAllPaths().length() > 0)
        paths << DBManager::instance()->getAllPaths().first();
    else
        paths << testPath_test + "/2e5y8y.jpg";

    AlbumView *a = w->m_pAlbumview;
//    AlbumCreateDialog *ad = new AlbumCreateDialog;
//    QTest::keyClicks(ad->getEdit(), "exportalbum");
//    emit ad->buttonClicked(1, "");
//    QTest::qWait(200);
//    ImageEngineApi::instance()->ImportImagesFromFileList((paths), "exportalbum", a, true);
    const QStringList albumPaths;
    const QString albumnam;

    int (*dlgexec)() = []() {
        return 1;
    };
    typedef int (*fptr)(QDialog *);
    fptr fptrexec = reinterpret_cast<fptr>(&QDialog::exec);  //obtaining an address
    Stub stub;
    stub.set(fptrexec, dlgexec);

    stub_ext::StubExt stu;
//    stu.set_lamda(ADDR(QFileDialog, exec), [](){
//        return QDialog::Accepted;
//    });

    QFileInfo fileinfo(paths.first());
    QDir dir = fileinfo.dir();
    stu.set_lamda(ADDR(QFileDialog, directory), [dir]() {
        return dir;
    });
    stu.set_lamda(ADDR(QDir, absolutePath), [paths]() {
        return paths.first();
    });

    QTestEventList event;
    event.addMousePress(Qt::MouseButton::LeftButton, Qt::NoModifier, QPoint(10, 10));
    event.addMouseRelease(Qt::MouseButton::LeftButton, Qt::NoModifier, QPoint(10, 10));
    event.simulate(a->m_pLeftListView->m_pCustomizeListView->viewport());
    QTest::qWait(500);

    Exporter::instance()->exportAlbum(paths, "TestAlbum");
    QTest::qWait(500);
}

TEST(Exporter, popupDialogSaveImage_test)
{
    TEST_CASE_NAME("popupDialogSaveImage_test")
    MainWindow *w = dApp->getMainWindow();
    QStringList paths;
    if (DBManager::instance()->getAllPaths().length() > 0)
        paths << DBManager::instance()->getAllPaths().first();
    else
        paths << testPath_test + "/2e5y8y.jpg";

    AlbumView *a = w->m_pAlbumview;

    int (*dlgexec)() = []() {
        return 1;
    };
    typedef int (*fptr)(QDialog *);
    fptr fptrexec = reinterpret_cast<fptr>(&QDialog::exec);  //obtaining an address
    Stub stub;
    stub.set(fptrexec, dlgexec);

    stub_ext::StubExt stu;
    QFileInfo fileinfo(paths.first());
    QDir dir = fileinfo.dir();
    stu.set_lamda(ADDR(QFileDialog, directory), [dir]() {
        return dir;
    });
    stu.set_lamda(ADDR(QDir, absolutePath), [paths]() {
        return paths.first();
    });

    Exporter::instance()->popupDialogSaveImage(paths);
    QTest::qWait(500);
}

TEST(Exporter, initValidFormatMap_test)
{
    TEST_CASE_NAME("initValidFormatMap_test")
    MainWindow *w = dApp->getMainWindow();
    w->albumBtnClicked();
    Exporter::instance()->initValidFormatMap();
    QTest::qWait(500);
}
