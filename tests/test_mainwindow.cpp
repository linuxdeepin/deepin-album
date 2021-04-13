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
#include "imgdeletedialog.h"
#include "navigationwidget.h"
#include "fileinotify.h"
#include "ac-desktop-define.h"
#include "qgesture.h"

#include <stub-tool/cpp-stub/stub.h>
#include <stub-tool/stub-ext/stubext.h>


//初始无图界面tab切换
TEST(MainWindow, noPicTab)
{
    TEST_CASE_NAME("noPicTab")
    MainWindow *w = dApp->getMainWindow();
    w->onLoadingFinished();
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
    TEST_CASE_NAME("Picimport")
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
    w->onLoadingFinished();
    w->getButG();
    w->allPicBtnClicked();

    AllPicView *allpicview = w->m_pAllPicView;
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

    CommandLine *commandline = w->m_commandLine;
    MainWidget *pmainwidget = nullptr;
    ViewPanel *viewpanel = nullptr;
    TTBContent *ttbc = nullptr;
    ImageView *imageview = nullptr;

    if (commandline)
        pmainwidget = commandline->findChild<MainWidget *>("MainWidget");
    if (pmainwidget)
        viewpanel = pmainwidget->m_viewPanel;

    QTest::qWait(200);
    QPoint p1(30, 100);
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
    QTest::qWait(300);

    //------右键菜单start---------
    QContextMenuEvent menuEvent(QContextMenuEvent::Mouse, p1);
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent);
    QTest::qWait(300);

    QTestEventList e;

    //查看
    DMenu *menuWidget = static_cast<DMenu *>(qApp->activePopupWidget());
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(1000);
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    QTest::qWait(300);

    //全屏
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(1000);
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    QTest::qWait(300);

    //TODO:打印

    //幻灯片
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(15000);

    SlideShowPanel *slideshowpanel = w->m_slidePanel;
    if (slideshowpanel) {
        SlideShowBottomBar *sliderbar = slideshowpanel->slideshowbottombar;
        if (sliderbar) {
            e.addMouseClick(Qt::MouseButton::LeftButton);
            DIconButton *preButton = sliderbar->findChild<DIconButton *>("SliderPreButton");
            DIconButton *NextButton = sliderbar->findChild<DIconButton *>("SliderNextButton");
            DIconButton *Play_PauseButton = sliderbar->findChild<DIconButton *>("SliderPlayPauseButton");
            DIconButton *ExitButton = sliderbar->findChild<DIconButton *>("SliderExitButton");
            if (NextButton) {
                e.simulate(NextButton);
                QTest::qWait(500);
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
        int (*dlgexec)() = []() {return 1;};
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

    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(1000);

    //复制7
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(300);

    //删除后重新选中最新的第一张
    e.addMouseMove(p1);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(allpicview->m_pThumbnailListView->viewport());
    e.clear();
    QTest::qWait(200);

    //收藏9
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(100);

    //顺时针10
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(1500);

    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(allpicview->m_pThumbnailListView->viewport());
    e.clear();
    QTest::qWait(200);

    //逆时针11
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent);;
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(1500);

    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(allpicview->m_pThumbnailListView->viewport());
    e.clear();
    QTest::qWait(200);

    //设为壁纸12
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(100);

    //文管显示13
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(300);
    w->raise();
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(allpicview->m_pThumbnailListView->viewport());
    e.clear();
    QTest::qWait(200);

    //照片信息14
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(500);

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
    QContextMenuEvent menuEvent2(QContextMenuEvent::Mouse, p2);

    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.addMouseMove(p2);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::ControlModifier, p2, 50);
    e.simulate(allpicview->m_pThumbnailListView->viewport());
    e.clear();
    QTest::qWait(300);

    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent2);
    QTest::qWait(300);
    DMenu *menuWidget2 = static_cast<DMenu *>(qApp->activePopupWidget());

    //幻灯片
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent2);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget2);
    e.clear();
    QTest::qWait(1000);

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
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent2);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget2);
    e.clear();
    QTest::qWait(500);

    w->resize(980, 600);
    ASSERT_TRUE(w != nullptr);
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

    QContextMenuEvent menuEvent(QContextMenuEvent::Mouse, p1);
    qApp->sendEvent(wid->m_viewPanel->m_viewB->viewport(), &menuEvent);
    QTest::qWait(300);
    DMenu *menuWidget = static_cast<DMenu *>(qApp->activePopupWidget());
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(400);

    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(wid->m_viewPanel->m_viewB->viewport());
    e.clear();
    QTest::qWait(300);

    qApp->sendEvent(wid->m_viewPanel->m_viewB->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(400);
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(wid->m_viewPanel->m_viewB->viewport());
    e.clear();
    QTest::qWait(300);

    QTimer::singleShot(1000, w, [ = ]() {
        //导出重复时，干掉覆盖提示框
        int (*dlgexec)() = []() {return 1;};
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

    qApp->sendEvent(wid->m_viewPanel->m_viewB->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(400);

    qApp->sendEvent(wid->m_viewPanel->m_viewB->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(400);

    qApp->sendEvent(wid->m_viewPanel->m_viewB->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(400);
    //取消收藏
    e.addKeyClick(Qt::Key_Period, Qt::NoModifier, 50);
    e.simulate(wid->m_viewPanel->m_viewB->viewport());
    e.clear();
    QTest::qWait(400);

    qApp->sendEvent(wid->m_viewPanel->m_viewB->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(2000);

    qApp->sendEvent(wid->m_viewPanel->m_viewB->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(2000);

    qApp->sendEvent(wid->m_viewPanel->m_viewB->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(500);

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

    qApp->sendEvent(wid->m_viewPanel->m_viewB->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(500);

    qApp->sendEvent(wid->m_viewPanel->m_viewB->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(500);

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

    CommandLine *commandline = w->m_commandLine;
    ImageView *imageview = commandline->findChild<MainWidget *>("MainWidget")->m_viewPanel->m_viewB;

    if (timelineview) {
        // ----右键菜单start----
        QPoint pr(80, 50);
        QContextMenuEvent menuEvent(QContextMenuEvent::Mouse, pr);
        //选中第一张
        e.addMouseMove(pr, 20);
        e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pr, 50);
        e.addMouseDClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pr, 50);
        e.simulate(timelineview->m_allThumbnailListView[0]->viewport());
        e.clear();
        QTest::qWait(1000);

        e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
        e.simulate(imageview->viewport());
        e.clear();
        QTest::qWait(300);

        e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pr, 50);
        e.simulate(timelineview->m_allThumbnailListView[0]->viewport());
        e.clear();
        QTest::qWait(300);

        qApp->sendEvent(timelineview->m_allThumbnailListView[0]->viewport(), &menuEvent);
        QTest::qWait(300);
        //查看
        DMenu *menuWidget = static_cast<DMenu *>(qApp->activePopupWidget());
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
        e.simulate(menuWidget);
        e.clear();
        QTest::qWait(1000);
        e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
        e.simulate(imageview->viewport());
        e.clear();
        QTest::qWait(300);
        //全屏
        qApp->sendEvent(timelineview->m_allThumbnailListView[0]->viewport(), &menuEvent);
        QTest::qWait(300);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
        e.simulate(menuWidget);
        e.clear();
        QTest::qWait(1000);
        e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
        e.simulate(imageview->viewport());
        e.clear();
        QTest::qWait(300);
        //TODO:打印

        //幻灯片
        qApp->sendEvent(timelineview->m_allThumbnailListView[0]->viewport(), &menuEvent);
        QTest::qWait(300);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
        e.simulate(menuWidget);
        e.clear();
        QTest::qWait(300);
        e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
        e.simulate(imageview->viewport());
        e.clear();
        QTest::qWait(300);
        //复制7
        qApp->sendEvent(timelineview->m_allThumbnailListView[0]->viewport(), &menuEvent);
        QTest::qWait(300);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
        e.simulate(menuWidget);
        e.clear();
        QTest::qWait(300);
        //TODO:删除
        //收藏9
        qApp->sendEvent(timelineview->m_allThumbnailListView[0]->viewport(), &menuEvent);
        QTest::qWait(300);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
        e.simulate(menuWidget);
        e.clear();
        QTest::qWait(100);
        //顺时针10
        qApp->sendEvent(timelineview->m_allThumbnailListView[0]->viewport(), &menuEvent);
        QTest::qWait(300);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
        e.simulate(menuWidget);
        e.clear();
        QTest::qWait(1500);
        e.addMouseMove(pr, 20);
        e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pr, 50);
        e.simulate(timelineview->m_allThumbnailListView[0]->viewport());
        e.clear();
        QTest::qWait(300);
        //逆时针11
        qApp->sendEvent(timelineview->m_allThumbnailListView[0]->viewport(), &menuEvent);;
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
        e.simulate(menuWidget);
        e.clear();
        QTest::qWait(1500);

        e.addMouseMove(pr, 20);
        e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pr, 50);
        e.simulate(timelineview->m_allThumbnailListView[0]->viewport());
        e.clear();
        QTest::qWait(300);
        //设为壁纸12
        qApp->sendEvent(timelineview->m_allThumbnailListView[0]->viewport(), &menuEvent);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
        e.simulate(menuWidget);
        e.clear();
        QTest::qWait(100);
        //文管显示13
        qApp->sendEvent(timelineview->m_allThumbnailListView[0]->viewport(), &menuEvent);
        QTest::qWait(300);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
        e.simulate(menuWidget);
        e.clear();
        QTest::qWait(300);
        w->raise();
        QTest::qWait(300);
        e.addMouseMove(pr, 20);
        e.addMouseClick(Qt::MouseButton::LeftButton);
        e.simulate(timelineview->m_allThumbnailListView[0]->viewport());
        e.clear();
        QTest::qWait(300);
        //照片信息14
        qApp->sendEvent(timelineview->m_allThumbnailListView[0]->viewport(), &menuEvent);
        QTest::qWait(300);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
        e.simulate(menuWidget);
        e.clear();
        QTest::qWait(500);

        QList<DCommandLinkButton *> btns = timelineview->m_allChoseButton;
        QTestEventList e1;
        e1.addMouseClick(Qt::MouseButton::LeftButton);
        for (int i = 0; i < btns.size(); i++) { //选中全部
            e1.simulate(btns.at(i));
            QTest::qWait(200);
        }
        e1.clear();

        qApp->sendEvent(timelineview->m_allThumbnailListView[0]->viewport(), &menuEvent);
        QTest::qWait(300);
        DMenu *menuWidget3 = static_cast<DMenu *>(qApp->activePopupWidget());

        //幻灯片
        qApp->sendEvent(timelineview->m_allThumbnailListView[0]->viewport(), &menuEvent);
        QTest::qWait(300);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
        e.simulate(menuWidget3);
        e.clear();
        QTest::qWait(1000);


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
        //导出-d
//        qApp->sendEvent(timelineview->m_allThumbnailListView[0]->viewport(), &menuEvent2);
//        QTest::qWait(300);
//        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
//        e.simulate(menuWidget3);
//        e.clear();
//        QTest::qWait(500);

        e1.addMouseMove(pr);
        e1.simulate(timelineview->m_allThumbnailListView[0]->viewport());
        e1.clear();
        QTest::qWait(300);

        //复制
        qApp->sendEvent(timelineview->m_allThumbnailListView[0]->viewport(), &menuEvent);
        QTest::qWait(300);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
        e.simulate(menuWidget3);
        e.clear();
        QTest::qWait(500);
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
    ThumbnailListView *firstThumb = albumview->m_pImpTimeLineView->getFirstListView();
    if (!firstThumb) {
        return;
    }

    DCommandLinkButton *btn = albumview->m_pImpTimeLineView->m_allChoseButton.at(0);
    QPoint btn1 = QPoint(btn->geometry().x(), btn->geometry().y());
    e.addMouseMove(btn1);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, btn1, 500);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, btn1, 500);
    e.simulate(btn);
    e.clear();
    QTest::qWait(500);

    QPoint p1 = firstThumb->viewport()->pos() + QPoint(30, 30); //已导入
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

    albumview->m_pImpTimeLineView->getCurrentSelectPics();

    //------右键菜单start---------
    QContextMenuEvent menuEvent(QContextMenuEvent::Mouse, p1);
    qApp->sendEvent(firstThumb->viewport(), &menuEvent);
    QTest::qWait(300);

    //查看
    DMenu *menuWidget = static_cast<DMenu *>(qApp->activePopupWidget());
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(1000);
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    QTest::qWait(300);

    //全屏
    qApp->sendEvent(firstThumb->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(1000);
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    QTest::qWait(300);
    //TODO:打印

    //幻灯片
    qApp->sendEvent(firstThumb->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(300);

    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    QTest::qWait(300);

    //复制7
    qApp->sendEvent(firstThumb->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(300);

    //收藏9
    qApp->sendEvent(firstThumb->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(100);

    //顺时针10
    qApp->sendEvent(firstThumb->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(1500);

    //文件有变动，需要重新获取指针
    ThumbnailListView *t = w->m_pAlbumview->m_pImpTimeLineView->getFirstListView();
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(t->viewport());
    e.clear();
    QTest::qWait(200);

    DMenu *menuWidget1 = static_cast<DMenu *>(qApp->activePopupWidget());
    //逆时针11
    qApp->sendEvent(t->viewport(), &menuEvent);;
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget1);
    e.clear();
    QTest::qWait(1500);

    //文件有变动，需要重新获取指针
    ThumbnailListView *t1 = w->m_pAlbumview->m_pImpTimeLineView->getFirstListView();
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(t1->viewport());
    e.clear();
    QTest::qWait(200);

    //设为壁纸12
    qApp->sendEvent(t1->viewport(), &menuEvent);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget1);
    e.clear();
    QTest::qWait(100);

    //文管显示13
    qApp->sendEvent(t1->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget1);
    e.clear();
    QTest::qWait(300);
    w->raise();
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(t1->viewport());
    e.clear();
    QTest::qWait(200);

    //照片信息14
    qApp->sendEvent(t1->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget1);
    e.clear();
    QTest::qWait(500);

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
    QContextMenuEvent menuEvent2(QContextMenuEvent::Mouse, p2);

    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.addMouseMove(p2);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::ControlModifier, p2, 50);
    e.addMouseMove(p3);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::ShiftModifier, p3, 50);
    e.addKeyClick(Qt::Key_A, Qt::ControlModifier, 50);
    e.simulate(t1->viewport());
    e.clear();
    QTest::qWait(300);

    qApp->sendEvent(t1->viewport(), &menuEvent2);
    QTest::qWait(300);
    DMenu *menuWidget2 = static_cast<DMenu *>(qApp->activePopupWidget());

    //幻灯片
    qApp->sendEvent(t1->viewport(), &menuEvent2);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget2);
    e.clear();
    QTest::qWait(1000);

    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    //导出-d

    //复制
    qApp->sendEvent(t1->viewport(), &menuEvent2);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget2);
    e.clear();
    QTest::qWait(500);

    //删除-d

    QList<DCommandLinkButton *> btns = albumview->m_pImpTimeLineView->m_allChoseButton;
    QTestEventList e1;
    e1.addMouseClick(Qt::MouseButton::LeftButton);
    for (int i = 0; i < btns.size(); i++) { //选中全部
        e1.simulate(btns.at(i));
        QTest::qWait(200);
    }
    e1.clear();

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
    e.addMouseClick(Qt::MouseButton::LeftButton);
    w->albumBtnClicked();
    e.simulate(w->getButG()->button(2));
    e.clear();

    ThumbnailListView *thumb = albumview->m_pRightTrashThumbnailList;
    LeftListView *leftview = albumview->m_pLeftListView;
    QPoint p = leftview->m_pPhotoLibListView->item(1)->view->pos();
    e.addMouseMove(p + QPoint(50, 5));
    e.addMouseClick(Qt::MouseButton::LeftButton);
    e.simulate(leftview->m_pPhotoLibListView->viewport());
    e.clear();
    QTest::qWait(300);

    QPoint p1 = thumb->viewport()->pos() + QPoint(30, 30); //最近删除
    e.addMouseMove(p1);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(thumb->viewport());
    e.clear();
    QTest::qWait(300);

    QContextMenuEvent menuEvent(QContextMenuEvent::Mouse, p1);
    qApp->sendEvent(thumb->viewport(), &menuEvent);
    QTest::qWait(300);
    DMenu *menuWidget = static_cast<DMenu *>(qApp->activePopupWidget());
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(1000);

    ThumbnailListView *thumbNew = w->m_pAlbumview->m_pRightTrashThumbnailList;
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(thumbNew->viewport());
    e.clear();
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(200);
    emit w->m_pAlbumview->m_pRecoveryBtn->clicked();
    ASSERT_TRUE(w->m_pAlbumview->m_pRightTrashThumbnailList != nullptr);
}

TEST(MainWindow, favorite)
{
    TEST_CASE_NAME("favorite")
    MainWindow *w = dApp->getMainWindow();
    AlbumView *albumview = w->m_pAlbumview;
    ImageView *imageview = w->m_commandLine->findChild<MainWidget *>("MainWidget")->m_viewPanel->m_viewB;
    QTestEventList e;
    e.addMouseClick(Qt::MouseButton::LeftButton);
    w->albumBtnClicked();
    e.simulate(w->getButG()->button(2));
    e.clear();
    ThumbnailListView *thumb = albumview->m_pRightFavoriteThumbnailList;
    LeftListView *leftview = albumview->m_pLeftListView;
    QPoint p = leftview->m_pPhotoLibListView->item(2)->view->pos();
    qDebug() << " " << p;
    e.addMouseMove(p + QPoint(50, 50));
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p + QPoint(50, 50), 50);
    e.simulate(leftview->m_pPhotoLibListView->viewport());
    e.clear();
    QTest::qWait(300);

    //------右键菜单start---------
    QPoint p1 = thumb->viewport()->pos() + QPoint(30, 30); //收藏
    e.addMouseMove(p1);
    e.addMouseDClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(thumb->viewport());
    e.clear();
    QTest::qWait(1000);
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    QTest::qWait(300);

    QContextMenuEvent menuEvent(QContextMenuEvent::Mouse, p1);
    qApp->sendEvent(thumb->viewport(), &menuEvent);
    QTest::qWait(300);

    //查看
    DMenu *menuWidget = static_cast<DMenu *>(qApp->activePopupWidget());
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(1000);
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    QTest::qWait(300);

    //全屏
    qApp->sendEvent(thumb->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(1000);
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    QTest::qWait(300);
    //TODO:打印

    //幻灯片
    qApp->sendEvent(thumb->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(300);

    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    QTest::qWait(300);

    //复制7
    qApp->sendEvent(thumb->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(300);

    //顺时针10
    qApp->sendEvent(thumb->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(1500);

    //文件有变动，需要重新获取指针
    ThumbnailListView *t = w->m_pAlbumview->m_pRightFavoriteThumbnailList;
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(t->viewport());
    e.clear();
    QTest::qWait(200);

    DMenu *menuWidget1 = static_cast<DMenu *>(qApp->activePopupWidget());
    //逆时针11
    qApp->sendEvent(t->viewport(), &menuEvent);;
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget1);
    e.clear();
    QTest::qWait(1500);

    //文件有变动，需要重新获取指针
    ThumbnailListView *t1 = w->m_pAlbumview->m_pRightFavoriteThumbnailList;
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(t1->viewport());
    e.clear();
    QTest::qWait(200);

    //设为壁纸12
    qApp->sendEvent(t1->viewport(), &menuEvent);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget1);
    e.clear();
    QTest::qWait(100);

    //文管显示13
    qApp->sendEvent(t1->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget1);
    e.clear();
    QTest::qWait(300);
    w->raise();
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(t1->viewport());
    e.clear();
    QTest::qWait(200);

    //照片信息14
    qApp->sendEvent(t1->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget1);
    e.clear();
    QTest::qWait(500);

    //收藏9
    qApp->sendEvent(t1->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(100);
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
    int (*dlgexec)() = []() {return 1;};
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

    int (*dlgexec)() = []() {return 1;};
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

    stub_ext::StubExt stu;
    stu.set_lamda(ADDR(DGuiApplicationHelper, themeType), []() {
        return DGuiApplicationHelper::DarkType;
    });

    w->setWaitDialogColor();
    QTest::qWait(500);
    stu.set_lamda(ADDR(DGuiApplicationHelper, themeType), []() {
        return DGuiApplicationHelper::LightType;
    });
    w->setWaitDialogColor();
    QTest::qWait(500);
}

TEST(MainWindow, onShowImageInfo_test)
{
    TEST_CASE_NAME("onShowImageInfo_test")
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

TEST(MainWindow, callFuncitons_test)
{
    TEST_CASE_NAME("callFuncitons_test")
    //TEST_CASE_NAME("load")

    ViewPanel viewPanel;
    QStringList strList;
    //strList << "0" << "1";
    strList << ":/2e5y8y.jpg" << ":/2ejqyx.jpg" << ":/2k9o1m.png";

    TTBContent ttbContent(true, strList);
    ttbContent.updateFilenameLayout();
    ttbContent.onImgListViewTestloadRight();
    ttbContent.setCurrentItem();
    ttbContent.setLeftlist(strList);
    ttbContent.onImgListViewTestloadLeft();
    ttbContent.setRightlist(strList);
    ttbContent.onImgListViewTestloadRight();

    viewPanel.onLoadLeft(strList);
    viewPanel.onttbcontentClicked();
    viewPanel.onRemoved();
    viewPanel.onViewBClicked();
    viewPanel.onViewBImageChanged(" ");
    viewPanel.removeCurrentImage();
    viewPanel.rotateImage(true);
    viewPanel.rotateImage(false);
    viewPanel.onMouseHoverMoved();

    viewPanel.appendAction_darkmenu(0, " ", "");
    QAction action;
    viewPanel.onMenuItemClicked(&action);

    QWidget *parent = nullptr;
    NavigationWidget *navigationWidget = new NavigationWidget(parent);
    QMouseEvent mouseEvent(QMouseEvent::None, QPointF(100, 100), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    navigationWidget->mousePressEvent(&mouseEvent);
    navigationWidget->mouseMoveEvent(&mouseEvent);
    navigationWidget->tryMoveRect(QPoint(1, 1));

    QRegion region(1, 1, 1, 1);
    QPaintEvent paintevent(region);
    navigationWidget->paintEvent(&paintevent);

    FileInotify fileinotify;
    fileinotify.isVaild();
    fileinotify.addWather(" ");
//    fileinotify.removeWatcher(" ");
    fileinotify.getAllPicture(true);
    fileinotify.getAllPicture(false);
    fileinotify.pathLoadOnce();

    QObject obj;
    MainWindow *w = dApp->getMainWindow();

    w->initRightKeyOrder(&obj);
    w->initUpKeyOrder();
    w->compareVersion();
    w->onButtonClicked(0);
    w->onButtonClicked(1);
    w->onButtonClicked(2);
    w->onImportSuccess();
    w->onImportFailed();
    w->onEscShortcutActivated();
    w->onDelShortcutActivated();
    w->onSearchEditClear();
    w->onCtrlFShortcutActivated();

    w->m_slidePanel->onThemeChanged(ViewerThemeManager::Dark);

    QTimerEvent timeEvent(100);
    w->m_slidePanel->timerEvent(&timeEvent);
    w->m_pSearchView->onFinishLoad();
    w->m_pSearchView->onKeyDelete();

    AlbumImageButton imageButton;
    imageButton.leaveEvent(&mouseEvent);

    AlbumDeleteDialog deleteDialog;
    QKeyEvent keyevent(QKeyEvent::None, 1, Qt::NoModifier, " ");
    deleteDialog.keyPressEvent(&keyevent);
    deleteDialog.onButtonClicked(0, " ");
    deleteDialog.onButtonClicked(1, " ");

    ImportTimeLineView importTimeLineView(nullptr);
    QMimeData mimedata;
    QDragEnterEvent dragenterevent(QPoint(100, 100), Qt::DropAction::IgnoreAction, &mimedata, Qt::MouseButton::LeftButton, Qt::NoModifier);
    importTimeLineView.dragEnterEvent(&dragenterevent);
    QDropEvent dropEvent(QPoint(100, 100), Qt::DropAction::IgnoreAction, &mimedata, Qt::MouseButton::LeftButton, Qt::NoModifier);
    importTimeLineView.dropEvent(&dropEvent);
    QDragMoveEvent dragMoveEvent(QPoint(100, 100), Qt::DropAction::IgnoreAction, &mimedata, Qt::MouseButton::LeftButton, Qt::NoModifier);
    importTimeLineView.dragMoveEvent(&dragMoveEvent);
    QDragLeaveEvent dragLeaveEvent;
    importTimeLineView.dragLeaveEvent(&dragLeaveEvent);
    importTimeLineView.mousePressEvent(&mouseEvent);

    ImageItem imageItem;
    imageItem.mouseReleaseEvent(&mouseEvent);
    imageItem.mousePressEvent(&mouseEvent);
    imageItem.setIndex(1);
    imageItem.emitClickSig();



    ImageView imageView;
    imageView.mousePressEvent(&mouseEvent);
    imageView.mouseMoveEvent(&mouseEvent);
    const QList<QGesture *> gestures;
    QGestureEvent gestureEvent(gestures);
    imageView.handleGestureEvent(&gestureEvent);
    QPinchGesture pinchGesture(&obj);
    imageView.pinchTriggered(&pinchGesture);
    QWheelEvent wheelEvent(QPointF(100, 100), 1, Qt::LeftButton, Qt::NoModifier, Qt::Orientation::Vertical);
    imageView.wheelEvent(&wheelEvent);

    SlideShowButton slideShowButton;
    slideShowButton.enterEvent(&wheelEvent);
    slideShowButton.mouseEvent(&mouseEvent);
}

//三个界面的删除操作
TEST(MainWindow, picdelete)
{
    TEST_CASE_NAME("picdelete")
    MainWindow *w = dApp->getMainWindow();
    QTestEventList e;
    e.addMouseClick(Qt::MouseButton::LeftButton);
    e.simulate(w->getButG()->button(0));
    e.clear();
    QTest::qWait(300);

    //------右键删除---------
    QPoint p1(30, 100);
    e.addMouseMove(p1);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(w->m_pAllPicView->m_pThumbnailListView->viewport());
    e.clear();

    QContextMenuEvent menuEvent(QContextMenuEvent::Mouse, p1);
    qApp->sendEvent(w->m_pAllPicView->m_pThumbnailListView->viewport(), &menuEvent);
    QTest::qWait(2000);

    //所有照片
    int (*dlgexec)() = []() {return 1;};
    typedef int (*fptr)(QDialog *);
    fptr fptrexec = reinterpret_cast<fptr>(&QDialog::exec);  //obtaining an address
    Stub stub;
    stub.set(fptrexec, dlgexec);

    DMenu *menuWidget = static_cast<DMenu *>(qApp->activePopupWidget());
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(500);

    //时间线
    e.addMouseClick(Qt::MouseButton::LeftButton);
    e.simulate(w->getButG()->button(1));
    e.clear();
    QTest::qWait(300);

    QPoint pr(80, 50);
    QContextMenuEvent menuEvent1(QContextMenuEvent::Mouse, pr);
    //选中第一张
    e.addMouseMove(pr, 20);
    e.addMouseDClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pr, 50);
    e.simulate(w->m_pTimeLineView->m_allThumbnailListView[0]->viewport());
    e.clear();
    QTest::qWait(300);

    qApp->sendEvent(w->m_pAllPicView->m_pThumbnailListView->viewport(), &menuEvent1);
    QTest::qWait(2000);

    int (*dlgexec1)() = []() {return 1;};
    typedef int (*fptr)(QDialog *);
    fptr fptrexec1 = reinterpret_cast<fptr>(&QDialog::exec);  //obtaining an address
    Stub stub1;
    stub1.set(fptrexec1, dlgexec1);

    DMenu *menuWidget1 = static_cast<DMenu *>(qApp->activePopupWidget());
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget1);
    e.clear();
    QTest::qWait(500);

    //相册界面
    e.addMouseClick(Qt::MouseButton::LeftButton);
    e.simulate(w->getButG()->button(2));
    e.clear();
    QTest::qWait(300);
    ThumbnailListView *firstThumb = w->m_pAlbumview->m_pImpTimeLineView->getFirstListView();
    if (!firstThumb) {
        return;
    }

    QPoint p2 = firstThumb->viewport()->pos() + QPoint(30, 30); //已导入
    e.addMouseMove(p2);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p2, 50);
    e.simulate(firstThumb->viewport());
    e.clear();
    QTest::qWait(2000);
    QContextMenuEvent menuEvent2(QContextMenuEvent::Mouse, p2);
    qApp->sendEvent(firstThumb->viewport(), &menuEvent2);

    int (*dlgexec2)() = []() {return 1;};
    typedef int (*fptr)(QDialog *);
    fptr fptrexec2 = reinterpret_cast<fptr>(&QDialog::exec);  //obtaining an address
    Stub stub2;
    stub2.set(fptrexec2, dlgexec2);

    DMenu *menuWidget2 = static_cast<DMenu *>(qApp->activePopupWidget());
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget2);
    e.clear();
    QTest::qWait(500);
    ASSERT_TRUE(w != nullptr);
}

TEST(TimeLineView, shiftandcontrol)
{
    TEST_CASE_NAME("shiftandcontrol")
    MainWindow *w = dApp->getMainWindow();
    TimeLineView *timelineview = w->m_pTimeLineView;
    w->timeLineBtnClicked();

    QTestEventList e;
    e.addMouseClick(Qt::MouseButton::LeftButton);
    e.simulate(w->getButG()->button(1));
    e.clear();
//    CommandLine *commandline = w->m_commandLine;
//    ImageView *imageview = commandline->findChild<MainWidget *>("MainWidget")->m_viewPanel->m_viewB;
    QPoint pr(80, 50);
    //control选中
    e.addMouseMove(pr, 20);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::ControlModifier, pr, 50);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::ControlModifier, pr + QPoint(140, 0), 50);
    e.simulate(timelineview->m_allThumbnailListView[0]->viewport());
    e.clear();
    QTest::qWait(300);
    //滑动滑块
    QScrollBar *bar = timelineview->m_mainListWidget->verticalScrollBar();
    bar->setValue(bar->maximum());

    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::ControlModifier, pr, 50);
    e.addKeyClick(Qt::Key_Delete, Qt::NoModifier, 50);
    e.simulate(timelineview->m_allThumbnailListView[0]->viewport());
    e.clear();
    QTest::qWait(300);
}

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
