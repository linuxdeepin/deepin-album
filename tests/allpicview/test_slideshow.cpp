#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "mainwindow.h"
#include "allpicview.h"
#include "../test_qtestDefine.h"

#include <QTestEventList>

TEST(allpicview, test_beginSlideShow)
{
    qDebug() << "allpicview test_beginSlideShow count = " << count_testDefine++;
    QTest::qWait(500);
    QString testPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + "test";
    MainWindow *w = dApp->getMainWindow();

    AllPicView *a = w->m_pAllPicView;
//    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "", a, false);

//    QTest::qWait(500);
    QStringList testPathlist = ImageEngineApi::instance()->get_AllImagePath();
    if (!testPathlist.isEmpty()) {
        qDebug() << "test ImageView load Success ";
        a->getThumbnailListView()->menuOpenImage(testPathlist.first(), testPathlist, true, true);
    }
    QTest::qWait(25000);
    emit w->m_slidePanel->slideshowbottombar->showCancel();
    QTest::qWait(500);
}
