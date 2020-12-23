#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <QMap>
#include <DFileDialog>
#include <QTestEventList>
#include <QObject>
#include <QDialog>
#include <QStringList>
#include <DSearchEdit>

#define private public
#define protected public

#include "mainwindow.h"
#include "importtimelineview.h"
#include "albumcreatedialog.h"
#include "test_qtestDefine.h"
#include "imginfodialog.h"
#include "imageengine/imageengineapi.h"
#include "viewpanel.h"
#include <stub-tool/cpp-stub/stub.h>
#include <stub-tool/stub-ext/stubext.h>


TEST(ImportTimeLineView, getIBaseHeight_test)
{
    qDebug() << "ImportTimeLineView getIBaseHeight_test count" << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->albumBtnClicked();
    QTest::qWait(500);

    ImportTimeLineView *impTimeline = w->m_pAlbumview->m_pImpTimeLineWidget;
    int a = 0;
    stub_ext::StubExt stu;
    stu.set_lamda(ADDR(DSlider, value), [&a]() {
        return a++;
    });
    for (int i = 0; i < 10; i++) {
        impTimeline->getIBaseHeight();
    }
    QTest::qWait(500);
}


TEST(ImportTimeLineView, test_func)
{
    qDebug() << "ImportTimeLineView test_func count" << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->albumBtnClicked();
    QTest::qWait(500);

    ImportTimeLineView *impTimeline = w->m_pAlbumview->m_pImpTimeLineWidget;
    impTimeline->updateChoseText();

    emit impTimeline->pSuspensionChose->clicked();
    QTest::qWait(500);

    impTimeline->clearAndStop();
    w->allPicBtnClicked();
    w->albumBtnClicked();
    QTest::qWait(500);
}

TEST(ImportTimeLineView, on_KeyEvent_test)
{
    qDebug() << "ImportTimeLineView on_KeyEvent_test count" << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->albumBtnClicked();
    QTest::qWait(500);

    ImportTimeLineView *impTimeline = w->m_pAlbumview->m_pImpTimeLineWidget;
    impTimeline->on_KeyEvent(Qt::Key_PageDown);
    QTest::qWait(100);
    impTimeline->on_KeyEvent(Qt::Key_PageUp);
    QTest::qWait(500);
}

TEST(ImportTimeLineView, resizeHand_test)

{
    qDebug() << "ImportTimeLineView getIBaseHeight_test count" << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->albumBtnClicked();
    QTest::qWait(500);
    ImportTimeLineView *impTimeline = w->m_pAlbumview->m_pImpTimeLineWidget;
    impTimeline->resizeHand();
    QTest::qWait(500);
}

TEST(ImportTimeLineView, thumbnaillistViewSlot_test)
{
    qDebug() << "ImportTimeLineView getIBaseHeight_test count" << count_testDefine++;
    QTest::qWait(500);
    MainWindow *w = dApp->getMainWindow();
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(2));
    event.clear();
    QTest::qWait(300);
    AlbumView *a = w->m_pAlbumview;
    event.addMousePress(Qt::MouseButton::LeftButton, Qt::NoModifier, QPoint(10, 10));
    event.addMouseRelease(Qt::MouseButton::LeftButton, Qt::NoModifier, QPoint(10, 10));
    event.simulate(a->m_pLeftListView->m_pPhotoLibListView->viewport());
    QTest::qWait(1000);

    ImportTimeLineView *impTimeline = w->m_pAlbumview->m_pImpTimeLineWidget;
    if (impTimeline->m_allThumbnailListView.count() >0) {
        ThumbnailListView* tempThumbnailListView =  impTimeline->m_allThumbnailListView.first();
        ViewPanel *viewPanel = nullptr;
        QList<QWidget*> widgets = w->findChildren<QWidget *>();
        foreach (auto widget, widgets) {
            if (!strcmp(widget->metaObject()->className(),"ViewPanel")) {
                viewPanel = dynamic_cast<ViewPanel*>(widget);
            }
        }

        QStringList tempPaths = tempThumbnailListView->getAllPaths();
        if (tempPaths.count() > 4 && viewPanel != nullptr) {
            emit tempThumbnailListView->openImage(0);
            QTest::qWait(500);
            viewPanel->onESCKeyActivated();

            QString temppath = tempPaths.at(1);
            emit tempThumbnailListView->menuOpenImage(temppath,tempPaths,false,false);
            QTest::qWait(500);
            viewPanel->onESCKeyActivated();

            QMouseEvent event(QEvent::MouseButtonPress, QPointF(60,60), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            emit tempThumbnailListView->sigMousePress(&event);
            QTest::qWait(500);

            QMouseEvent shiftevent(QEvent::MouseButtonPress, QPointF(60,60), Qt::LeftButton, Qt::LeftButton, Qt::ShiftModifier);
            emit tempThumbnailListView->sigShiftMousePress(&shiftevent);
            QTest::qWait(500);

            QMouseEvent ctrlevent(QEvent::MouseButtonPress, QPointF(60,60), Qt::LeftButton, Qt::LeftButton, Qt::ControlModifier);
            emit tempThumbnailListView->sigShiftMousePress(&ctrlevent);
            QTest::qWait(500);

            QStringList selectPaths = tempThumbnailListView->getAllPaths();
            if (selectPaths.count() > 0) {
                QStringList paths;
                paths.append(selectPaths.first());
                emit tempThumbnailListView->sigGetSelectedPaths(&paths);
                QTest::qWait(500);
            }

            emit tempThumbnailListView->sigSelectAll();
            QTest::qWait(500);

            emit tempThumbnailListView->sigMouseMove();
            QTest::qWait(500);

            emit tempThumbnailListView->sigMouseRelease();
            QTest::qWait(500);

//            QPoint point(10,10);
//            emit tempThumbnailListView->customContextMenuRequested(point);
//            QTest::qWait(500);

            emit tempThumbnailListView->needResizeLabel();
            QTest::qWait(500);
        }
    }
    QTest::qWait(500);
}
