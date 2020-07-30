#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "mainwindow.h"
#include "allpicview.h"

#include <QTestEventList>

TEST(allpicview, test_beginSlideShow)
{
    QString testPath = "/home/djh/Pictures/test";
    MainWindow *w = dApp->getMainWindow();

    AllPicView *a = w->m_pAllPicView;
    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "", a, false);
    QTime t;
    t.start();
    while (t.elapsed() < 10000)
        dApp->processEvents();
    QStringList testPathlist = ImageEngineApi::instance()->get_AllImagePath();
    if (!testPathlist.isEmpty()) {
        qDebug() << "test ImageView load Success ";
        a->getThumbnailListView()->menuOpenImage(testPathlist.first(), testPathlist, true, true);
    }
    t.restart();
    while (t.elapsed() < 20000)
        dApp->processEvents();
}
