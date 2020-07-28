#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "mainwindow.h"
#include "timelineview.h"

#include "utils/baseutils.h"

#include <QTestEventList>

TEST(TimeLineView, T1)
{
    QThreadPool::globalInstance()->waitForDone();
    MainWindow *w = dApp->getMainWindow();
    w->showEvent(nullptr);
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(1));
    event.clear();
    TimeLineView *t = w->m_pTimeLineView;
    ASSERT_TRUE(t->getIBaseHeight());
}
