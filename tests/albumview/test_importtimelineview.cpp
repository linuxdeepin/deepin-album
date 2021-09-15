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
#include <QTestEventList>
#include <QObject>
#include <QDialog>
#include <QStringList>

#include <DSearchEdit>
#include <DFileDialog>

#define private public
#define protected public

#include "mainwindow.h"
#include "importtimelineview.h"
#include "albumcreatedialog.h"
#include "test_qtestDefine.h"
#include "imginfodialog.h"
#include "imageengine/imageengineapi.h"
#include "viewpanel.h"
#include "ac-desktop-define.h"

#include <stub-tool/cpp-stub/stub.h>
#include <stub-tool/stub-ext/stubext.h>


TEST(ImportTimeLineView, getIBaseHeight_test)
{
    TEST_CASE_NAME("getIBaseHeight_test")
    MainWindow *w = dApp->getMainWindow();
    w->albumBtnClicked();
    QTest::qWait(500);

    ImportTimeLineView *impTimeline = w->m_pAlbumview->m_pImpTimeLineView;
    int a = 0;
    stub_ext::StubExt stu;
    stu.set_lamda(ADDR(DSlider, value), [&a]() {
        return a++;
    });
    for (int i = 0; i < 10; i++) {
        impTimeline->getIBaseHeight();
    }
    impTimeline->onSuspensionChoseBtnClicked();
    impTimeline->slotNoPicOrNoVideo(true);
    impTimeline->selectPaths();
    QTest::qWait(500);
}


TEST(ImportTimeLineView, test_func)
{
    TEST_CASE_NAME("test_func")
    MainWindow *w = dApp->getMainWindow();
    w->albumBtnClicked();
    QTest::qWait(500);

    ImportTimeLineView *impTimeline = w->m_pAlbumview->m_pImpTimeLineView;
    impTimeline->updateChoseText();

    QTest::qWait(500);

    w->allPicBtnClicked();
    w->albumBtnClicked();
    QTest::qWait(500);
}

TEST(ImportTimeLineView, on_KeyEvent_test)
{
    TEST_CASE_NAME("on_KeyEvent_test")
    MainWindow *w = dApp->getMainWindow();
    w->albumBtnClicked();
    QTest::qWait(500);
//todo
//    ImportTimeLineView *impTimeline = w->m_pAlbumview->m_pImpTimeLineView;
//    impTimeline->on_KeyEvent(Qt::Key_PageDown);
//    QTest::qWait(100);
//    impTimeline->on_KeyEvent(Qt::Key_PageUp);
//    QTest::qWait(500);
}

TEST(ImportTimeLineView, resizeHand_test)
{
    TEST_CASE_NAME("resizeHand_test")
    MainWindow *w = dApp->getMainWindow();
    w->albumBtnClicked();
    QTest::qWait(500);
    ImportTimeLineView *impTimeline = w->m_pAlbumview->m_pImpTimeLineView;
}

TEST(ImportTimeLineView, thumbnaillistViewSlot_test)
{
    TEST_CASE_NAME("thumbnaillistViewSlot_test")
    QTest::qWait(500);
    MainWindow *w = dApp->getMainWindow();
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(2));
    event.clear();
    QTest::qWait(300);
    AlbumView *a = w->m_pAlbumview;
    event.addMousePress(Qt::MouseButton::LeftButton, Qt::NoModifier, QPoint(10, 10));
    event.addMouseRelease(Qt::MouseButton::LeftButton, Qt::NoModifier, QPoint(10, 10));
    event.simulate(a->m_pLeftListView->m_pPhotoLibListView->viewport());
    QTest::qWait(1000);
}

//已导入页面再次导入图片
TEST(ImportTimeLineView, Picimport)
{
    TEST_CASE_NAME("load")
    QStringList list;
    QStringList listtemp = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
    if (listtemp.size() > 0) {
        list << listtemp.at(0) + "/second";
    }

    qDebug() << "Picture count = " << list.size() << list.at(0);
    MainWindow *w = dApp->getMainWindow();
    w->getButG();
    w->allPicBtnClicked();

    AllPicView *allpicview = w->m_pAllPicView;
    ImageEngineApi::instance()->ImportImagesFromFileList(list, "", allpicview, true);
    allpicview->update();
    QTest::qWait(2000);

    QTestEventList event;

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
