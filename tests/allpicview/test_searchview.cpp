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
    SlideShowButton *btn = new SlideShowButton;
    QPoint point = btn->pos();
    int width = btn->width();
    QTestEventList e;
    e.addMouseMove(point, 10);
    e.addMouseMove(point + QPoint(1, 1), 10);
    e.addMousePress(Qt::MouseButton::LeftButton, Qt::NoModifier, point + QPoint(1, 1), 10);
    e.addMouseRelease(Qt::MouseButton::LeftButton, Qt::NoModifier, point + QPoint(1, 1), 10);
    e.addMouseMove(point + QPoint(width + 5, 0), 10);
    e.simulate(btn);
    e.clear();
    btn->deleteLater();
}

TEST(searchview, search)
{
    TEST_CASE_NAME("search")
    MainWindow *w = dApp->getMainWindow();
    w->showEvent(nullptr);
    SearchView *s = w->m_pSearchView;
    s->onSlideShowBtnClicked();
}
