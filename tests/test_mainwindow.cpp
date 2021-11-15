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
#include "fileinotify.h"
#include "ac-desktop-define.h"
#include "qgesture.h"
#include "batchoperatewidget.h"
#include "expansionmenu.h"
#include "imageengineapi.h"

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

    //初始界面筛选可见性校验bug89836
    BatchOperateWidget *batch = w->m_pAllPicView->m_batchOperateWidget;
    if (batch) {
        ASSERT_FALSE(batch->isVisible());
    }

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

    w->startMonitor();
    QTest::qWait(2000);

    AllPicView *allpicview = w->m_pAllPicView;
    //绑定信号
    ImageEngineApi::instance()->thumbnailLoadThread(80);
    QString AVI = list.at(0) + "/500KAVI.AVI";
    ImageEngineApi::instance()->ImportImagesFromFileList(QStringList() << AVI, "", allpicview, true);
    QTest::qWait(1000);
    ImageEngineApi::instance()->ImportImagesFromFileList(list, "", allpicview, true);
    allpicview->update();
    QTest::qWait(2000);

    //判断视频大写后缀导入是否正常
    bool iscontain = ImageEngineApi::instance()->m_AllImageData.contains(AVI);
    qDebug() << "------" << AVI << iscontain;
//    EXPECT_TRUE(iscontain);

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
    EXPECT_TRUE(list.size() > 0);

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

    QTest::qWait(200);
    QPoint p1(60, 100);
    event.addMouseMove(p1);
    event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    event.addMouseDClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    event.simulate(allpicview->m_pThumbnailListView->viewport());
    QTest::qWait(1000);
    event.clear();

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

    //全屏
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Fullscreen"));
    QTest::qWait(1000);
    w->onHideImageView();
    QTest::qWait(500);

    //幻灯片
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Slide show"));
    QTest::qWait(2000);
    w->onHideImageView();
    QTest::qWait(500);

    //TODO:打印
    //fix：没有调用startSlideShow而直接用鼠标去点，导致UT崩溃
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

        QTestEventList e1;
        e1.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e1.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e1.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e1.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e1.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e1.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
        e1.simulate(Exporter::instance()->m_exportImageDialog->getButton(1));
        e1.clear();
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

    if (timelineview) {
        // ----右键菜单start----
        QPoint pr(60, 140);

        //选中第一张
        e.addMouseMove(pr, 20);
        e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pr, 50);
        e.simulate(timelineview->getThumbnailListView()->viewport());
        e.clear();
        QTest::qWait(1000);

        e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pr, 50);
        e.simulate(timelineview->getThumbnailListView()->viewport());
        e.clear();
        QTest::qWait(300);

        auto menu = runContextMenu(timelineview->getThumbnailListView()->viewport(), pr);
        using TR_SUBORDINATE_t = PointerTypeGetter < decltype(timelineview->getThumbnailListView()) >::type;

        runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Fullscreen"));
        QTest::qWait(1000);
        w->onHideImageView();
        QTest::qWait(500);

        runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Slide show"));
        QTest::qWait(2000);
        w->onHideImageView();
        QTest::qWait(500);

        //TODO:打印
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
    QScrollBar *thumbBar = timelineview->m_timeLineThumbnailListView->verticalScrollBar();
    int num = thumbBar->maximum();
    thumbBar->setValue(num / 2);
    QTest::qWait(500);
    bar1->m_pSlider->slider()->setValue(4);
    QTest::qWait(300);
    EXPECT_TRUE(bar1->m_pSlider->slider()->value() == 4);
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

    e.addMouseMove(w->m_pAlbumview->m_pLeftListView->m_pPhotoLibListView->viewport()->pos() + QPoint(10, 10));
    e.addMouseClick(Qt::MouseButton::LeftButton);
    e.addKeyClick(Qt::Key_Down, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Down, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Up, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Up, Qt::NoModifier, 50);
    e.simulate(w->m_pAlbumview->m_pLeftListView->m_pPhotoLibListView->viewport());
    e.clear();
    QTest::qWait(300);

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

    ThumbnailListView *firstThumb = albumview->m_pImpTimeLineView->getListView();
    if (!firstThumb) {
        return;
    }

    QPoint p1 = firstThumb->viewport()->pos() + QPoint(45, 110); //已导入
    e.addMouseMove(p1);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(firstThumb->viewport());
    e.clear();

    //------右键菜单start---------
    auto menu = runContextMenu(firstThumb->viewport(), p1);
    using TR_SUBORDINATE_t = PointerTypeGetter < decltype(firstThumb) >::type;

    //全屏
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Fullscreen"));
    QTest::qWait(1000);
    w->onHideImageView();
    QTest::qWait(500);

    //幻灯片
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Slide show"));
    QTest::qWait(2000);
    w->onHideImageView();
    QTest::qWait(500);

    //TODO:打印
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

    ASSERT_TRUE(w->m_pAlbumview->m_pImpTimeLineView->m_importTimeLineListView->selectedPaths().size() == 0);
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

    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Restore"));
}

TEST(MainWindow, favorite)
{
    TEST_CASE_NAME("favorite")
    MainWindow *w = dApp->getMainWindow();
    AlbumView *albumview = w->m_pAlbumview;
    QTestEventList e;
    ThumbnailListView *thumb = albumview->m_favoriteThumbnailList;

    clickToFavoritePage();

    //------右键菜单start---------
    QPoint p1 = thumb->viewport()->pos() + QPoint(80, 135); //收藏
    e.addMouseMove(p1);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(thumb->viewport());
    e.clear();

    auto menu = runContextMenu(thumb->viewport(), p1);
    using TR_SUBORDINATE_t = PointerTypeGetter < decltype(thumb) >::type;

    //全屏
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Fullscreen"));
    QTest::qWait(1000);
    w->onHideImageView();
    QTest::qWait(500);

    //幻灯片
    runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Slide show"));
    QTest::qWait(2000);
    w->onHideImageView();
    QTest::qWait(500);

    //TODO:打
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
    //所有界面搜索bug89849
    w->m_pSearchEdit->setText("a");
    w->onSearchEditFinished();//第一次搜索
    QTest::qWait(300);
    int first = w->m_pSearchView->m_pThumbnailListView->m_model->rowCount();//第一次结果
    w->onSearchEditFinished();//第二次搜索
    int second = w->m_pSearchView->m_pThumbnailListView->m_model->rowCount();//第二次结果
    qDebug() << "------" << first << second;
//    EXPECT_TRUE(first == second);

    QModelIndex index = w->m_pSearchView->m_pThumbnailListView->m_model->index(1, 0);
    if (index.isValid()) {
        QRect videoItem = w->m_pSearchView->m_pThumbnailListView->visualRect(index);
        QPoint p = QPoint(videoItem.x() + 5, videoItem.y() + 5);
        e.clear();

        auto menu = runContextMenu(w->m_pSearchView->m_pThumbnailListView->viewport(), p);
        using TR_SUBORDINATE_t = PointerTypeGetter < decltype(w->m_pSearchView->m_pThumbnailListView) >::type;
        //删除弹窗处理
        int (*dlgexec1)() = []() {
            return 0;
        };
        typedef int (*fptr)(QDialog *);
        fptr fptrexec1 = reinterpret_cast<fptr>(&QDialog::exec);  //obtaining an address
        Stub stub1;
        stub1.set(fptrexec1, dlgexec1);

        runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Delete"));
    }

    w->m_pSearchEdit->clear();
    w->m_pSearchEdit->setText("testNoreault");//无搜索结果
    w->onSearchEditFinished();
    QTest::qWait(300);

    //时间线搜索
    w->timeLineBtnClicked();
    w->m_pSearchEdit->setText("a");
    w->onSearchEditFinished();
    QTest::qWait(300);

    w->m_pSearchEdit->clear();
    w->m_pSearchEdit->setText("testNoreault");//无搜索结果
    w->onSearchEditFinished();
    QTest::qWait(300);

    //相册界面搜索
    w->albumBtnClicked();
    w->m_pSearchEdit->setText("a");
    w->onSearchEditFinished();
    QTest::qWait(300);

    w->m_pSearchEdit->clear();
    w->m_pSearchEdit->setText("testNoreault");//无搜索结果
    w->onSearchEditFinished();
    QTest::qWait(300);
    ASSERT_TRUE(w->m_pSearchEdit != nullptr);
}

// 从菜单创建相册
TEST(MainWindow, createalbumFromTitlebarMenu)
{
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
                tempDlg->onTextEdited("albumFromAction");
                tempDlg->onReturnPressed();
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
        filelist << testPath_Pictures + "/a.jpg" << testPath_Pictures + "/aa.jpg" << testPath_Pictures + "500Kavi.avi";
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

//标题栏创建
/*TEST(MainWindow, titlebarcreate)
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
}*/

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

TEST(ImgInfoDialog, DetailInfo)
{
    TEST_CASE_NAME("load")
    MainWindow *w = dApp->getMainWindow();
    AllPicView *allpicview = w->m_pAllPicView;
    QModelIndex idx;
    for (int i = 0; i < allpicview->m_pThumbnailListView->m_model->rowCount(); i++) {
        QModelIndex index = allpicview->getThumbnailListView()->m_model->index(i, 0);
        DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
        if (data.fileName == "DetailInfo.jpg") {
            idx = index;
            break;
        }
    }
    if (idx.isValid()) {
        QRect videoItem = allpicview->m_pThumbnailListView->visualRect(idx);
        QPoint p = QPoint(videoItem.x() + 5, videoItem.y() + 5);

        auto menu = runContextMenu(allpicview->m_pThumbnailListView->viewport(), p);
        using TR_SUBORDINATE_t = PointerTypeGetter < decltype(allpicview->m_pThumbnailListView) >::type;

        runActionFromMenu(menu, TR_SUBORDINATE_t::tr("Photo info"));
    }
    QTest::qWait(500);
    EXPECT_TRUE(w != nullptr);
}

TEST(ToolButton, BatchScreen)
{
    TEST_CASE_NAME("BatchScreen")
    MainWindow *w = dApp->getMainWindow();
    AllPicView *allpicview = w->m_pAllPicView;
    QTestEventList e;
    e.addMouseClick(Qt::MouseButton::LeftButton);
    e.simulate(w->getButG()->button(0));
    QTest::qWait(300);
    QTimer::singleShot(1500, w, [ = ] {
        allpicview->m_batchOperateWidget->m_expansionMenu->panel->hide();
    });
    allpicview->m_batchOperateWidget->m_ToolButton->onClicked();
    QTest::qWait(500);
}

TEST(MainWindow, ImportButton)
{
    TEST_CASE_NAME("ImportButton")
    MainWindow *w = dApp->getMainWindow();

    {
        int (*dlgexec)() = []() {
            return 1;
        };
        typedef int (*fptr)(QDialog *);
        fptr fptrexec = reinterpret_cast<fptr>(&QDialog::exec);  //obtaining an address
        Stub stub;
        stub.set(fptrexec, dlgexec);
        QTest::qWait(300);

        auto button = w->m_addImageBtn;
        button->click();
    }
}
