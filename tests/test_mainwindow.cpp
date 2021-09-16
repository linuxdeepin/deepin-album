/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     ZhangYong <zhangyong@uniontech.com>
 *
 * Maintainer: ZhangYong <ZhangYong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include <QMap>
#include <QTimer>
#include <QTestEventList>
#include <QObject>
#include <QDialog>
#include <QStringList>
#include <QScrollBar>

#include <DSearchEdit>
#include <DIconButton>
#include <DMenu>
#include <DFileDialog>

#define private public
#define protected public

#include "mainwindow.h"
#include "albumcreatedialog.h"
#include "test_qtestDefine.h"
#include "imginfodialog.h"
#include "mainwidget.h"
#include "commandline.h"
#include "imageview.h"
//#include "imgdeletedialog.h"
#include "navigationwidget.h"
#include "fileinotify.h"
#include "ac-desktop-define.h"
#include "qgesture.h"

#include <stub-tool/cpp-stub/stub.h>
#include <stub-tool/stub-ext/stubext.h>
#include "testtoolkits.h"


//初始无图界面tab切换
TEST(MainWindow, noPicTab)
{
    TEST_CASE_NAME("noPicTab")
    MainWindow *w = dApp->getMainWindow();
    w->getButG();
    w->allPicBtnClicked();

    QTestEventList e;
    e.addMouseClick(Qt::MouseButton::LeftButton);
    e.simulate(w->getButG()->button(1));
    QTest::qWait(300);
    e.simulate(w->getButG()->button(2));
    QTest::qWait(300);
    e.simulate(w->getButG()->button(0));
    e.clear();
    QTest::qWait(300);

    QKeyEvent EventPress(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
    qApp->sendEvent(w->m_pAllPicBtn, &EventPress);
    QTest::qWait(100);

    QTestEventList tabEvent;
    tabEvent.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 100);
    tabEvent.simulate(w->getButG()->button(0));
    tabEvent.clear();
    QTest::qWait(200);

    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.simulate(w);
    tabEvent.clear();
    QTest::qWait(200);

    // 左键切换
    QKeyEvent EventPressl(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
    qApp->sendEvent(w->m_pAllPicBtn, &EventPressl);
    QTest::qWait(100);

    // 右键切换
    QKeyEvent EventPressr(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
    qApp->sendEvent(w->m_pAllPicBtn, &EventPressr);
    QTest::qWait(100);

    // timeline tab键切换
    w->timeLineBtnClicked();
    QTest::qWait(200);
    qApp->sendEvent(w->m_pTimeBtn, &EventPress);
    QTest::qWait(100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.simulate(w);
    tabEvent.clear();
    QTest::qWait(200);

    // album view tab
    w->albumBtnClicked();
    QTest::qWait(200);
    qApp->sendEvent(w->m_pAlbumBtn, &EventPress);
    QTest::qWait(100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.simulate(w);
    tabEvent.clear();
    QTest::qWait(200);
}

// 3个button界面主视图切换显示,右键菜单
TEST(MainWindow, Picimport)
{
    TEST_CASE_NAME("load")
    QStringList list = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    if (list.size() > 0) {
    } else {
        QStringList listtemp = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
        if (listtemp.size() > 0) {
            list << listtemp.at(0) + "/Pictures";
        }
    }
    qDebug() << "Picture count = " << list.size() << list.at(0);
    MainWindow *w = dApp->getMainWindow();
    w->getButG();
    w->allPicBtnClicked();

    AllPicView *allpicview = w->m_pAllPicView;
    //绑定信号
    ImageEngineApi::instance()->thumbnailLoadThread(80);
    ImageEngineApi::instance()->ImportImagesFromFileList(list, "", allpicview, true);
    allpicview->update();
    QTest::qWait(2000);

    QTestEventList event;
    QPoint p1(30, 100);
    event.addMouseMove(p1);
    event.addKeyClick(Qt::Key_A, Qt::ControlModifier, 50);
    event.addKeyClick(Qt::Key_Delete, Qt::NoModifier, 50);
    event.simulate(allpicview->m_pThumbnailListView);
    event.clear();
    QTest::qWait(300);

    ImageEngineApi::instance()->ImportImagesFromFileList(list, "", allpicview, true);
    QTest::qWait(300);

    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(1));
    QTest::qWait(300);
    event.simulate(w->getButG()->button(2));
    QTest::qWait(300);
    event.simulate(w->getButG()->button(0));
    event.clear();
    QTest::qWait(300);
    ASSERT_TRUE(w != nullptr);

    //测试导入单个图片
    ImageEngineApi::instance()->ImportImagesFromFileList({"~/Pictures/2ejqyx.jpg"}, "", allpicview, true);
    ImageEngineApi::instance()->ImportImagesFromFileList({"~/Pictures/2ejqyx.jpg"}, "111", allpicview, true);
    ImageEngineApi::instance()->ImportImagesFromFileList({"~/Pictures/album_ut_mount_point/DCIM/0jll1w.jpg"}, "", allpicview, true);
}

TEST(MainWindow, allpicture)
{
    TEST_CASE_NAME("allpicture")
    MainWindow *w = dApp->getMainWindow();

    AllPicView *allpicview = w->m_pAllPicView;
    StatusBar *allbar = allpicview->m_pStatusBar;
    QTest::qWait(400);

    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(1));
    QTest::qWait(300);
    event.simulate(w->getButG()->button(2));
    QTest::qWait(300);
    event.simulate(w->getButG()->button(0));
    event.clear();
    QTest::qWait(300);

    allbar->m_pSlider->slider()->setValue(0);
    QTest::qWait(300);
    allbar->m_pSlider->slider()->setValue(1);
    QTest::qWait(300);
    allbar->m_pSlider->slider()->setValue(2);
    QTest::qWait(300);
    allbar->m_pSlider->slider()->setValue(3);
    QTest::qWait(300);
    allbar->m_pSlider->slider()->setValue(5);
    QTest::qWait(300);
    allbar->m_pSlider->slider()->setValue(6);
    QTest::qWait(300);
    allbar->m_pSlider->slider()->setValue(7);
    QTest::qWait(300);
    allbar->m_pSlider->slider()->setValue(8);
    QTest::qWait(300);
    allbar->m_pSlider->slider()->setValue(9);
    QTest::qWait(300);
    allbar->m_pSlider->slider()->setValue(4);
    QTest::qWait(300);

    //尝试解决UT崩溃
    //CommandLine *commandline = w->m_commandLine;
    CommandLine *commandline = CommandLine::instance();
    MainWidget *pmainwidget = nullptr;
    ViewPanel *viewpanel = nullptr;
    TTBContent *ttbc = nullptr;
    ImageView *imageview = nullptr;

    if (commandline)
        pmainwidget = commandline->findChild<MainWidget *>("MainWidget");
    if (pmainwidget)
        viewpanel = pmainwidget->m_viewPanel;

    QTest::qWait(200);
    QPoint p1(60, 100);
    event.addMouseMove(p1);
    event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    event.addMouseDClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    event.simulate(allpicview->m_pThumbnailListView->viewport());
    QTest::qWait(1000);
    event.clear();
    //全屏
    event.addMouseDClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    if (viewpanel) {
        imageview = viewpanel->m_viewB;
        ttbc = viewpanel->m_ttbc;
    }
    if (imageview) {
        event.simulate(imageview->viewport());
        QString path = imageview->path();
        imageview->isWholeImageVisible();
    }
    QTest::qWait(1000);
    event.clear();

    if (ttbc) {
        DIconButton *adapt = ttbc->findChild<DIconButton *>("TtbcontentAdaptImgButton");
        event.addMouseClick(Qt::MouseButton::LeftButton);
        if (adapt)
            event.simulate(adapt);
        QTest::qWait(500);

        DIconButton *adaptscreen = ttbc->findChild<DIconButton *>("TtbcontentAdaptScreenButton");
        if (adaptscreen)
            event.simulate(adaptscreen);
        QTest::qWait(500);

        DIconButton *next = ttbc->findChild<DIconButton *>("TtbcontentNextButton");
        if (next)
            event.simulate(next);
        QTest::qWait(500);

        DIconButton *pre = ttbc->findChild<DIconButton *>("TtbcontentPreButton");
        if (pre)
            event.simulate(pre);
        QTest::qWait(500);

        DIconButton *collect = ttbc->findChild<DIconButton *>("TtbcontentCollectButton");
        if (collect)
            event.simulate(collect);
        QTest::qWait(500);

        DIconButton *rotateR = ttbc->findChild<DIconButton *>("TtbcontentRotateRightButton");
        if (rotateR)
            event.simulate(rotateR);
        QTest::qWait(1500);

        DIconButton *rotateL = ttbc->findChild<DIconButton *>("TtbcontentRotateLeftButton");
        if (rotateL)
            event.simulate(rotateL);
        QTest::qWait(1500);

        DIconButton *rotatedel = ttbc->findChild<DIconButton *>("TtbcontentTrashButton");
        if (rotatedel)
            event.simulate(rotatedel);
        QTest::qWait(1500);

        DIconButton *back = ttbc->findChild<DIconButton *>("TtbcontentBackButton");
        if (back)
            event.simulate(back);
        QTest::qWait(500);
        event.clear();

        event.addMouseMove(p1);
        event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
        event.simulate(allpicview->m_pThumbnailListView->viewport());
        event.clear();
        QTest::qWait(500);
    }
    // allpicview tab键切换
    w->allPicBtnClicked();
    QTest::qWait(500);
    QKeyEvent EventPress(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
    qApp->sendEvent(w->m_pAllPicBtn, &EventPress);
    QTest::qWait(100);
    QTestEventList tabEvent;
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.simulate(w);
    tabEvent.clear();
    QTest::qWait(500);

    // 左键切换
    QKeyEvent EventPressl(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
    qApp->sendEvent(w->m_pAllPicBtn, &EventPressl);
    QTest::qWait(100);

    // 右键切换
    QKeyEvent EventPressr(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
    qApp->sendEvent(w->m_pAllPicBtn, &EventPressr);
    QTest::qWait(100);

    // timeline tab键切换
    w->timeLineBtnClicked();
    QTest::qWait(200);
    qApp->sendEvent(w->m_pTimeBtn, &EventPress);
    QTest::qWait(100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.simulate(w);
    tabEvent.clear();
    QTest::qWait(200);

    // album view tab
    w->albumBtnClicked();
    QTest::qWait(200);
    qApp->sendEvent(w->m_pAlbumBtn, &EventPress);
    QTest::qWait(100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    tabEvent.simulate(w);
    tabEvent.clear();
    QTest::qWait(200);

    w->allPicBtnClicked();
    QTest::qWait(200);
    event.addMouseMove(p1);
    event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    event.simulate(allpicview->m_pThumbnailListView->viewport());
    event.clear();
    QTest::qWait(500);

    //------右键菜单start---------
    QTestEventList e;
    auto menu = runContextMenu(allpicview->m_pThumbnailListView->viewport(), p1);
    using TR_SUBORDINATE_t = PointerTypeGetter < decltype(allpicview->m_pThumbnailListView) >::type;

    //查看
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("View"));
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    QTest::qWait(300);

    //全屏
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Fullscreen"));
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    QTest::qWait(1000);

    //TODO:打印

    //幻灯片
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Slide show"));
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(w->m_slidePanel);
    e.clear();
    QTest::qWait(1000);

    SlideShowPanel *slideshowpanel = w->m_slidePanel;

    //fix：没有调用startSlideShow而直接用鼠标去点，导致UT崩溃

    SignalManager::ViewInfo info;
    info.album = "";
    info.lastPanel = nullptr;
    info.paths = allpicview->m_pThumbnailListView->getFileList();
    if (info.paths.size() > 0) {
        info.path = info.paths.at(0);
    }
    info.fullScreen = true;
    info.slideShow = true;
    info.viewType = utils::common::VIEW_ALLPIC_SRN;
    info.viewMainWindowID = VIEW_MAINWINDOW_ALLPIC;
    slideshowpanel->setIsRandom(false);
    slideshowpanel->startSlideShow(info, true);
    emit dApp->signalM->showSlidePanel(VIEW_MAINWINDOW_ALLPIC);

    if (slideshowpanel) {
        SlideShowBottomBar *sliderbar = slideshowpanel->slideshowbottombar;
        if (sliderbar) {
            e.addMouseClick(Qt::MouseButton::LeftButton);
            DIconButton *preButton = sliderbar->findChild<DIconButton *>(Slider_Pre_Button);
            DIconButton *NextButton = sliderbar->findChild<DIconButton *>(Slider_Next_Button);
            DIconButton *Play_PauseButton = sliderbar->findChild<DIconButton *>(Slider_Play_Pause_Button);
            DIconButton *ExitButton = sliderbar->findChild<DIconButton *>(Slider_Next_Button);
            if (NextButton) {
                e.simulate(NextButton);
                QTest::qWait(500);
            }
            if (NextButton) {
                e.simulate(NextButton);
                QTest::qWait(2000);
            }
            if (NextButton) {
                e.simulate(NextButton);
                QTest::qWait(2000);
            }
            if (NextButton) {
                e.simulate(NextButton);
                QTest::qWait(2000);
            }
            if (NextButton) {
                e.simulate(NextButton);
                QTest::qWait(2000);
            }
            if (NextButton) {
                e.simulate(NextButton);
                QTest::qWait(2000);
            }
            if (preButton) {
                e.simulate(preButton);
                QTest::qWait(500);
            }
            if (Play_PauseButton) {
                e.simulate(Play_PauseButton);
                QTest::qWait(500);
                e.simulate(Play_PauseButton);
                QTest::qWait(500);
            }
            if (ExitButton) {
                e.simulate(ExitButton);
                QTest::qWait(300);
            }
            e.clear();

            if (slideshowpanel->isVisible()) {
                e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 10);
                e.simulate(slideshowpanel);
                e.clear();
            }
        } else { //无法获得切换控件，退出幻灯片播放
            QTest::qWait(1000);
            e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 10);
            e.simulate(slideshowpanel);
            e.clear();
        }
    }
    //TODO:添加到自定义相册5
    //导出6
    QTimer::singleShot(1000, w, [ = ]() {
        //导出重复时，干掉覆盖提示框
        int (*dlgexec)() = []() {
            return 1;
        };
        typedef int (*fptr)(QDialog *);
        fptr fptrexec = reinterpret_cast<fptr>(&QDialog::exec);  //obtaining an address
        Stub stub;
        stub.set(fptrexec, dlgexec);
        QTest::qWait(300);

        QTestEventList e;
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
        e.simulate(Exporter::instance()->m_exportImageDialog->getButton(1));
        e.clear();
    });

    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Export"));
    QTest::qWait(1000);

    //复制7
    {
        //https://gerrit.uniontech.com/c/deepin-album/+/50941 ARM64频繁失败原因：没有干掉&QDialog::exec，导致UT阻塞
        int (*dlgexec)() = []() {
            return 1;
        };
        typedef int (*fptr)(QDialog *);
        fptr fptrexec = reinterpret_cast<fptr>(&QDialog::exec);  //obtaining an address
        Stub stub;
        stub.set(fptrexec, dlgexec);

        runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Copy"));
    }

    //删除后重新选中最新的第一张
    e.addMouseMove(p1);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(allpicview->m_pThumbnailListView->viewport());
    e.clear();
    QTest::qWait(200);

    //收藏9
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Favorite"));

    //顺时针10
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Rotate clockwise"));

    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(allpicview->m_pThumbnailListView->viewport());
    e.clear();
    QTest::qWait(200);

    //逆时针11
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Rotate counterclockwise"));

    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(allpicview->m_pThumbnailListView->viewport());
    e.clear();
    QTest::qWait(200);

    //设为壁纸12
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Set as wallpaper"));

    //文管显示13
    e.addMouseClick(Qt::MouseButton::LeftButton);
    e.simulate(allpicview->m_pThumbnailListView->viewport());
    e.clear();
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Display in file manager"));
    w->raise();
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(allpicview->m_pThumbnailListView->viewport());
    e.clear();
    QTest::qWait(200);

    //照片信息14
    e.addMouseClick(Qt::MouseButton::LeftButton);
    e.simulate(allpicview->m_pThumbnailListView->viewport());
    e.clear();
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Photo info"));

    QList<QWidget *> lis =  dApp->getDAppNew()->topLevelWidgets();
    auto iter = lis.cbegin();
    while (iter != lis.cend()) {
        if ((*iter)->objectName() == "ImgInfoDialog") {
            (*iter)->hide();
            break;
        }
        ++iter;
    }
    //------右键菜单end---------

    //多选
    QPoint p2(280, 100);

    //这坨代码是在激活多选操作
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.addMouseMove(p2);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::ControlModifier, p2, 50);
    e.simulate(allpicview->m_pThumbnailListView->viewport());
    e.clear();
    QTest::qWait(300);

    auto menu2 = runContextMenu(allpicview->m_pThumbnailListView->viewport(), p2);

    //幻灯片
    runActionFromMenu(menu2, TR_SUBORDINATE_t::tr("Slide show"));

    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    //导出-d
//    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent2);
//    QTest::qWait(300);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
//    e.simulate(menuWidget2);
//    e.clear();
//    QTest::qWait(500);
    //复制
    runActionFromMenu(menu2, TR_SUBORDINATE_t::tr("Copy"));

    w->resize(980, 600);
    ASSERT_TRUE(w != nullptr);
}

TEST(MainWindow, videoInfo)
{
    TEST_CASE_NAME("videoInfo")
    MainWindow *w = dApp->getMainWindow();

    AllPicView *allpicview = w->m_pAllPicView;

    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(0));
    event.clear();
    QTest::qWait(300);

    QModelIndex index;
    for (int i = 0; i < allpicview->getThumbnailListView()->m_model->rowCount(); i++) {
        index = allpicview->getThumbnailListView()->m_model->index(i, 0);
        DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
        if (data.itemType == ItemType::ItemTypeVideo) {
            break;
        }
    }
    QRect videoItem = allpicview->getThumbnailListView()->visualRect(index);

    QTest::qWait(200);
    QPoint p1(videoItem.x() + 10, videoItem.y() + 10);
    event.addMouseMove(p1);
    event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    event.simulate(allpicview->m_pThumbnailListView->viewport());
    QTest::qWait(1000);
    event.clear();

    //------右键菜单start---------
    QTestEventList e;
    auto menu = runContextMenu(allpicview->m_pThumbnailListView->viewport(), p1);
    using TR_SUBORDINATE_t = PointerTypeGetter < decltype(allpicview->m_pThumbnailListView) >::type;

    //照片信息14
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Video info"));

    QList<QWidget *> lis =  dApp->getDAppNew()->topLevelWidgets();
    auto iter = lis.cbegin();
    while (iter != lis.cend()) {
        if ((*iter)->objectName() == "VideoInfoDialog") {
            (*iter)->hide();
            break;
        }
        ++iter;
    }
    //------右键菜单end---------
}

TEST(MainWindow, viewpanelmenu)
{
    TEST_CASE_NAME("viewpanelmenu")
    MainWindow *w = dApp->getMainWindow();
    MainWidget *wid = w->m_commandLine->findChild<MainWidget *>("MainWidget");
    QTestEventList e;
    QPoint p1(30, 100);
    e.addMouseMove(p1);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.addMouseDClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(w->m_pAllPicView->m_pThumbnailListView->viewport());
    QTest::qWait(500);
    e.clear();

    auto menu = runContextMenu(wid->m_viewPanel->m_viewB->viewport(), p1);
    using TR_SUBORDINATE_t = PointerTypeGetter < decltype(wid->m_viewPanel) >::type;

    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Fullscreen"));
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(wid->m_viewPanel->m_viewB->viewport());
    e.clear();
    QTest::qWait(300);

    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Slide show"));
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(wid->m_viewPanel->m_viewB->viewport());
    e.clear();
    QTest::qWait(300);

    QTimer::singleShot(1000, w, [ = ]() {
        //导出重复时，干掉覆盖提示框
        int (*dlgexec)() = []() {
            return 1;
        };
        typedef int (*fptr)(QDialog *);
        fptr fptrexec = reinterpret_cast<fptr>(&QDialog::exec);  //obtaining an address
        Stub stub;
        stub.set(fptrexec, dlgexec);
        QTest::qWait(300);

        QTestEventList e;
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
        e.simulate(Exporter::instance()->m_exportImageDialog->getButton(1));
        e.clear();
    });

    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Export"));

    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Copy"));

//    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Unfavorite"));

    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Rotate clockwise"));
    QTest::qWait(1500);

    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Rotate counterclockwise"));
    QTest::qWait(1500);

    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Set as wallpaper"));

    //模拟拖拽
//    QPoint p = wid->m_viewPanel->m_ttbc->m_imgListView->m_selectItem->pos();
    //先放大
    e.addKeyClick(Qt::Key_Plus, Qt::ControlModifier, 100);
    e.addKeyClick(Qt::Key_Plus, Qt::ControlModifier, 100);
    e.addKeyClick(Qt::Key_Plus, Qt::ControlModifier, 100);
    e.simulate(wid->m_viewPanel->m_viewB->viewport());
    QTest::qWait(300);
    e.clear();
    e.addKeyClick(Qt::Key_Down, Qt::NoModifier, 50);
    e.simulate(wid->m_viewPanel->m_viewB->viewport());
    QTest::qWait(300);

    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Display in file manager"));

    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Photo info"));

    //从这个case中没有找到可以初始化m_ttbc的代码
    if (wid->m_viewPanel->m_ttbc == nullptr) {
        wid->m_viewPanel->bottomTopLeftContent();
    }

    emit wid->m_viewPanel->m_ttbc->m_backButton->click();
    w->raise();
    QTest::qWait(300);
}

TEST(MainWindow, timelineview)
{
    TEST_CASE_NAME("timelineview")
    MainWindow *w = dApp->getMainWindow();
    TimeLineView *timelineview = w->m_pTimeLineView;
    w->timeLineBtnClicked();

    QTestEventList e;
    e.addMouseClick(Qt::MouseButton::LeftButton);
    e.simulate(w->getButG()->button(1));
    e.clear();

    //ARM64下UT总是报告这里的w->m_commandLine是nullptr，因此置换为从单例获取
    //CommandLine *commandline = w->m_commandLine;
    CommandLine *commandline = CommandLine::instance();
    ImageView *imageview = commandline->findChild<MainWidget *>("MainWidget")->m_viewPanel->m_viewB;

    if (timelineview) {
        // ----右键菜单start----
        QPoint pr(60, 140);

        //选中第一张
        e.addMouseMove(pr, 20);
        e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pr, 50);
        e.addMouseDClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pr, 50);
        e.simulate(timelineview->getThumbnailListView()->viewport());
        e.clear();
        QTest::qWait(1000);

        e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
        e.simulate(imageview->viewport());
        e.clear();
        QTest::qWait(300);

        e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pr, 50);
        e.simulate(timelineview->getThumbnailListView()->viewport());
        e.clear();
        QTest::qWait(300);

        auto menu = runContextMenu(timelineview->getThumbnailListView()->viewport(), pr);
        using TR_SUBORDINATE_t = PointerTypeGetter < decltype(timelineview->getThumbnailListView()) >::type;

        //查看
        runActionFromMenu(menu, TR_SUBORDINATE_t::tr("View"));
        e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
        e.simulate(imageview->viewport());
        e.clear();
        QTest::qWait(300);
        //全屏
        e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pr, 50);
        e.simulate(timelineview->getThumbnailListView()->viewport());
        e.clear();
        runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Fullscreen"));
        e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
        e.simulate(imageview->viewport());
        e.clear();
        QTest::qWait(300);
        //TODO:打印

        //幻灯片
        e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pr, 50);
        e.simulate(timelineview->getThumbnailListView()->viewport());
        e.clear();
        runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Slide show"));
        e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
        e.simulate(imageview->viewport());
        e.clear();
        QTest::qWait(300);
        //复制7
        e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pr, 50);
        e.simulate(timelineview->getThumbnailListView()->viewport());
        e.clear();
        runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Copy"));
        //TODO:删除
        //收藏9
        e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pr, 50);
        e.simulate(timelineview->getThumbnailListView()->viewport());
        e.clear();
        runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Favorite"));
        //顺时针10
        e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pr, 50);
        e.simulate(timelineview->getThumbnailListView()->viewport());
        e.clear();
        runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Rotate clockwise"));
        QTest::qWait(1500);
        e.addMouseMove(pr, 20);
        e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pr, 50);
        e.simulate(timelineview->getThumbnailListView()->viewport());
        e.clear();
        QTest::qWait(300);
        //逆时针11
        e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pr, 50);
        e.simulate(timelineview->getThumbnailListView()->viewport());
        e.clear();
        runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Rotate counterclockwise"));
        QTest::qWait(1500);

        e.addMouseMove(pr, 20);
        e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pr, 50);
        e.simulate(timelineview->getThumbnailListView()->viewport());
        e.clear();
        QTest::qWait(300);
        //设为壁纸12
        runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Set as wallpaper"));
        QTest::qWait(1500);
        //文管显示13
        e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pr, 50);
        e.simulate(timelineview->getThumbnailListView()->viewport());
        e.clear();
        runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Display in file manager"));
        w->raise();
        QTest::qWait(300);

        e.addMouseMove(pr, 20);
        e.addMouseClick(Qt::MouseButton::LeftButton);
        e.simulate(timelineview->getThumbnailListView()->viewport());
        e.clear();
        QTest::qWait(300);
        //照片信息14
        runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Photo info"));

        //幻灯片
        e.addMouseMove(pr, 20);
        e.addMouseClick(Qt::MouseButton::LeftButton);
        e.simulate(timelineview->getThumbnailListView()->viewport());
        e.clear();

        runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Slide show"));

        commandline->findChild<MainWidget *>("MainWidget")->m_extensionPanel->setContent(nullptr);
        commandline->findChild<MainWidget *>("MainWidget")->m_extensionPanel->updateRectWithContent();
        e.addMouseMove(commandline->findChild<MainWidget *>("MainWidget")->m_extensionPanel->pos());
        e.simulate(commandline->findChild<MainWidget *>("MainWidget")->m_extensionPanel);
        commandline->findChild<MainWidget *>("MainWidget")->m_extensionPanel->update();
        e.clear();
        QTest::qWait(300);
        commandline->findChild<MainWidget *>("MainWidget")->onBackToMainPanel();
        commandline->findChild<MainWidget *>("MainWidget")->onActiveWindow();
        commandline->findChild<MainWidget *>("MainWidget")->onShowInFileManager("");
        commandline->findChild<MainWidget *>("MainWidget")->onMouseMove(false);
        commandline->findChild<MainWidget *>("MainWidget")->onShowFullScreen();

        e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
        e.simulate(imageview->viewport());
        e.clear();

        //复制
//        qApp->sendEvent(timelineview->getThumbnailListView()->viewport(), &menuEvent);
//        QTest::qWait(300);
//        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
//        e.simulate(menuWidget3);
//        e.clear();
//        QTest::qWait(500);
    }
    StatusBar *bar1 = timelineview->m_pStatusBar;
    bar1->m_pSlider->slider()->setValue(0);
    QTest::qWait(300);
    bar1->m_pSlider->slider()->setValue(1);
    QTest::qWait(300);
    bar1->m_pSlider->slider()->setValue(2);
    QTest::qWait(300);
    bar1->m_pSlider->slider()->setValue(3);
    QTest::qWait(300);
    bar1->m_pSlider->slider()->setValue(5);
    QTest::qWait(300);
    bar1->m_pSlider->slider()->setValue(6);
    QTest::qWait(300);
    bar1->m_pSlider->slider()->setValue(7);
    QTest::qWait(300);
    bar1->m_pSlider->slider()->setValue(8);
    QTest::qWait(300);
    bar1->m_pSlider->slider()->setValue(9);
    QTest::qWait(300);
    bar1->m_pSlider->slider()->setValue(4);
    QTest::qWait(300);
    ASSERT_TRUE(timelineview != nullptr);
}

TEST(MainWindow, AlbumView)
{
    TEST_CASE_NAME("AlbumView")
    MainWindow *w = dApp->getMainWindow();
    AlbumView *albumview = w->m_pAlbumview;

    QTestEventList e;
    e.addMouseClick(Qt::MouseButton::LeftButton);
    w->albumBtnClicked();
    e.simulate(w->getButG()->button(2));
    e.clear();

    StatusBar *bar = albumview->m_pStatusBar;
    bar->m_pSlider->slider()->setValue(0);
    QTest::qWait(300);
    bar->m_pSlider->slider()->setValue(1);
    QTest::qWait(300);
    bar->m_pSlider->slider()->setValue(2);
    QTest::qWait(300);
    bar->m_pSlider->slider()->setValue(3);
    QTest::qWait(300);
    bar->m_pSlider->slider()->setValue(5);
    QTest::qWait(300);
    bar->m_pSlider->slider()->setValue(6);
    QTest::qWait(300);
    bar->m_pSlider->slider()->setValue(7);
    QTest::qWait(300);
    bar->m_pSlider->slider()->setValue(8);
    QTest::qWait(300);
    bar->m_pSlider->slider()->setValue(9);
    QTest::qWait(300);
    bar->m_pSlider->slider()->setValue(4);
    QTest::qWait(300);

    CommandLine *commandline = w->m_commandLine;
    ImageView *imageview = commandline->findChild<MainWidget *>("MainWidget")->m_viewPanel->m_viewB;
    ThumbnailListView *firstThumb = albumview->m_pImpTimeLineView->getListView();
    if (!firstThumb) {
        return;
    }

    QPoint p1 = firstThumb->viewport()->pos() + QPoint(45, 110); //已导入
    e.addMouseMove(p1);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.addMouseDClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(firstThumb->viewport());
    e.clear();
    QTest::qWait(1000);
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    QTest::qWait(300);

    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(firstThumb->viewport());
    e.clear();
    QTest::qWait(300);

    //todo
//    albumview->m_pImpTimeLineView->getCurrentSelectPics();

    //------右键菜单start---------
    auto menu = runContextMenu(firstThumb->viewport(), p1);
    using TR_SUBORDINATE_t = PointerTypeGetter < decltype(firstThumb) >::type;

    //查看
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("View"));
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    QTest::qWait(300);

    //全屏
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Fullscreen"));
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    QTest::qWait(300);
    //TODO:打印

    //幻灯片
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Slide show"));
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    QTest::qWait(300);

    //复制7
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Copy"));

    //收藏9
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Favorite"));

    //顺时针10
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Rotate clockwise"));
    QTest::qWait(1500);

    //文件有变动，需要重新获取指针
    ThumbnailListView *t = w->m_pAlbumview->m_pImpTimeLineView->getListView();
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(t->viewport());
    e.clear();
    QTest::qWait(200);
    auto menu2 = runContextMenu(t->viewport(), p1);

    //逆时针11
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Rotate counterclockwise"));
    QTest::qWait(1500);

    //文件有变动，需要重新获取指针
    ThumbnailListView *t1 = w->m_pAlbumview->m_pImpTimeLineView->getListView();
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(t1->viewport());
    e.clear();
    QTest::qWait(200);
    auto menu3 = runContextMenu(t1->viewport(), p1);

    //设为壁纸12
    runActionFromMenu(menu3, TR_SUBORDINATE_t::tr("Set as wallpaper"));

    //文管显示13
    runActionFromMenu(menu3, TR_SUBORDINATE_t::tr("Display in file manager"));
    w->raise();
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(t1->viewport());
    e.clear();
    QTest::qWait(200);

    //照片信息14
    runActionFromMenu(menu3, TR_SUBORDINATE_t::tr("Photo info"));

    QList<QWidget *> lis =  dApp->getDAppNew()->topLevelWidgets();
    auto iter = lis.cbegin();
    while (iter != lis.cend()) {
        if ((*iter)->objectName() == "ImgInfoDialog") {
            (*iter)->hide();
            break;
        }
        ++iter;
    }
    //------右键菜单end---------

    //多选
    QPoint p2 = p1 + QPoint(200, 0);
    QPoint p3 = p2 + QPoint(200, 0);

    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.addMouseMove(p2);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::ControlModifier, p2, 50);
    e.addMouseMove(p3);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::ShiftModifier, p3, 50);
    e.addKeyClick(Qt::Key_A, Qt::ControlModifier, 50);
    e.simulate(t1->viewport());
    e.clear();
    QTest::qWait(300);

    auto menu4 = runContextMenu(t1->viewport(), p2);

    //幻灯片
    runActionFromMenu(menu4, TR_SUBORDINATE_t::tr("Slide show"));
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    //导出-d

    //复制
    runActionFromMenu(menu4, TR_SUBORDINATE_t::tr("Copy"));

    //重新选中，拖拽
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(t1->viewport());
    e.clear();
    QTest::qWait(300);

    e.addMousePress(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.addMouseMove(QPoint(0, 0), 50);
    e.addMouseRelease(Qt::MouseButton::LeftButton, Qt::NoModifier, QPoint(0, 0), 50);
    e.simulate(t1->viewport());
    e.clear();
    QTest::qWait(300);

    w->m_pAlbumview->m_pImpTimeLineView->clearAllSelection();

    ASSERT_TRUE(albumview != nullptr);
}

TEST(MainWindow, recentlydelete)
{
    TEST_CASE_NAME("recentlydelete")
    MainWindow *w = dApp->getMainWindow();
    AlbumView *albumview = w->m_pAlbumview;

    QTestEventList e;

    ThumbnailListView *thumb = albumview->m_pRightTrashThumbnailList;

    clickToDeletePage();

    QPoint p1 = thumb->viewport()->pos() + QPoint(80, 135); //最近删除
    e.addMouseMove(p1);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(thumb->viewport());
    e.clear();
    QTest::qWait(300);

    auto menu = runContextMenu(thumb->viewport(), p1);
    using TR_SUBORDINATE_t = PointerTypeGetter < decltype(thumb) >::type;

//    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Photo info"));

    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Restore"));

//    ASSERT_TRUE(w->m_pAlbumview->m_pRightTrashThumbnailList != nullptr);
}

TEST(MainWindow, favorite)
{
    TEST_CASE_NAME("favorite")
    MainWindow *w = dApp->getMainWindow();
    AlbumView *albumview = w->m_pAlbumview;
    ImageView *imageview = w->m_commandLine->findChild<MainWidget *>("MainWidget")->m_viewPanel->m_viewB;
    QTestEventList e;
    ThumbnailListView *thumb = albumview->m_favoriteThumbnailList;

    clickToFavoritePage();

    //------右键菜单start---------
    QPoint p1 = thumb->viewport()->pos() + QPoint(80, 135); //收藏
    e.addMouseMove(p1);
    e.addMouseDClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(thumb->viewport());
    e.clear();
    QTest::qWait(1000);
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    QTest::qWait(300);

    auto menu = runContextMenu(thumb->viewport(), p1);
    using TR_SUBORDINATE_t = PointerTypeGetter < decltype(thumb) >::type;

    //查看
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("View"));
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    QTest::qWait(300);

    //全屏
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Fullscreen"));
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    QTest::qWait(300);
    //TODO:打印

    //幻灯片
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Slide show"));
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    QTest::qWait(300);

    //复制7
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Copy"));

    //顺时针10
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Rotate clockwise"));
    QTest::qWait(1500);

    //文件有变动，需要重新获取指针
    ThumbnailListView *t = w->m_pAlbumview->m_favoriteThumbnailList;
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(t->viewport());
    e.clear();
    QTest::qWait(200);

    auto menu2 = runContextMenu(t->viewport(), p1);

    //逆时针11
    runActionFromMenu(menu2, TR_SUBORDINATE_t::tr("Rotate counterclockwise"));
    QTest::qWait(1500);

    //文件有变动，需要重新获取指针
    ThumbnailListView *t1 = w->m_pAlbumview->m_favoriteThumbnailList;
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(t1->viewport());
    e.clear();
    QTest::qWait(200);

    auto menu3 = runContextMenu(t1->viewport(), p1);

    //设为壁纸12
    runActionFromMenu(menu3, TR_SUBORDINATE_t::tr("Set as wallpaper"));

    //文管显示13
    runActionFromMenu(menu3, TR_SUBORDINATE_t::tr("Display in file manager"));
    w->raise();
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(t1->viewport());
    e.clear();
    QTest::qWait(200);

    //照片信息14
    runActionFromMenu(menu3, TR_SUBORDINATE_t::tr("Photo info"));

    //收藏9
    runActionFromMenu(menu3, TR_SUBORDINATE_t::tr("Favorite"));

    ASSERT_TRUE(w->m_pAlbumview != nullptr);
}

TEST(MainWindow, search)
{
    //3界面搜索界面
    TEST_CASE_NAME("search")
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTestEventList e;
    e.addMouseClick(Qt::MouseButton::LeftButton);
    e.simulate(w->getButG()->button(0));
    QTest::qWait(300);
    e.clear();
    //所有界面搜索
    w->m_pSearchEdit->setText("a");
    w->m_pSearchEdit->editingFinished();
    QTest::qWait(300);
    w->m_pSearchView->onSlideShowBtnClicked();
    QTest::qWait(1000);
    e.addKeyClick(Qt::Key_Escape);
    e.simulate(w->m_commandLine->findChild<MainWidget *>("MainWidget")->m_viewPanel->m_viewB->viewport());
//    e.clear();
    QTest::qWait(300);

    w->m_pSearchEdit->clear();
    w->m_pSearchEdit->setText("testNoreault");//无搜索结果
    w->m_pSearchEdit->editingFinished();
    QTest::qWait(300);

    //时间线搜索
    w->timeLineBtnClicked();
    w->m_pSearchEdit->setText("a");
    w->m_pSearchEdit->editingFinished();
    QTest::qWait(300);
    w->m_pSearchView->onSlideShowBtnClicked();
    QTest::qWait(1000);
    e.simulate(w->m_commandLine->findChild<MainWidget *>("MainWidget")->m_viewPanel->m_viewB->viewport());
    QTest::qWait(300);

    w->m_pSearchEdit->clear();
    w->m_pSearchEdit->setText("testNoreault");//无搜索结果
    w->m_pSearchEdit->editingFinished();
    QTest::qWait(300);

    //相册界面搜索
    w->albumBtnClicked();
    w->m_pSearchEdit->setText("a");
    w->m_pSearchEdit->editingFinished();
    QTest::qWait(300);
    w->m_pSearchView->onSlideShowBtnClicked();
    QTest::qWait(1000);
    e.simulate(w->m_commandLine->findChild<MainWidget *>("MainWidget")->m_viewPanel->m_viewB->viewport());
    QTest::qWait(300);

    w->m_pSearchEdit->clear();
    w->m_pSearchEdit->setText("testNoreault");//无搜索结果
    w->m_pSearchEdit->editingFinished();
    QTest::qWait(300);
    ASSERT_TRUE(w->m_pSearchEdit != nullptr);
}

// 从菜单创建相册
TEST(MainWindow, createalbumFromTitlebarMenu)
{
#ifdef SYSTEM_MIPS
    return;
#endif
    TEST_CASE_NAME("createalbumFromTitlebarMenu")
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(2000);
    QAction *act = w->findChild<QAction *> ("New album");
    QTimer::singleShot(1000, w, [ = ]() {
        QList<QWidget *> widgets = w->findChildren<QWidget *>();
        foreach (auto widget, widgets) {
            if (!strcmp(widget->metaObject()->className(), "AlbumCreateDialog")) {
                AlbumCreateDialog *tempDlg = dynamic_cast<AlbumCreateDialog *>(widget);
                tempDlg->getEdit()->setText("albumFromAction");
                emit tempDlg->buttonClicked(1, "");
                break;
            }
        }
    });
    if (act) {
        emit w->m_pTitleBarMenu->triggered(act);
    }

    //往此相册导入图片
    int (*dlgexec)() = []() {
        return 1;
    };
    typedef int (*fptr)(QDialog *);
    fptr fptrexec = reinterpret_cast<fptr>(&QDialog::exec);  //obtaining an address
    Stub stub;
    stub.set(fptrexec, dlgexec);

    stub_ext::StubExt stu;
    stu.set_lamda(ADDR(DFileDialog, selectedFiles), []() {
        QStringList filelist;
        filelist << testPath_Pictures + "/a.jpg" << testPath_Pictures + "/aa.jpg" ;
        return filelist;
    });
    emit w->m_pAlbumview->m_pImportView->m_pImportBtn->clicked(true);

    QTest::qWait(500);
}

// 从菜单导入照片
TEST(MainWindow, ImportPhotosFromTitlebarMenu)
{
    TEST_CASE_NAME("ImportPhotosFromTitlebarMenu")
    MainWindow *w = dApp->getMainWindow();
    w->albumBtnClicked();
    QTest::qWait(500);
    QTestEventList e;
    e.addMouseClick(Qt::MouseButton::LeftButton);
    e.simulate(w->getButG()->button(2));
    QTest::qWait(2000);
    e.clear();

    int (*dlgexec)() = []() {
        return 1;
    };
    typedef int (*fptr)(QDialog *);
    fptr fptrexec = reinterpret_cast<fptr>(&QDialog::exec);  //obtaining an address
    Stub stub;
    stub.set(fptrexec, dlgexec);

    stub_ext::StubExt stu;
    stu.set_lamda(ADDR(DFileDialog, selectedFiles), []() {
        QStringList filelist;
        filelist << testPath_Pictures + "/a.jpg" << testPath_Pictures + "/3333.jpg" ;
        return filelist;
    });
    QAction *act = w->findChild<QAction *> (Import_Image_View);
    if (act) {
        emit w->m_pTitleBarMenu->triggered(act);
    }
    QTest::qWait(200);
    //自定义相册右键点击
    QPoint p = w->m_pAlbumview->m_pLeftListView->m_pCustomizeLabel->pos();
    p += QPoint(2, 50);
    e.addMouseMove(p);
    e.simulate(w->m_pAlbumview->m_pLeftListView->m_pCustomizeLabel);
    e.clear();

    e.addMouseClick(Qt::MouseButton::RightButton, Qt::NoModifier, p, 10);
    QMap<QString, QAction *> act1 = w->m_pAlbumview->m_pLeftListView->m_MenuActionMap;
    QList<QAction *> allact;
    auto item = act1.begin();
    for (; item != act1.end(); item ++) {
        allact.append(item.value());
    }
    //导出
    emit allact.at(1)->trigger();
    QTest::qWait(2000);
    //幻灯片
    emit allact.at(2)->trigger();
    QTest::qWait(2000);
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 10);
    e.simulate(w->m_slidePanel);
    e.clear();
    QTest::qWait(500);
    //重命名
    emit allact.at(4)->trigger();

    e.addMouseClick(Qt::MouseButton::LeftButton);
    e.simulate(w->getButG()->button(0));
    w->allPicBtnClicked();
    QTest::qWait(500);
    e.simulate(w->getButG()->button(2));
    w->albumBtnClicked();
    e.clear();
    QTest::qWait(500);

    //新建
    QTimer::singleShot(1000, w, [ = ]() {
        QList<QWidget *> widgets = w->findChildren<QWidget *>();
        foreach (auto widget, widgets) {
            if (!strcmp(widget->metaObject()->className(), "AlbumCreateDialog")) {
                AlbumCreateDialog *tempDlg = dynamic_cast<AlbumCreateDialog *>(widget);
                tempDlg->getEdit()->setText("albumFromAction2");
                emit tempDlg->buttonClicked(1, "");
                break;
            }
        }
    });
    emit allact.at(3)->trigger();
    QTest::qWait(1000);

    //删除
    QTimer::singleShot(1000, w, [ = ]() {
        emit w->m_pAlbumview->m_pLeftListView->deletDialg->buttonClicked(1, "");
    });
    emit allact.at(0)->trigger();

    QTest::qWait(200);
}

TEST(MainWindow, setWaitDialogColor_test)
{
    TEST_CASE_NAME("setWaitDialogColor_test")
#ifdef SYSTEM_MIPS
    return;
#endif
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    //根据研发内部技术分享，打桩需要修改为对同一函数打桩只能打一次，再次打桩前要先把之前的桩销毁
    {
        stub_ext::StubExt stu;
        stu.set_lamda(ADDR(DGuiApplicationHelper, themeType), []() {
            return DGuiApplicationHelper::DarkType;
        });

        w->setWaitDialogColor();
        QTest::qWait(500);
    }

    {
        stub_ext::StubExt stu;
        stu.set_lamda(ADDR(DGuiApplicationHelper, themeType), []() {
            return DGuiApplicationHelper::LightType;
        });
        w->setWaitDialogColor();
        QTest::qWait(500);
    }
}

TEST(MainWindow, loadZoomRatio_test)
{
    TEST_CASE_NAME("loadZoomRatio_test")
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    w->loadZoomRatio();
    QTest::qWait(500);
}

TEST(MainWindow, createShorcutJson_test)
{
    TEST_CASE_NAME("createShorcutJson_test")
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    w->createShorcutJson();
    QTest::qWait(500);
}

TEST(MainWindow, thumbnailZoomIn_test)
{
    TEST_CASE_NAME("thumbnailZoomIn_test")
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    w->thumbnailZoomIn();
    QTest::qWait(500);
}

TEST(MainWindow, thumbnailZoomOut_test)
{
    TEST_CASE_NAME("thumbnailZoomOut_test")
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    w->thumbnailZoomOut();
    QTest::qWait(500);
}

TEST(MainWindow, saveWindowState_test)
{
    TEST_CASE_NAME("saveWindowState_test")
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    w->saveWindowState();
    QTest::qWait(500);
}

TEST(MainWindow, compareVersion_test)
{
    TEST_CASE_NAME("compareVersion_test")
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    w->compareVersion();
    QTest::qWait(500);
}

TEST(MainWindow, onSearchEditFinished_test)
{
    TEST_CASE_NAME("onSearchEditFinished_test")
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
    TEST_CASE_NAME("onNewAPPOpen_test")
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    stub_ext::StubExt stu;
    stu.set_lamda(ADDR(DSearchEdit, text), []() {
        return "hello";
    });

    w->timeLineBtnClicked();
    w->albumBtnClicked();
    w->allPicBtnClicked();

    QStringList filelist;
    filelist << "noid" << ":/2e5y8y.jpg" << ":/2ejqyx.jpg" << ":/2k9o1m.png";
    w->onNewAPPOpen(1, filelist);
    QTest::qWait(500);
    w->onSearchEditFinished();
    QTest::qWait(500);
    emit dApp->signalM->SearchEditClear();
    emit dApp->signalM->ImportFailed();
    emit dApp->signalM->ImportSomeFailed(2, 1);
    QTestEventList e;
    e.addKeyPress(Qt::Key_Slash, Qt::ControlModifier | Qt::ShiftModifier, 10);
    e.simulate(w);

    QPoint p = w->pos();
    int wi = w->width();
    int h = w->height();
    e.addMouseMove(p + QPoint(wi / 2, h / 2));
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier);
    e.simulate(w);
    e.clear();
}

TEST(MainWindow, initRightKeyOrder)
{
    TEST_CASE_NAME("initRightKeyOrder")

    auto w = dApp->getMainWindow();

    auto m_iCurrentView_temp = w->m_iCurrentView;

    //分支1
    {
        w->m_pAllPicView->m_pStackedWidget->setCurrentIndex(0);
        w->m_iCurrentView = 1;
        QWidget *t = w->m_pTimeBtn;
        w->initRightKeyOrder(t);
    }

    //分支2
    {
        w->m_pAllPicView->m_pStackedWidget->setCurrentIndex(0);
        w->m_iCurrentView = 0;
        QWidget *t = w->m_pAllPicBtn;
        w->initRightKeyOrder(t);
    }

    //分支3
    {
        w->m_pAllPicView->m_pStackedWidget->setCurrentIndex(1);
        w->m_iCurrentView = 0;
        QWidget *t = w->m_pTimeBtn;
        w->initRightKeyOrder(t);
    }

    w->m_iCurrentView = m_iCurrentView_temp;
}

//三个界面的删除操作
TEST(MainWindow, picdelete)
{
    TEST_CASE_NAME("picdelete")
    MainWindow *w = dApp->getMainWindow();
    QTestEventList e;

    using TR_SUBORDINATE_t = PointerTypeGetter < decltype(w->m_pAllPicView->m_pThumbnailListView) >::type;
    //时间线
    clickToTimelineView();

    QPoint pr(80, 50);
    //选中第一张
    e.addMouseMove(pr, 20);
    e.addMouseDClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pr, 50);
    e.simulate(w->m_pTimeLineView->getThumbnailListView()->viewport());
    e.clear();
    QTest::qWait(300);

    int (*dlgexec1)() = []() {
        return 1;
    };
    typedef int (*fptr)(QDialog *);
    fptr fptrexec1 = reinterpret_cast<fptr>(&QDialog::exec);  //obtaining an address
    Stub stub1;
    stub1.set(fptrexec1, dlgexec1);

    auto menu1 = runContextMenu(w->m_pAllPicView->m_pThumbnailListView->viewport(), pr);
    runActionFromMenu(menu1, TR_SUBORDINATE_t::tr("Delete"));

    //相册界面
    clickToAlbumView();

    ThumbnailListView *firstThumb = w->m_pAlbumview->m_pImpTimeLineView->getListView();
    if (!firstThumb) {
        return;
    }

    QPoint p2 = firstThumb->viewport()->pos() + QPoint(30, 30); //已导入
    e.addMouseMove(p2);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p2, 50);
    e.simulate(firstThumb->viewport());
    e.clear();

    int (*dlgexec2)() = []() {
        return 1;
    };
    typedef int (*fptr)(QDialog *);
    fptr fptrexec2 = reinterpret_cast<fptr>(&QDialog::exec);  //obtaining an address
    Stub stub2;
    stub2.set(fptrexec2, dlgexec2);

    auto menu2 = runContextMenu(firstThumb->viewport(), p2);
    runActionFromMenu(menu2, TR_SUBORDINATE_t::tr("Delete"));

    ASSERT_TRUE(w != nullptr);
}

//TEST(TimeLineView, shiftandcontrol)
//{
//    TEST_CASE_NAME("shiftandcontrol")
//    MainWindow *w = dApp->getMainWindow();
//    TimeLineView *timelineview = w->m_pTimeLineView;
//    w->timeLineBtnClicked();

//    QTestEventList e;
//    e.addMouseClick(Qt::MouseButton::LeftButton);
//    e.simulate(w->getButG()->button(1));
//    e.clear();
////    CommandLine *commandline = w->m_commandLine;
////    ImageView *imageview = commandline->findChild<MainWidget *>("MainWidget")->m_viewPanel->m_viewB;
//    QPoint pr(80, 50);
//    //control选中
//    e.addMouseMove(pr, 20);
//    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::ControlModifier, pr, 50);
//    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::ControlModifier, pr + QPoint(140, 0), 50);
//    e.simulate(timelineview->getThumbnailListView()->viewport());
//    e.clear();
//    QTest::qWait(300);
//    //滑动滑块
//    QScrollBar *bar = timelineview->getThumbnailListView()->verticalScrollBar();
//    bar->setValue(bar->maximum());

//    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::ControlModifier, pr, 50);
//    e.addKeyClick(Qt::Key_Delete, Qt::NoModifier, 50);
//    e.simulate(timelineview->getThumbnailListView()->viewport());
//    e.clear();
//    QTest::qWait(300);

//    //删除dbus，过一下析构函数
//    w->m_pDBus->deleteLater();
//}

//标题栏创建
TEST(MainWindow, titlebarcreate)
{
    TEST_CASE_NAME("titlebarcreate")
    MainWindow *w = dApp->getMainWindow();

    w->allPicBtnClicked();
    QTestEventList e;
    e.addMouseClick(Qt::MouseButton::LeftButton);
    e.simulate(w->getButG()->button(0));
    e.clear();
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
    e.simulate(w);
    e.clear();

    QTimer::singleShot(2000, w, [ = ] {
        w->activateWindow();
        QWidget *c = w->findChild<AlbumCreateDialog *>();
        if (c)
        {
            DDialog *d = static_cast<DDialog *>(c);
            emit d->getButton(1)->click();
            c->close();
        }
    });

    QWidget *optionButton =  w->titlebar()->findChild<QWidget *>("DTitlebarDWindowOptionButton");
    if (optionButton) {
        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 100);
        e.simulate(optionButton);
        e.clear();
    }

    DMenu *menuWidget = w->titlebar()->menu();
    if (menuWidget) {
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 100);
        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 100);
        e.simulate(menuWidget);
        e.clear();
    }
    QTest::qWait(300);
}

//ctrl+滚轮
TEST(MainWindow, wheelEvent)
{
    TEST_CASE_NAME("wheelEvent")

    MainWindow *w = dApp->getMainWindow();

    QWheelEvent event_1(QPointF(0, 0), 10, Qt::LeftButton, Qt::ControlModifier);
    w->wheelEvent(&event_1);

    QWheelEvent event_2(QPointF(0, 0), -10, Qt::LeftButton, Qt::ControlModifier);
    w->wheelEvent(&event_2);

    QWheelEvent event_3(QPointF(0, 0), 10, Qt::LeftButton, Qt::NoModifier);
    w->wheelEvent(&event_3);
}

//从菜单关闭
TEST(MainWindow, closeFromMenu)
{
    TEST_CASE_NAME("closeFromMenu")

    MainWindow *w = dApp->getMainWindow();
    stub_ext::StubExt stu;

    //这里需要干掉instance，否则测试结束进行清理的时候会崩
    stu.set_lamda(&MainWindow::instance, [w]() -> MainWindow& {
        return *w;
    });
}

//搜索框
TEST(MainWindow, onSearchEditIsDisplay)
{
    TEST_CASE_NAME("onSearchEditIsDisplay")

    MainWindow *w = dApp->getMainWindow();

    auto temp = w->m_pCenterWidget->currentIndex();

    w->m_pCenterWidget->setCurrentIndex(2);
    w->onSearchEditIsDisplay(true);

    w->m_pCenterWidget->setCurrentIndex(1);
    w->onSearchEditIsDisplay(false);

    w->m_pCenterWidget->setCurrentIndex(temp);
}
