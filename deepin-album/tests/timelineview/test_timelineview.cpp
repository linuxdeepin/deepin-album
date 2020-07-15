#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "mainwindow.h"
#include "timelineview.h"

#include "utils/baseutils.h"

TEST(TimeLineView, T1)
{
    TimeLineView *a = static_cast<TimeLineView *>(dApp->getMainWindow()->m_pTimeLineView);
    if (nullptr ==  a) {
        return;
    }
}
