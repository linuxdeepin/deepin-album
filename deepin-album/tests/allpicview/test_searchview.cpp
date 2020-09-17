#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "mainwindow.h"
#include "allpicview.h"
#include "searchview.h"

#include <QTestEventList>

TEST(searchview, test_ini)
{
    QTime time;
    time.start();
    while (time.elapsed() < 200)
        dApp->processEvents();
    MainWindow *w = dApp->getMainWindow();
    w->showEvent(nullptr);
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(0));
    event.clear();
    ASSERT_TRUE(w->m_pSearchView);
}
