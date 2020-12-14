#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "mainwindow.h"
#include "allpicview.h"
#include "searchview.h"
#include "../test_qtestDefine.h"

#include <QTestEventList>

TEST(searchview, test_ini)
{
    qDebug() << "searchview test_ini count = " << count_testDefine++;
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
