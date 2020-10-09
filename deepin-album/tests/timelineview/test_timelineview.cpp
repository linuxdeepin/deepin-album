#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "mainwindow.h"
#include "timelineview.h"
#include "../test_qtestDefine.h"

#include "utils/baseutils.h"

#include <QTestEventList>

TEST(TimeLineView, T1)
{
    if (!switch_on_test) {
        return;
    }
    qDebug() << "TimeLineView T1 count = " << count_testDefine++;
//    QThreadPool::globalInstance()->waitForDone();
    MainWindow *w = dApp->getMainWindow();
    w->showEvent(nullptr);
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(1));
    event.clear();
    QTest::qWait(500);
    TimeLineView *t = w->m_pTimeLineView;

    dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Light);
    QTest::qWait(500);
    dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Dark);
    QTest::qWait(500);

    ASSERT_TRUE(t->getIBaseHeight());
}
