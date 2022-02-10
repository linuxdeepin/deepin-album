/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     ZhangYong <zhangyong@uniontech.com>
 *
 * Maintainer: ZhangYong <ZhangYong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include <QTestEventList>

#define private public
#define protected public

#include "application.h"
#include "mainwindow.h"
#include "allpicview.h"
#include "imgdeletedialog.h"
#include "wallpapersetter.h"
#include "signalmanager.h"
#include "../test_qtestDefine.h"
#include "ac-desktop-define.h"
#include "testtoolkits.h"
#include "batchoperatewidget.h"
#include "imagedataservice.h"
#include "imageengineapi.h"

TEST(allpicview, test_ini)
{
    TEST_CASE_NAME("test_ini")
    MainWindow *w = dApp->getMainWindow();

    QPoint pos(20, 20);
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pos);
    event.simulate(w->getButG()->button(0));
    AllPicView *a = w->m_pAllPicView;

    int width = a->m_pStatusBar->m_pSlider->slider()->width();
    pos = QPoint(width / 10 * a->m_pStatusBar->m_pSlider->slider()->sliderPosition(), 10);
    event.addMousePress(Qt::LeftButton, Qt::NoModifier, pos);
    pos = pos - QPoint(200, 0);
    event.addMouseMove(pos);
    QTest::qWait(500);
    event.simulate(a->m_pStatusBar->m_pSlider->slider());
    pos = QPoint(width / 10 * a->m_pStatusBar->m_pSlider->slider()->sliderPosition(), 10);
    for (int i = 0; i < 5; i++) {
        event.addMousePress(Qt::LeftButton, Qt::NoModifier, pos);
        pos = pos + QPoint(20, 0);
        event.addMouseMove(pos);
        event.simulate(a->m_pStatusBar->m_pSlider->slider());
        event.clear();
        QTest::qWait(500);
    }
    pos = QPoint(width / 10 * a->m_pStatusBar->m_pSlider->slider()->sliderPosition(), 10);
    for (int i = 0; i < 5; i++) {
        event.addMousePress(Qt::LeftButton, Qt::NoModifier, pos);
        pos = pos - QPoint(20, 0);
        event.addMouseMove(pos);
        event.simulate(a->m_pStatusBar->m_pSlider->slider());
        event.clear();
        QTest::qWait(500);
    }
    event.addMousePress(Qt::LeftButton, Qt::NoModifier, pos);
    pos = pos - QPoint(20, 0);
    event.addMouseMove(pos);
    event.addMouseRelease(Qt::LeftButton, Qt::NoModifier, pos);
    event.simulate(a->m_pStatusBar->m_pSlider->slider());
    event.clear();
}

TEST(allpicview, resize)
{
    TEST_CASE_NAME("resize")
    MainWindow *w = dApp->getMainWindow();

    QTest::qWait(200);
    w->resize(w->size() + QSize(300, 300));
    QTest::qWait(200);
    QResizeEvent resize2(w->size() - QSize(100, 100), w->size());
    w->resize(w->size() - QSize(300, 300));
    QTest::qWait(200);
}

TEST(allpicview, dragevent)
{
    TEST_CASE_NAME("dragevent")
    QTest::qWait(200);
    MainWindow *w = dApp->getMainWindow();
    AllPicView *a = w->m_pAllPicView;
    w->show();

    // 打开保存绘制的 ddf
    QString jpgItemPath = testPath_test + "/2e5y8y.jpg";

    QString text = "xxxxxxxxxxxxxx";
    QIcon icon = QIcon(":/resources/images/other/deepin-album.svg");
    QIcon icon_hover = QIcon(":/resources/images/other/deepin-album.svg");
    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << text << icon << icon_hover;
//    QMimeData *mimeData = new QMimeData;
    QMimeData mimedata;
    mimedata.setData(QStringLiteral("TestListView/text-icon-icon_hover"), itemData);
    QList<QUrl> li;
    li.append(QUrl::fromLocalFile(jpgItemPath));
    mimedata.setUrls(li);

    const QPoint pos = a->getThumbnailListView()->viewport()->rect().center();
    QDragEnterEvent eEnter(pos, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
    dApp->getDAppNew()->sendEvent(a->getThumbnailListView()->viewport(), &eEnter);
    QTest::qWait(200);

    QDragMoveEvent eMove(pos + QPoint(100, 100), Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
    dApp->getDAppNew()->sendEvent(a->getThumbnailListView()->viewport(), &eMove);
    QTest::qWait(200);

    QDropEvent e(pos, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
    dApp->getDAppNew()->sendEvent(a->getThumbnailListView()->viewport(), &e);
    QTest::qWait(200);

    dApp->getDAppNew()->sendEvent(a->getThumbnailListView()->viewport(), &eEnter);
    QTest::qWait(200);

    QDragLeaveEvent eLeave;
    dApp->getDAppNew()->sendEvent(a->getThumbnailListView()->viewport(), &eLeave);
    QTest::qWait(2000);
}

TEST(allpicview, mousePress)
{
    TEST_CASE_NAME("mousePress")
    MainWindow *w = dApp->getMainWindow();

    QPoint pos(40, 60);
    AllPicView *a = w->m_pAllPicView;
    QTestEventList event;
    event.addMousePress(Qt::LeftButton, Qt::NoModifier, pos);
    event.addMouseMove(pos + QPoint(200, 200));
    event.addMouseRelease(Qt::LeftButton, Qt::NoModifier, pos + QPoint(200, 200));
    event.simulate(a->getThumbnailListView()->viewport());
    event.clear();
    QTest::qWait(500);
}

TEST(allpicview, test_select)
{
    TEST_CASE_NAME("test_select")
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

    dApp->signalM->sigShortcutKeyDelete();
    QTest::qWait(100);
//    a->getThumbnailListView()->sigLoad80ThumbnailsFinish();
    QTest::qWait(100);
    a->updateStackedWidget();
    a->restorePicNum();
    a->updatePicNum();
    dApp->signalM->imagesRemoved();
    QTest::qWait(100);
//    emit a->getThumbnailListView()->openImage(0, ImageEngineApi::instance()->get_AllImagePath().first(), false);
//    QTest::qWait(100);
}

TEST(allpicview, BatchOperateWidget)
{
    TEST_CASE_NAME("BatchOperateWidget")

    //定位到所有图片
    clickToAllPictureView();
    //定位到第一张图片位置
    QModelIndex index;
    for (int i = 0; i < dApp->getMainWindow()->m_pAllPicView->getThumbnailListView()->m_model->rowCount(); i++) {
        index = dApp->getMainWindow()->m_pAllPicView->getThumbnailListView()->m_model->index(i, 0);
        DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
        if (data.itemType == ItemType::ItemTypePic) {
            break;
        }
    }
    QRect picItem = dApp->getMainWindow()->m_pAllPicView->getThumbnailListView()->visualRect(index);
    //选中第一张图片
    QTest::qWait(200);
    QPoint p1(picItem.x() + 20, picItem.y() + 20);
    QTestEventList e;
    e.addMouseMove(p1);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(dApp->getMainWindow()->m_pAllPicView->m_pThumbnailListView->viewport());
    e.clear();
    QTest::qWait(200);

    BatchOperateWidget *batchOperateWidget = dApp->getMainWindow()->m_pAllPicView->findChild<BatchOperateWidget *>(All_Picture_BatchOperateWidget);
    if (batchOperateWidget) {
        batchOperateWidget->sltCollectSelect(true);
        QTest::qWait(200);
        batchOperateWidget->sltCollectSelect(true);
        QTest::qWait(200);
        batchOperateWidget->sltRightRotate(true);
        QTest::qWait(200);
        batchOperateWidget->sltLeftRotate(true);
        QTest::qWait(200);
        batchOperateWidget->sltSelectAll();
        QTest::qWait(200);
        batchOperateWidget->sltUnSelectAll();
    }

    dApp->getMainWindow()->m_pAllPicView->m_pThumbnailListView->clearSelection();
    if (batchOperateWidget) {
        ExpansionPanel::FilteData data;

        data.type = ItemType::ItemTypeNull;
        batchOperateWidget->sltCurrentFilterChanged(data);
        QTest::qWait(200);

        data.type = ItemType::ItemTypePic;
        batchOperateWidget->sltCurrentFilterChanged(data);
        QTest::qWait(200);

        data.type = ItemType::ItemTypeVideo;
        batchOperateWidget->sltCurrentFilterChanged(data);
        QTest::qWait(200);
    }
}

TEST(allpicview, viewpaneltest)
{
    TEST_CASE_NAME("viewpaneltest")
    QString publicTestPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + "test";
    QString testPath = publicTestPath;
    MainWindow *w = dApp->getMainWindow();
    AllPicView *a = w->m_pAllPicView;
    QTestEventList tl;
    tl.addMouseMove(QPoint(0, 0), 100);
    tl.addMousePress(Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier, QPoint(0, 0), 100);
    tl.addMouseMove(a->m_pImportView->geometry().center());
    tl.simulate(a->m_pImportView);
    QTest::qWait(500);
}

TEST(allpicview, allpicview_other_test)
{
    TEST_CASE_NAME("allpicview_other_test")
    MainWindow *w = dApp->getMainWindow();
    QMimeData mimedata;
    QList<QUrl> li;
    QString lastImportPath =  QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).first();
    lastImportPath += "/AlbumtestResource/test/4l6r5y.png";
    li.append(QUrl(lastImportPath));
    mimedata.setUrls(li);

    QPoint pos = QPoint(20, 20);
    QDragEnterEvent dee(pos, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
    w->m_pAllPicView->dragEnterEvent(&dee);

    QDragMoveEvent dme(pos, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
    w->m_pAllPicView->dragMoveEvent(&dme);

    QDropEvent de(pos, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
    w->m_pAllPicView->dropEvent(&de);
}
