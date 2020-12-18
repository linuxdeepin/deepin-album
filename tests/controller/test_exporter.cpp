#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <QMap>
#include <DFileDialog>
#include <QTestEventList>
#include <QObject>
#include <QDialog>
#include <QStringList>
#include <DSearchEdit>
#include <QFileDialog>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QMimeDatabase>

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
#include <QCommandLineParser>
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
    qDebug() << "Exporter test_Exporter_init count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    Exporter::instance();
    QUrl UrlInfo1(QString path);
}

TEST(Exporter, exportImage_test)
{
    qDebug() << "Exporter exportImage_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->albumBtnClicked();
    QTest::qWait(500);
    AlbumView *a = w->m_pAlbumview;

    QStringList paths;
    paths << DBManager::instance()->getAllPaths().first();
    Exporter::instance()->exportImage(paths);
    QTest::qWait(500);
}


TEST(Exporter, exportAlbum_test)
{
    qDebug() << "Exporter exportAlbum_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    QStringList paths;
    paths << DBManager::instance()->getAllPaths().first();

    AlbumView *a = w->m_pAlbumview;
//    AlbumCreateDialog *ad = new AlbumCreateDialog;
//    QTest::keyClicks(ad->getEdit(), "exportalbum");
//    emit ad->buttonClicked(1, "");
//    QTest::qWait(200);
//    ImageEngineApi::instance()->ImportImagesFromFileList((paths), "exportalbum", a, true);
    const QStringList albumPaths;
    const QString albumnam;

    int (*dlgexec)() = [](){return 1;};
    typedef int (*fptr)(QDialog*);
    fptr fptrexec = (fptr)(&QDialog::exec);   //obtaining an address
    Stub stub;
    stub.set(fptrexec, dlgexec);

    stub_ext::StubExt stu;
//    stu.set_lamda(ADDR(QFileDialog, exec), [](){
//        return QDialog::Accepted;
//    });

    QFileInfo fileinfo(paths.first());
    QDir dir = fileinfo.dir();
    stu.set_lamda(ADDR(QFileDialog, directory), [dir](){
        return dir;
    });
    stu.set_lamda(ADDR(QDir, absolutePath), [paths](){
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
    qDebug() << "Exporter popupDialogSaveImage_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    QStringList paths;
    paths << DBManager::instance()->getAllPaths().first();

    AlbumView *a = w->m_pAlbumview;

    int (*dlgexec)() = [](){return 1;};
    typedef int (*fptr)(QDialog*);
    fptr fptrexec = (fptr)(&QDialog::exec);   //obtaining an address
    Stub stub;
    stub.set(fptrexec, dlgexec);

    stub_ext::StubExt stu;
    QFileInfo fileinfo(paths.first());
    QDir dir = fileinfo.dir();
    stu.set_lamda(ADDR(QFileDialog, directory), [dir](){
        return dir;
    });
    stu.set_lamda(ADDR(QDir, absolutePath), [paths](){
        return paths.first();
    });

    Exporter::instance()->popupDialogSaveImage(paths);
    QTest::qWait(500);
}

TEST(Exporter, initValidFormatMap_test)
{
    qDebug() << "Exporter initValidFormatMap_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->albumBtnClicked();
    Exporter::instance()->initValidFormatMap();
    QTest::qWait(500);
}
