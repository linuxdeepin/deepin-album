#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <QTestEventList>
#include <QString>

#define private public
#define protected public

#include "leftlistview.h"
#include "application.h"
#include "mainwindow.h"
#include "albumview.h"
#include "baseutils.h"
#include "dbmanager.h"
#include "albumcreatedialog.h"
#include "../test_qtestDefine.h"
#include <stub-tool/cpp-stub/stub.h>
#include <stub-tool/stub-ext/stubext.h>

TEST(ThumbnailListView, initThumbnailListView)
{
    qDebug() << "ThumbnailListView initThumbnailListView count" << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->timeLineBtnClicked();
    w->allPicBtnClicked();
    // w->m_pAllPicView->m_pThumbnailListView->
    QStringList paths = DBManager::instance()->getAllPaths();
    QStringList files;
    files << paths.first();
    bool needcache = true;
    bool needcheck = true;;
    w->m_pAllPicView->m_pThumbnailListView->loadFilesFromLocal(files, needcache, needcheck);
    QTest::qWait(200);
    DBImgInfoList infolist = DBManager::instance()->getAllInfos();
    DBImgInfoList info;
    info << infolist.first();
    w->m_pAllPicView->m_pThumbnailListView->loadFilesFromLocal(info, needcache, needcheck);
    QTest::qWait(200);
    w->m_pAllPicView->m_pThumbnailListView->isLoading();
    w->m_pAllPicView->m_pThumbnailListView->isAllPicSeleted();
    w->m_pAllPicView->m_pThumbnailListView->checkResizeNum();
    w->m_pAllPicView->m_pThumbnailListView->getDagItemPath();
    w->m_pAllPicView->m_pThumbnailListView->getSelectedIndexes();
    QPoint point(10,10);
    w->m_pAllPicView->m_pThumbnailListView->getRow(point);
    int row = 1;
    int start = 1;
    int end = 2;
    w->m_pAllPicView->m_pThumbnailListView->selectRear(row);
    w->m_pAllPicView->m_pThumbnailListView->selectFront(row);
    w->m_pAllPicView->m_pThumbnailListView->selectExtent(start, end);
    w->m_pAllPicView->m_pThumbnailListView->resizeHand();
    w->m_pAllPicView->m_pThumbnailListView->onShowMenu(point);
    QList<int> ilist;
    ilist << 0 << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 10;
    foreach (auto i, ilist) {
        w->m_pAllPicView->m_pThumbnailListView->onPixMapScale(i);
    }

    w->m_pAllPicView->m_pThumbnailListView->getListViewHeight();
    w->m_pAllPicView->m_pThumbnailListView->initMenuAction();
    w->m_pAllPicView->m_pThumbnailListView->createAlbumMenu();
    w->m_pAllPicView->m_pThumbnailListView->getDamagedPixmap();
    w->m_pAllPicView->m_pThumbnailListView->updateThumbnaillistview();
    //
}

TEST(ThumbnailListView, createNewAlbumFromDialog1)
{
    qDebug() << "ThumbnailListView createNewAlbumFromDialog1 count" << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->albumBtnClicked();

    QList<QAction *> actions = w->actions();

    AlbumImageButton *btn = w->m_pAlbumview->m_pLeftListView->m_pAddListBtn;
    QPoint pointbtn = btn->pos();
    QTestEventList event;
    event.addMouseMove(pointbtn + QPoint(1, 1), 10);
    event.addMouseMove(pointbtn + QPoint(2, 2), 10);
    event.addMouseClick(Qt::LeftButton, Qt::NoModifier);
    event.addMouseMove(pointbtn + QPoint(200, 200), 10);
    event.simulate(btn);
    event.clear();
    QTest::qWait(500);
    QList<QWidget *> widgets = w->findChildren<QWidget *>("");
    foreach (auto widget, widgets) {
        if (!strcmp(widget->metaObject()->className(), "AlbumCreateDialog")) {
            AlbumCreateDialog *temp = static_cast<AlbumCreateDialog *>(widget);
            QPoint pos(10, 10);
            event.addMouseMove(pos);
            event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pos);
            event.simulate(temp->getButton(1));
            event.clear();
            break;
        }
    }
    QTest::qWait(200);
    emit dApp->signalM->sigCreateNewAlbumFromDialog("test-album");
}
