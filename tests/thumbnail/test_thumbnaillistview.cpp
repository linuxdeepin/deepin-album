// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
#include "thumbnaillistview.h"
#include "ac-desktop-define.h"
#include "timelinedatewidget.h"

#include <stub-tool/cpp-stub/stub.h>
#include <stub-tool/stub-ext/stubext.h>

TEST(ThumbnailListView, initThumbnailListView)
{
    TEST_CASE_NAME("initThumbnailListView")
    MainWindow *w = dApp->getMainWindow();
    w->timeLineBtnClicked();
    w->allPicBtnClicked();
    // w->m_pAllPicView->m_pThumbnailListView->
    QStringList paths;
    if (DBManager::instance()->getAllPaths().length() > 0)
        paths << DBManager::instance()->getAllPaths().first();
    else
        paths << testPath_test + "/2e5y8y.jpg";
    QStringList files;
    files << paths.first();
    bool needcache = true;
    bool needcheck = true;;
//    w->m_pAllPicView->m_pThumbnailListView->loadFilesFromLocal(files, needcache, needcheck);
    QTest::qWait(200);
    DBImgInfoList infolist;
    DBImgInfo temp;
    temp.filePath = testPath_test + "/2k9o1m.png";
    if (DBManager::instance()->getAllInfos().size() > 0)
        infolist = DBManager::instance()->getAllInfos();
    else
        infolist << temp;
    DBImgInfoList info;
    info << infolist.first();
    w->m_pAllPicView->m_pThumbnailListView->insertThumbnailByImgInfos(info);
    QTest::qWait(200);
    //todo
//    w->m_pAllPicView->m_pThumbnailListView->isLoading();
//    w->m_pAllPicView->m_pThumbnailListView->isAllPicSeleted();
    w->m_pAllPicView->m_pThumbnailListView->getDagItemPath();
//    w->m_pAllPicView->m_pThumbnailListView->getSelectedIndexes();
    QPoint point(10, 10);
    w->m_pAllPicView->m_pThumbnailListView->getRow(point);
    int row = 1;
    int start = 1;
    int end = 2;
    //todo
//    w->m_pAllPicView->m_pThumbnailListView->selectRear(row);
//    w->m_pAllPicView->m_pThumbnailListView->selectFront(row);
//    w->m_pAllPicView->m_pThumbnailListView->selectExtent(start, end);
//    w->m_pAllPicView->m_pThumbnailListView->resizeHand();
    w->m_pAllPicView->m_pThumbnailListView->onShowMenu(point);
    QList<int> ilist;
    ilist << 0 << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 10;
    foreach (auto i, ilist) {
        w->m_pAllPicView->m_pThumbnailListView->onPixMapScale(i);
    }

    w->m_pAllPicView->m_pThumbnailListView->getListViewHeight();
    w->m_pAllPicView->m_pThumbnailListView->initMenuAction();
    w->m_pAllPicView->m_pThumbnailListView->createAlbumMenu();
//    w->m_pAllPicView->m_pThumbnailListView->updateThumbnaillistview();
    w->m_pAllPicView->m_pThumbnailListView->startDrag(Qt::DropAction::CopyAction);

    QPoint pointbtn = w->m_pAllPicView->m_pThumbnailListView->pos();
    QTestEventList event;
    event.addMousePress(Qt::LeftButton, Qt::NoModifier, pointbtn + QPoint(1, 1), 600);
    event.addMouseMove(pointbtn + QPoint(200, 200), 10);
    event.addMouseRelease(Qt::LeftButton, Qt::NoModifier, pointbtn + QPoint(200, 200), 600);
    event.simulate(w->m_pAllPicView->m_pThumbnailListView);
    event.clear();
    QTest::qWait(500);
}

TEST(ThumbnailListView, createNewAlbumFromDialog1)
{
    TEST_CASE_NAME("createNewAlbumFromDialog1")
    MainWindow *w = dApp->getMainWindow();
    w->albumBtnClicked();

    QList<QAction *> actions = w->actions();

    AlbumImageButton *btn = w->m_pAlbumview->m_pLeftListView->m_pAddListBtn;
    QPoint pointbtn = btn->pos();
    QTestEventList event;
    event.addMouseMove(pointbtn + QPoint(1, 1), 10);
    event.addMouseMove(pointbtn + QPoint(2, 2), 10);
    event.addMouseClick(Qt::LeftButton, Qt::NoModifier);
    event.addMousePress(Qt::LeftButton, Qt::NoModifier);
    event.addMouseMove(pointbtn + QPoint(200, 200), 10);
    event.addMouseRelease(Qt::LeftButton, Qt::NoModifier, pointbtn + QPoint(200, 200), 10);
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
    emit dApp->signalM->sigCreateNewAlbumFromDialog("test-album1", 999); //UID乱写的
}

TEST(TimeLineDateWidget, functions)
{
    TEST_CASE_NAME("TimeLineDateWidget_functions")

    auto type = DGuiApplicationHelper::instance()->themeType();
    DGuiApplicationHelper::instance()->setThemeType(DGuiApplicationHelper::DarkType);

    auto w = new TimeLineDateWidget(nullptr, "123", "321");
    w->onGetBtnStatus();

    w->deleteLater();

    DGuiApplicationHelper::instance()->setThemeType(type);
    QTest::qWait(500);
}

TEST(importTimeLineDateWidget, functions)
{
    TEST_CASE_NAME("importTimeLineDateWidgett_functions")

    auto type = DGuiApplicationHelper::instance()->themeType();
    DGuiApplicationHelper::instance()->setThemeType(DGuiApplicationHelper::DarkType);

    auto w1 = new importTimeLineDateWidget(nullptr, "123", "321");
    w1->onChooseBtnCliked();
    QTest::qWait(200);
    w1->onChooseBtnCliked();
    w1->onChangeChooseBtnVisible(true);
    w1->onGetBtnStatus();

    DGuiApplicationHelper::instance()->setThemeType(DGuiApplicationHelper::LightType);
    QTest::qWait(500);

    auto w2 = new importTimeLineDateWidget(nullptr, "321", "123");
    w2->onTimeLinePicSelectAll(true);
    w2->onTimeLinePicSelectAll(false);

    DGuiApplicationHelper::instance()->setThemeType(DGuiApplicationHelper::DarkType);
    QTest::qWait(500);

    w1->deleteLater();
    w2->deleteLater();

    DGuiApplicationHelper::instance()->setThemeType(type);
    QTest::qWait(500);
}
