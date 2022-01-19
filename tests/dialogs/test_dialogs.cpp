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
#include <QtConcurrent/QtConcurrent>
#include <gmock/gmock-matchers.h>
#include <QTestEventList>

#define private public
#define protected public

#include "application.h"
#include "dialogs/albumcreatedialog.h"
#include "dialogs/albumdeletedialog.h"
#include "widgets/dialogs/imgdeletedialog.h"
#include "controller/exporter.h"
#include "../test_qtestDefine.h"
#include "ac-desktop-define.h"
#include "testtoolkits.h"
#include "thumbnaillistview.h"
#include "QTestEventList"

//三个界面的删除操作
TEST(allPic, picdelete)
{
    TEST_CASE_NAME("picdelete")
    MainWindow *w = dApp->getMainWindow();
    QTestEventList e;

    clickToAllPictureView();

    //------右键删除---------
    QPoint p1(60, 120);
    e.addMouseMove(p1);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(w->m_pAllPicView->m_pThumbnailListView->viewport());
    e.clear();

    //所有照片
    auto menu = runContextMenu(w->m_pAllPicView->m_pThumbnailListView->viewport(), p1);
    using TR_SUBORDINATE_t = PointerTypeGetter < decltype(w->m_pAllPicView->m_pThumbnailListView) >::type;

    asynchronousObject asynchronous;
    stubDialog(
    [ & ]() {
        QMetaObject::invokeMethod(&asynchronous, "asynchronousRunActionFromMenu"
                                  , Qt::QueuedConnection, Q_ARG(QMenu *, menu), Q_ARG(QString, TR_SUBORDINATE_t::tr("Delete")));
    },
    [ = ]() {
        QMetaObject::invokeMethod(w, [ = ]() {
            ImgDeleteDialog *dialog = qobject_cast<ImgDeleteDialog *>(qApp->activeModalWidget());
            QTestEventList e;
            e.addKeyPress(Qt::Key::Key_Tab);
            e.simulate(dialog->getButton(0));
            QTest::qWait(200);

            e.simulate(dialog->getButton(1));
            QTest::qWait(200);

            QWidget *closeButton =  dialog->findChild<QWidget *>("DTitlebarDWindowCloseButton");
            e.simulate(closeButton);
            QTest::qWait(200);

            e.simulate(dialog->getButton(0));
            QTest::qWait(200);

            e.clear();
            e.addKeyPress(Qt::Key::Key_Escape);//这个会让它退出去，不需要执行done
            e.simulate(dialog);
        }, Qt::QueuedConnection);

    });
}

TEST(albumcreatedialog, dia1)
{
    TEST_CASE_NAME("dia1")
    AlbumCreateDialog *a = new AlbumCreateDialog;
    ASSERT_TRUE(a->getCreateAlbumName().isEmpty());

    QTestEventList event;
    event.addKeyClick(Qt::Key_Escape);
    event.simulate(a);

    QTest::keyClicks(a->getEdit(), "test1");
    emit a->buttonClicked(0, "");
}

TEST(albumdeletedialog, deletdialog)
{
    TEST_CASE_NAME("deletdialog")
    AlbumDeleteDialog *d = new AlbumDeleteDialog;
    QTestEventList e;
    e.addKeyClick(Qt::Key_A);
    e.simulate(d);
    e.clear();
}

TEST(albumdeletedialog, exportdialog)
{
    TEST_CASE_NAME("exportdialog")
    CExportImageDialog *c = new CExportImageDialog;
    c->deleteLater();
}
