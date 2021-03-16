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

    emit impTimeline->pSuspensionChose->clicked();
    QTest::qWait(500);

    impTimeline->clearAndStop();
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

    ImportTimeLineView *impTimeline = w->m_pAlbumview->m_pImpTimeLineView;
    impTimeline->on_KeyEvent(Qt::Key_PageDown);
    QTest::qWait(100);
    impTimeline->on_KeyEvent(Qt::Key_PageUp);
    QTest::qWait(500);
}

TEST(ImportTimeLineView, resizeHand_test)
{
    TEST_CASE_NAME("resizeHand_test")
    MainWindow *w = dApp->getMainWindow();
    w->albumBtnClicked();
    QTest::qWait(500);
    ImportTimeLineView *impTimeline = w->m_pAlbumview->m_pImpTimeLineView;
    impTimeline->resizeHand();
    QTest::qWait(500);
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

    ImportTimeLineView *impTimeline = w->m_pAlbumview->m_pImpTimeLineView;
    if (impTimeline->m_allThumbnailListView.count() > 0) {
        ThumbnailListView *tempThumbnailListView =  impTimeline->m_allThumbnailListView.first();
        ViewPanel *viewPanel = nullptr;
        QList<QWidget *> widgets = w->findChildren<QWidget *>();
        foreach (auto widget, widgets) {
            if (!strcmp(widget->metaObject()->className(), "ViewPanel")) {
                viewPanel = dynamic_cast<ViewPanel *>(widget);
            }
        }

        QStringList tempPaths = tempThumbnailListView->getAllPaths();
        if (tempPaths.count() > 4 && viewPanel != nullptr) {
            emit tempThumbnailListView->openImage(0);
            QTest::qWait(500);
            viewPanel->onESCKeyActivated();

            QString temppath = tempPaths.at(1);
            emit tempThumbnailListView->menuOpenImage(temppath, tempPaths, false, false);
            QTest::qWait(500);
            viewPanel->onESCKeyActivated();

            QMouseEvent event(QEvent::MouseButtonPress, QPointF(60, 60), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            emit tempThumbnailListView->sigMousePress(&event);
            QTest::qWait(500);

            QMouseEvent shiftevent(QEvent::MouseButtonPress, QPointF(60, 60), Qt::LeftButton, Qt::LeftButton, Qt::ShiftModifier);
            emit tempThumbnailListView->sigShiftMousePress(&shiftevent);
            QTest::qWait(500);

            QMouseEvent ctrlevent(QEvent::MouseButtonPress, QPointF(60, 60), Qt::LeftButton, Qt::LeftButton, Qt::ControlModifier);
            emit tempThumbnailListView->sigShiftMousePress(&ctrlevent);
            QTest::qWait(500);

            QStringList selectPaths = tempThumbnailListView->getAllPaths();
            if (selectPaths.count() > 0) {
                QStringList paths;
                paths.append(selectPaths.first());
                emit tempThumbnailListView->sigGetSelectedPaths(&paths);
                QTest::qWait(500);
            }

            emit tempThumbnailListView->sigSelectAll();
            QTest::qWait(500);

            emit tempThumbnailListView->sigMouseMove();
            QTest::qWait(500);

            emit tempThumbnailListView->sigMouseRelease();
            QTest::qWait(500);

//            QPoint point(10,10);
//            emit tempThumbnailListView->customContextMenuRequested(point);
//            QTest::qWait(500);

            emit tempThumbnailListView->needResizeLabel();
            QTest::qWait(500);
        }
    }
    QTest::qWait(500);
}
