// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include <QTestEventList>

#include "application.h"
#include "mainwindow.h"
#include "allpicview.h"
#include "searchview.h"
#include "../test_qtestDefine.h"
#include "ac-desktop-define.h"


TEST(searchview, test_ini)
{
    TEST_CASE_NAME("test_ini")
    QTest::qWait(200);
    MainWindow *w = dApp->getMainWindow();
    w->showEvent(nullptr);
    QTestEventList event;
    QPoint p = w->getButG()->button(0)->pos();
    event.addMouseMove(p);
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.addKeyClick(Qt::Key_A);
    event.addMouseMove(p - QPoint(10, 10));
    event.simulate(w->getButG()->button(0));
    event.clear();

    ASSERT_TRUE(w->m_pSearchView);
}

TEST(SlideShowButton, slidebtn)
{
    TEST_CASE_NAME("slidebtn")
//    SlideShowButton *btn = new SlideShowButton;
//    QPoint point = btn->pos();
//    int width = btn->width();
//    QTestEventList e;
//    e.addMouseMove(point, 10);
//    e.addMouseMove(point + QPoint(1, 1), 10);
//    e.addMousePress(Qt::MouseButton::LeftButton, Qt::NoModifier, point + QPoint(1, 1), 10);
//    e.addMouseRelease(Qt::MouseButton::LeftButton, Qt::NoModifier, point + QPoint(1, 1), 10);
//    e.addMouseMove(point + QPoint(width + 5, 0), 10);
//    e.simulate(btn);
//    e.clear();
//    btn->deleteLater();
}

TEST(searchview, search)
{
    TEST_CASE_NAME("search")
    MainWindow *w = dApp->getMainWindow();
    w->showEvent(nullptr);
    SearchView *s = w->m_pSearchView;
    s->onSlideShowBtnClicked();

    w->m_pSearchEdit->setText(".jpg");

    QTestEventList e;
    e.addKeyClick(Qt::Key_Enter);
    e.addDelay(500);
    e.simulate(w);

    s->onOpenImage(1, testPath_Pictures + "2e5y8y.jpg", false);
    QTest::qWait(500);
    w->onHideImageView();
    QTest::qWait(500);

    s->onSlideShow(testPath_Pictures + "2e5y8y.jpg");
    QTest::qWait(500);
    w->onHideFromFullScreen();
    QTest::qWait(500);
}
