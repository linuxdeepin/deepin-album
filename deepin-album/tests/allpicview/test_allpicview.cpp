#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "mainwindow.h"
#include "allpicview.h"

#include <QTestEventList>

TEST(allpicview, test_ini)
{
    QThreadPool::globalInstance()->waitForDone();
    MainWindow *w = dApp->getMainWindow();
    w->showEvent(nullptr);
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(0));
    event.clear();
    AllPicView *a = w->m_pAllPicView;
    for (int i = 0; i < 10; i++) {
        a->m_pStatusBar->m_pSlider->setValue(i);
    }
    a->m_pStatusBar->m_pSlider->setValue(1);
}

TEST(allpicview, test_open)
{
    QString testPath = "/home/djh/Pictures/test";
    MainWindow *w = dApp->getMainWindow();

    AllPicView *a = w->m_pAllPicView;
    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "", a, false);
    QTime t;
    t.start();
    while (t.elapsed() < 3000)
        dApp->processEvents();
    QStringList testPathlist = ImageEngineApi::instance()->get_AllImagePath();
    if (!testPathlist.isEmpty()) {
        qDebug() << "test ImageView Success ";
        emit a->getThumbnailListView()->menuOpenImage(testPathlist.first(), testPathlist, false);
        emit dApp->signalM->showImageInfo(testPathlist.first());
    }

    t.restart();
    while (t.elapsed() < 3000)
        dApp->processEvents();
    QTestEventList e;
    e.addKeyClick(Qt::Key_Escape);
    e.simulate(w);
}

TEST(allpicview, test_select)
{
    MainWindow *w = dApp->getMainWindow();
    AllPicView *a = w->m_pAllPicView;
    QTestEventList e;
    QPoint p = a->getThumbnailListView()->pos();
    p.setX(p.x() + 10);
    p.setY(p.y() + 10);
    e.addMouseMove(p);
    e.addKeyPress(Qt::LeftButton);
    e.addMouseMove(QPoint(p.x() + 400, p.y() + 200));
    e.clear();
    e.simulate(a->getThumbnailListView());
}
