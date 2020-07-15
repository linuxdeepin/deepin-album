#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "mainwindow.h"

#include <QTestEventList>

TEST(MainWindow, BtnGroupClick)
{
    MainWindow *w = dApp->getMainWindow();
    w->hide();
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->test_getButG()->button(1));
    event.simulate(w->test_getButG()->button(2));
    event.clear();

}
