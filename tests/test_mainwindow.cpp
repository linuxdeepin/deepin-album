#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <QMap>
#include <DFileDialog>
#include <QTestEventList>
#include <QObject>
#include <QDialog>
#include <QStringList>
#include <DSearchEdit>

#define private public
#define protected public

#include "mainwindow.h"
#include "albumcreatedialog.h"
#include "test_qtestDefine.h"
#include "imginfodialog.h"
#include <stub-tool/cpp-stub/stub.h>
#include <stub-tool/stub-ext/stubext.h>

// 3个button界面主视图切换显示
TEST(MainWindow, BtnGroupClick)
{
    qDebug() << "MainWindow BtnGroupClick count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    w->onLoadingFinished();
    w->getButG();
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(1));
    QTest::qWait(500);
    event.simulate(w->getButG()->button(2));
    QTest::qWait(500);
    event.simulate(w->getButG()->button(0));
    event.clear();
    QTest::qWait(500);
}

// 从菜单创建相册
TEST(MainWindow, createalbumFromTitlebarMenu)
{
    qDebug() << "MainWindow createalbumFromTitlebarMenu count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);
    QList<QAction*> actions = w->actions();
    foreach (auto act, actions) {
        if (act->text() == QObject::tr("New album")) {
            act->trigger();
            break;
        }
    }
    QTest::qWait(500);
    QList<QWidget*> widgets = w->findChildren<QWidget*>();
    foreach (auto widget, widgets) {
        if (!strcmp(widget->metaObject()->className() ,"AlbumCreateDialog")) {
            AlbumCreateDialog *tempDlg = dynamic_cast<AlbumCreateDialog*>(widget);
            tempDlg->getEdit()->setText("TestAlbum");
            emit tempDlg->buttonClicked(1,"");
            break;
        }
    }
    w->albumBtnClicked();
    QTest::qWait(500);
}

// 从菜单导入照片
TEST(MainWindow, ImportPhotosFromTitlebarMenu)
{
    qDebug() << "MainWindow ImportPhotosFromTitlebarMenu count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    int (*dlgexec)() = [](){return 1;};
    typedef int (*fptr)(QDialog*);
    fptr fptrexec = (fptr)(&QDialog::exec);   //obtaining an address
    Stub stub;
    stub.set(fptrexec, dlgexec);

    stub_ext::StubExt stu;
    stu.set_lamda(ADDR(DFileDialog, selectedFiles), [](){
        QStringList filelist;
        filelist << ":/2e5y8y.jpg" << ":/2ejqyx.jpg" << ":/2k9o1m.png";
        return filelist;
    });
    QList<QAction*> actions = w->actions();
    foreach (auto act, actions) {
        if (act->text() == QObject::tr("Import photos")) {
            act->trigger();
            break;
        }
    }
    QTest::qWait(500);
}


TEST(MainWindow, setWaitDialogColor_test)
{
    qDebug() << "MainWindow setWaitDialogColor_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    stub_ext::StubExt stu;
    stu.set_lamda(ADDR(DGuiApplicationHelper, themeType), [](){
        return DGuiApplicationHelper::DarkType;
    });

    w->setWaitDialogColor();
    QTest::qWait(500);
    stu.set_lamda(ADDR(DGuiApplicationHelper, themeType), [](){
        return DGuiApplicationHelper::LightType;
    });
    w->setWaitDialogColor();
    QTest::qWait(500);
}

TEST(MainWindow, onShowImageInfo_test)
{
    qDebug() << "MainWindow onShowImageInfo_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    w->onShowImageInfo(":/2e5y8y.jpg");
    QTest::qWait(500);
    w->onShowImageInfo(":/lq6rmy.png");
    QTest::qWait(500);
}

TEST(MainWindow, loadZoomRatio_test)
{
    qDebug() << "MainWindow loadZoomRatio_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    w->loadZoomRatio();
    QTest::qWait(500);
}

TEST(MainWindow, createShorcutJson_test)
{
    qDebug() << "MainWindow createShorcutJson_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    w->createShorcutJson();
    QTest::qWait(500);
}

TEST(MainWindow, thumbnailZoomIn_test)
{
    qDebug() << "MainWindow thumbnailZoomIn_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    w->thumbnailZoomIn();
    QTest::qWait(500);
}

TEST(MainWindow, thumbnailZoomOut_test)
{
    qDebug() << "MainWindow thumbnailZoomOut_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    w->thumbnailZoomOut();
    QTest::qWait(500);
}

TEST(MainWindow, saveWindowState_test)
{
    qDebug() << "MainWindow saveWindowState_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    w->saveWindowState();
    QTest::qWait(500);
}

TEST(MainWindow, compareVersion_test)
{
    qDebug() << "MainWindow compareVersion_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    w->compareVersion();
    QTest::qWait(500);
}

TEST(MainWindow, onSearchEditFinished_test)
{
    qDebug() << "MainWindow onSearchEditFinished_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);
    w->onSearchEditFinished();
    w->timeLineBtnClicked();
    QTest::qWait(500);
    w->onSearchEditFinished();
    w->albumBtnClicked();
    QTest::qWait(500);
    w->onSearchEditFinished();

    QTest::qWait(500);
}

TEST(MainWindow, onNewAPPOpen_test)
{
    qDebug() << "MainWindow onNewAPPOpen_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    stub_ext::StubExt stu;
    stu.set_lamda(ADDR(DSearchEdit, text), [](){
        return "hello";
    });

    w->timeLineBtnClicked();
    w->albumBtnClicked();


    QStringList filelist;
    filelist << "noid" << ":/2e5y8y.jpg" << ":/2ejqyx.jpg" << ":/2k9o1m.png";
    w->onNewAPPOpen(1, filelist);
    QTest::qWait(500);
    w->onSearchEditFinished();
    QTest::qWait(500);
}
