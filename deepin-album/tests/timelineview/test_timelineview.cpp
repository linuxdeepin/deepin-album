#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "mainwindow.h"
#include "timelineview.h"

#include "utils/baseutils.h"

#include <QTestEventList>

TEST(TimeLineView, T1)
{
//    QThreadPool::globalInstance()->waitForDone();
    MainWindow *w = dApp->getMainWindow();
    w->showEvent(nullptr);
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(1));
    event.clear();
    QTime time;
    time.start();
    while (time.elapsed() < 2000)
        dApp->processEvents();
    TimeLineView *t = w->m_pTimeLineView;


    dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Light);
    time.restart();
    while (time.elapsed() < 1000)
        dApp->processEvents();
    dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Dark);
    time.restart();
    while (time.elapsed() < 1000)
        dApp->processEvents();

    ASSERT_TRUE(t->getIBaseHeight());
}
