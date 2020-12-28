#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "../test_qtestDefine.h"
#include <QTestEventList>

#define private public
#define protected public

#include "application.h"
#include "mainwindow.h"
#include "allpicview.h"
#include "slideshowpanel.h"



//TEST(slideshowpanel, test_beginSlideShow)
//{
//    qDebug() << "allpicview test_beginSlideShow count = " << count_testDefine++;
//    QTest::qWait(500);
//    QString testPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + "test";
//    MainWindow *w = dApp->getMainWindow();

//    AllPicView *a = w->m_pAllPicView;
////    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "", a, false);

////    QTest::qWait(500);
//    QStringList testPathlist = ImageEngineApi::instance()->get_AllImagePath();
//    if (!testPathlist.isEmpty()) {
//        qDebug() << "test ImageView load Success ";
//        a->getThumbnailListView()->menuOpenImage(testPathlist.first(), testPathlist, true, true);
//    }
//    QTest::qWait(25000);

//    int h = w->m_slidePanel->rect().height();
//    QPoint pos(10, h);
//    QTestEventList event;
//    event.addMouseMove(pos);
//    event.simulate(w->m_slidePanel);
//    event.clear();
//    QTest::qWait(500);

//    w->m_slidePanel->slideshowbottombar->onPreButtonClicked();
//    QTest::qWait(100);
//    w->m_slidePanel->slideshowbottombar->onNextButtonClicked();
//    QTest::qWait(100);
//    w->m_slidePanel->slideshowbottombar->onPlaypauseButtonClicked();
//    QTest::qWait(100);
//    w->m_slidePanel->slideshowbottombar->onUpdatePauseButton();
//    QTest::qWait(100);
//    w->m_slidePanel->slideshowbottombar->onCancelButtonClicked();

//    if (!testPathlist.isEmpty()) {
//        qDebug() << "test ImageView load Success ";
//        a->getThumbnailListView()->menuOpenImage(testPathlist.first(), testPathlist, true, true);
//    }
//    QTest::qWait(100);
//    w->m_slidePanel->onShowPause();
//    QTest::qWait(100);
//    w->m_slidePanel->onShowContinue();
//    QTest::qWait(100);
//    w->m_slidePanel->onShowPrevious();
//    QTest::qWait(100);
//    w->m_slidePanel->onShowNext();
//    QTest::qWait(100);
//    w->m_slidePanel->onCustomContextMenuRequested();
//    QTest::qWait(100);

//    w->m_slidePanel->onESCKeyStopSlide();
//    if (!testPathlist.isEmpty()) {
//        qDebug() << "test ImageView load Success ";
//        a->getThumbnailListView()->menuOpenImage(testPathlist.first(), testPathlist, true, true);
//    }
//    QList<QAction *> acts =  w->m_slidePanel->actions();
//    w->m_slidePanel->onMenuItemClicked(acts.at(0));
//    QTest::qWait(500);
//    w->m_slidePanel->onMenuItemClicked(acts.at(1));
//    QTest::qWait(500);
//}
