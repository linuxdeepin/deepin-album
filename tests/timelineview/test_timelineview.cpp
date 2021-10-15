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

#include <QTestEventList>
#include <QScrollBar>

#include "application.h"
#include "mainwindow.h"
#include "timelineview.h"
#include "../test_qtestDefine.h"
#include "ac-desktop-define.h"
#include "utils/baseutils.h"
#include "viewerthememanager.h"


TEST(TimeLineView, T1)
{
    TEST_CASE_NAME("T1")
    MainWindow *w = dApp->getMainWindow();
    w->showEvent(nullptr);
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(1));
    event.clear();
    QTest::qWait(500);
    TimeLineView *t = w->m_pTimeLineView;
    t->m_pStatusBar->m_pSlider->setValue(1);
    t->m_pStatusBar->m_pSlider->setValue(2);
    t->m_pStatusBar->m_pSlider->setValue(3);
    t->m_pStatusBar->m_pSlider->setValue(4);
    t->m_pStatusBar->m_pSlider->setValue(5);
    t->m_pStatusBar->m_pSlider->setValue(6);
    t->m_pStatusBar->m_pSlider->setValue(7);
    t->m_pStatusBar->m_pSlider->setValue(8);
    t->m_pStatusBar->m_pSlider->setValue(9);
    t->m_pStatusBar->m_pSlider->setValue(10);
    t->m_pStatusBar->m_pSlider->setValue(4);

    dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Light);
    QTest::qWait(500);
    dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Dark);
    QStringList list;
    list << testPath_test + "2k9o1m.png";
    t->updataLayout(list);
    t->on_DCommandLinkButton();

    QString jpgItemPath = testPath_test + "/2k9o1m.png";
    QString text = "xxxxxxxxxxxxxx";
    QIcon icon = QIcon(":/resources/images/other/deepin-album.svg");
    QIcon icon_hover = QIcon(":/resources/images/other/deepin-album.svg");
    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << text << icon << icon_hover;
    QMimeData mimedata;
    mimedata.setData(QStringLiteral("TestListView/text-icon-icon_hover"), itemData);
    QList<QUrl> li;
    li.append(QUrl::fromLocalFile(jpgItemPath));
    mimedata.setUrls(li);

    QPoint pos1 = t->pos();
    QDropEvent ed(pos1, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
    dApp->getDAppNew()->sendEvent(t, &ed);
    QTest::qWait(100);

    QTestEventList e;
    e.addMousePress(Qt::MouseButton::LeftButton, Qt::NoModifier, pos1, 10);
    e.simulate(t);
    e.clear();

//    ASSERT_TRUE(t->getIBaseHeight());
}

TEST(TimeLineView, dragPhotoToAnAlbum)
{
    TEST_CASE_NAME("dragPhotoToAnAlbum")
    MainWindow *w = dApp->getMainWindow();

    w->timeLineBtnClicked();
    QTest::qWait(100);
    TimeLineView *a = w->m_pTimeLineView;
    QList<QWidget *> widgets = a->findChildren<QWidget *>("");
    for (int index = 0; index < widgets.count(); index++) {
        if (!strcmp(widgets.at(index)->metaObject()->className(), ("TimelineList"))) {
        }
        if (!strcmp(widgets.at(index)->metaObject()->className(), ("ThumbnailListView"))) {
            QString jpgItemPath = testPath_test + "/2k9o1m.png";
            QString text = "xxxxxxxxxxxxxx";
            QIcon icon = QIcon(":/resources/images/other/deepin-album.svg");
            QIcon icon_hover = QIcon(":/resources/images/other/deepin-album.svg");
            QByteArray itemData;
            QDataStream dataStream(&itemData, QIODevice::WriteOnly);
            dataStream << text << icon << icon_hover;
            QMimeData mimedata;
            mimedata.setData(QStringLiteral("TestListView/text-icon-icon_hover"), itemData);
            QList<QUrl> li;
            li.append(QUrl::fromLocalFile(jpgItemPath));
            mimedata.setUrls(li);
            QTest::qWait(200);

            const QPoint pos1 = a->rect().center();
            QDragEnterEvent eEnter(pos1, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
            dApp->getDAppNew()->sendEvent(a, &eEnter);
            QTest::qWait(200);

            QDragMoveEvent eMove(pos1, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
            dApp->getDAppNew()->sendEvent(a, &eMove);
            QTest::qWait(200);

            QDropEvent e(pos1, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
//            dApp->getDAppNew()->sendEvent(a, &e);
            QTest::qWait(200);

            dApp->getDAppNew()->sendEvent(a, &eEnter);
            QTest::qWait(200);

            QDragLeaveEvent eLeave;
            dApp->getDAppNew()->sendEvent(a, &eLeave);
            QTest::qWait(500);
            break;
        }
    }
}
TEST(TimeLineView, SelectTimeLinesBtn)
{
    TEST_CASE_NAME("SelectTimeLinesBtn")
    MainWindow *w = dApp->getMainWindow();
    QTestEventList event;
    w->timeLineBtnClicked();
    QTest::qWait(500);
    TimeLineView *t = w->m_pTimeLineView;
    QList<QWidget *> widgets = t->findChildren<QWidget *>("");
    foreach (auto widget, widgets) {
        if (widget->objectName() == "TimeLineChooseButton") {
            DCommandLinkButton *temp = static_cast<DCommandLinkButton *>(widget);
            QPoint pos(10, 10);
            event.addMouseMove(pos);
            event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pos);
            event.simulate(temp);
            event.clear();
            QTest::qWait(500);

            event.addMouseMove(pos);
            event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pos);
            event.simulate(temp);
            event.clear();
            QTest::qWait(500);
            break;
        }
    }
}

TEST(TimeLineView, selectBtn)
{
    TEST_CASE_NAME("selectBtn")
    MainWindow *w = dApp->getMainWindow();
    w->timeLineBtnClicked();
    QTest::qWait(500);

    TimeLineView *t = w->m_pTimeLineView;
    QList<QWidget *> widgets = t->findChildren<QWidget *>("");
    for (int i = 0; i < widgets.count(); i++) {
        if (!strcmp(widgets.at(i)->metaObject()->className(), ("Dtk::Widget::DCommandLinkButton"))) {
            DCommandLinkButton *pDCmdBtnSelect = dynamic_cast<DCommandLinkButton *>(widgets.at(i));
            if (pDCmdBtnSelect->text() == QObject::tr("Select")) {
                QTestEventList event;
                event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, QPoint(10, 10));
                event.simulate(widgets.at(i));
                event.clear();
                break;
            }
        }
    }
    QTest::qWait(500);
    for (int i = 0; i < widgets.count(); i++) {
        if (!strcmp(widgets.at(i)->metaObject()->className(), ("Dtk::Widget::DCommandLinkButton"))) {
            DCommandLinkButton *pDCmdBtnUnselect = dynamic_cast<DCommandLinkButton *>(widgets.at(i));
            if (pDCmdBtnUnselect->text() == QObject::tr("Unselect")) {
                QTestEventList event;
                event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, QPoint(10, 10));
                event.simulate(widgets.at(i));
                event.clear();
                break;
            }
        }
    }
    QTest::qWait(500);
}

TEST(TimeLineView, changeTheme)
{
    TEST_CASE_NAME("changeTheme")
    MainWindow *w = dApp->getMainWindow();
    w->timeLineBtnClicked();
    QTest::qWait(500);
    TimeLineView *t = w->m_pTimeLineView;
    t->setFocus();
    dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Dark);
    QTest::qWait(500);
}

TEST(TimeLineView, oneThumblistview_test)
{
    TEST_CASE_NAME("oneThumblistview_test")
    MainWindow *w = dApp->getMainWindow();
    w->showEvent(nullptr);
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(1));
    event.clear();
    QTest::qWait(500);


    TimeLineView *tlView = w->m_pTimeLineView;
    QList<QWidget *> widgets =  tlView->findChildren<QWidget *>();
    foreach (auto wgt, widgets) {
        if (!strcmp(wgt->metaObject()->className(), "ThumbnailListView")) {

            QPoint pos(60, 60);
            event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pos);
            event.simulate(wgt);
            event.clear();
            QTest::qWait(500);

            event.addMousePress(Qt::MouseButton::LeftButton, Qt::NoModifier, pos);
            event.simulate(wgt);
            event.clear();
            QTest::qWait(500);

            event.addMousePress(Qt::MouseButton::LeftButton, Qt::ControlModifier, pos);
            event.simulate(wgt);
            event.clear();
            QTest::qWait(500);

            event.addMousePress(Qt::MouseButton::LeftButton, Qt::ShiftModifier, pos);
            event.simulate(wgt);
            event.clear();
            QTest::qWait(500);

            event.addMousePress(Qt::MouseButton::LeftButton, Qt::NoModifier, pos);
            event.addMouseRelease(Qt::MouseButton::LeftButton, Qt::NoModifier, pos);
            event.simulate(wgt);
            event.clear();
            QTest::qWait(500);
        }
    }
}


