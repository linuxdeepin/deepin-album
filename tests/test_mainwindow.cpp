#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "mainwindow.h"
#include "test_qtestDefine.h"

#include <QTestEventList>

TEST(MainWindow, BtnGroupClick)
{
    qDebug() << "MainWindow BtnGroupClick count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    w->onImprotBtnClicked();
    w->onLoadingFinished();
    w->getButG();
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(1));
    event.simulate(w->getButG()->button(2));
    event.clear();
    QTest::qWait(500);
}
TEST(MainWindow, createalbum)
{
    MainWindow *w = dApp->getMainWindow();
    QStringList list;
    list << "test";
    w->showCreateDialog(list);
}

//TEST(MainWindow, destory)
//{
//    MainWindow *w = dApp->getMainWindow();
//    delete w;
//    w = nullptr;
//}
