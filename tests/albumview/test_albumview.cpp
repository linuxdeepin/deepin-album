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
#include <QString>
#include <QTestEventList>

#define private public
#define protected public

#include "leftlistview.h"
#include "application.h"
#include "frame/mainwidget.h"
#include "module/view/navigationwidget.h"
#include "module/view/scen/graphicsitem.h"
#include "searchview/searchview.h"
#include "module/view/scen/imageview.h"
#include "mainwindow.h"
#include "albumview.h"
#include "baseutils.h"
#include "dbmanager.h"
#include "albumcreatedialog.h"
#include "../test_qtestDefine.h"
#include "ac-desktop-define.h"

#include <stub-tool/cpp-stub/stub.h>
#include <stub-tool/stub-ext/stubext.h>

TEST(AlbumView, deleteAll)
{
    TEST_CASE_NAME("deleteAll")
    ImageEngineApi::instance()->loadFirstPageThumbnails(50);
    QTest::qWait(500);
    MainWindow *w = dApp->getMainWindow();
    w->loadZoomRatio();
}

TEST(AlbumView, removeTestImagesInfo)
{
    TEST_CASE_NAME("removeTestImagesInfo")
    QStringList image_list;
    auto finfos = utils::image::getImagesInfo(testPath_test);
    auto finfos1 = utils::image::getImagesInfo(testPath_test, false);
    auto pixmap = utils::image::getDamagePixmap(true);
    for (auto info : finfos) {
        image_list << info.absoluteFilePath();
    }
    QStringList res = DBManager::instance()->getAllPaths();
    QTest::qWait(1000);
    DBManager::instance()->removeImgInfos(res);
    DBImgInfoList dbinfos;
    for (auto i : image_list) {
        DBImgInfo info;
        info.filePath = i;
        dbinfos << info;
    }
    MainWindow *w = dApp->getMainWindow();
    AllPicView *a = w->m_pAllPicView;
    a->updatePicsThumbnailView();
    QTest::qWait(1000);
}

TEST(AlbumView, clickImportViewBtn)
{
    TEST_CASE_NAME("clickImportViewBtn")
    MainWindow *w = dApp->getMainWindow();
//    AllPicView *a = w->m_pAllPicView;

    QStringList image_list;
    auto finfos = utils::image::getImagesInfo(testPath_test);
    for (auto info : finfos) {
        image_list << info.absoluteFilePath();
    }
    QTest::qWait(500);

    if (image_list.size() > 0)
        ImageEngineApi::instance()->insertImage(image_list.first(), "");
}

TEST(AlbumView, ini)
{
    TEST_CASE_NAME("ini")
    MainWindow *w = dApp->getMainWindow();
    w->loadZoomRatio();

    QPoint pos(10, 10);
    QTestEventList event;
    event.addMouseMove(pos);
    event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pos);
    event.simulate(w->getButG()->button(0));
    event.clear();
    QTest::qWait(500);

    event.addMouseMove(pos);
    event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pos);
    event.simulate(w->getButG()->button(1));
    event.clear();
    QTest::qWait(500);

    event.addMouseMove(pos);
    event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pos);
    event.simulate(w->getButG()->button(2));
    event.clear();
    QTest::qWait(500);
}

TEST(AlbumView, iniAlbum)
{
    TEST_CASE_NAME("iniAlbum")
    QTest::qWait(500);
    MainWindow *w = dApp->getMainWindow();
    w->showEvent(nullptr);
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(2));
    event.clear();
    QTest::qWait(500);

    AlbumView *a = w->m_pAlbumview;
    int width = a->m_pStatusBar->m_pSlider->slider()->width();
    QPoint pos(width / 10 * a->m_pStatusBar->m_pSlider->slider()->sliderPosition(), 10);
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
        QTest::qWait(400);
    }
    pos = QPoint(width / 10 * a->m_pStatusBar->m_pSlider->slider()->sliderPosition(), 10);
    for (int i = 0; i < 5; i++) {
        event.addMousePress(Qt::LeftButton, Qt::NoModifier, pos);
        pos = pos - QPoint(20, 0);
        event.addMouseMove(pos);
        event.simulate(a->m_pStatusBar->m_pSlider->slider());
        event.clear();
        QTest::qWait(400);
    }
}

TEST(AlbumView, createNewAlbumFromDialog)
{
    TEST_CASE_NAME("createNewAlbumFromDialog")
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
    emit dApp->signalM->sigCreateNewAlbumFrom("test-album2");
}

TEST(AlbumView, dragPhotoToAnAlbum)
{
    TEST_CASE_NAME("dragPhotoToAnAlbum")
    MainWindow *w = dApp->getMainWindow();

    w->albumBtnClicked();
    QTest::qWait(100);
    AlbumView *a = w->m_pAlbumview;
    // open import widget
    QTestEventList event;
    event.addMousePress(Qt::MouseButton::LeftButton, Qt::NoModifier, QPoint(10, 10));
    event.addMouseRelease(Qt::MouseButton::LeftButton, Qt::NoModifier, QPoint(10, 10));
    event.simulate(a->m_pLeftListView->m_pPhotoLibListView->viewport());
    event.clear();
    QTest::qWait(500);

    LeftListWidget *albumList = a->m_pLeftListView->m_pCustomizeListView;
    a->pImportTimeLineWidget->setFocus();
    if (albumList->count() > 0) {
        QList<QWidget *> widgets = a->pImportTimeLineWidget->findChildren<QWidget *>("");
        for (int index = 0; index < widgets.count(); index++) {
            if (!strcmp(widgets.at(index)->metaObject()->className(), ("ThumbnailListView"))) {
                ThumbnailListView *listview = static_cast<ThumbnailListView *>(widgets.at(index));
                listview->setFocus();

                QString jpgItemPath = testPath_test + "/2k9o1m.png";
                QString text = "xxxxxxxxxxxxxx";
                QIcon icon = QIcon(":/resources/images/other/deepin-album.svg");
                QIcon icon_hover = QIcon(":/resources/images/other/deepin-album.svg");
                QByteArray itemData;
                QDataStream dataStream(&itemData, QIODevice::WriteOnly);
                dataStream << text << icon << icon_hover;
                QMimeData mimedata;
                mimedata.setData(QStringLiteral("TestListView/text-icon-icon_hover"), itemData);
                QList<QUrl> li;
                li.append(QUrl::fromLocalFile(jpgItemPath));
                mimedata.setUrls(li);

                QModelIndex model_index;
                emit a->m_pLeftListView->m_pCustomizeListView->pressed(model_index);
                QTest::qWait(200);
                const QPoint pos1 = a->m_pLeftListView->m_pCustomizeListView->pos() + QPoint(90, 20);
                qDebug() << "pos1 " << pos1;
                QDragEnterEvent eEnter(pos1, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
                dApp->getDAppNew()->sendEvent(a->m_pLeftListView->m_pCustomizeListView, &eEnter);
                QTest::qWait(200);

                QDragMoveEvent eMove(pos1, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
                dApp->getDAppNew()->sendEvent(a->m_pLeftListView->m_pCustomizeListView, &eMove);
                QTest::qWait(200);

                QDropEvent e(pos1, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
                dApp->getDAppNew()->sendEvent(a->m_pLeftListView->m_pCustomizeListView, &e);
                QTest::qWait(200);

                dApp->getDAppNew()->sendEvent(a->m_pLeftListView->m_pCustomizeListView, &eEnter);
                QTest::qWait(200);

                QDragLeaveEvent eLeave;
                dApp->getDAppNew()->sendEvent(a->m_pLeftListView->m_pCustomizeListView, &eLeave);
                QTest::qWait(500);
                break;
            }
        }
    }
}

TEST(AlbumView, leftMenu)
{
    TEST_CASE_NAME("leftMenu")
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
    QTest::qWait(500);
    event.simulate(a->m_pLeftListView->m_pCustomizeListView->viewport());
    QTest::qWait(500);
    dApp->getDAppNew()->processEvents();
    event.simulate(a->m_pLeftListView->m_pMountListWidget->viewport());
    QTest::qWait(500);
    event.simulate(a->m_waitDeviceScandialog->m_closeDeviceScan);
    QTest::qWait(500);
    dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Light);
    dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Dark);
    QTest::qWait(500);
}

TEST(AlbumView, iniAlbum1)
{
    TEST_CASE_NAME("iniAlbum1")
    QTest::qWait(500);
    MainWindow *w = dApp->getMainWindow();
    w->showEvent(nullptr);
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(2));
    event.clear();
    QTest::qWait(500);

    AlbumView *a = w->m_pAlbumview;
    int width = a->m_pStatusBar->m_pSlider->slider()->width();
    QPoint pos(width / 10 * a->m_pStatusBar->m_pSlider->slider()->sliderPosition(), 10);
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
        QTest::qWait(400);
    }
    pos = QPoint(width / 10 * a->m_pStatusBar->m_pSlider->slider()->sliderPosition(), 10);
    for (int i = 0; i < 5; i++) {
        event.addMousePress(Qt::LeftButton, Qt::NoModifier, pos);
        pos = pos - QPoint(20, 0);
        event.addMouseMove(pos);
        event.simulate(a->m_pStatusBar->m_pSlider->slider());
        event.clear();
        QTest::qWait(400);
    }
}

TEST(AlbumView, deleteImgRecovery)
{
    TEST_CASE_NAME("deleteImgRecovery")
    QTest::qWait(500);
    MainWindow *w = dApp->getMainWindow();
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(2));
    event.clear();
    AlbumView *a = w->m_pAlbumview;
    a->m_currentType = COMMON_STR_TRASH;
    a->updateRightView();
    ImageEngineApi::instance()->recoveryImagesFromTrash(a->m_pRightThumbnailList->getAllPaths());
}

TEST(AlbumView, exportAlbum)//roc
{
    TEST_CASE_NAME("exportAlbum")
//    qDebug() << "AlbumView exportAlbum count = " << count_testDefine++;
//    AlbumCreateDialog *ad = new AlbumCreateDialog;
//    QTest::keyClicks(ad->getEdit(), "newtestalbum1");
//    emit ad->buttonClicked(1, "");
//    QTest::qWait(200);
//    QString testPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + "test";

//    MainWindow *w = dApp->getMainWindow();
//    AlbumView *a = w->m_pAlbumview;
//    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "newtestalbum1", a, false);
//    QTest::qWait(500);
//    QStringList testImage = ImageEngineApi::instance()->get_AllImagePath();
//    QString exportFile = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + "exportTest";
//    QDir Dir(exportFile);
//    if (!Dir.isEmpty()) {
//        QDirIterator DirsIterator(exportFile, QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDirIterator::NoIteratorFlags);
//        while (DirsIterator.hasNext()) {
//            if (!Dir.remove(DirsIterator.next())) { // 删除文件操作如果返回否，那它就是目录
//                QDir(DirsIterator.filePath()).removeRecursively(); // 删除目录本身以及它下属所有的文件及目录
//            }
//        }
//    }

//    Exporter::instance()->instance()->exportAlbum(testImage, "newtestalbum1");
//    Exporter::instance()->instance()->exportImage(testImage.mid(0, 1));
}

TEST(AlbumImageButton, btn)
{
    TEST_CASE_NAME("btn")
    QString pic = testPath_test + "/2k9o1m.png";
    AlbumImageButton *b = new AlbumImageButton(pic, pic, pic, pic, nullptr);
    b->show();
    b->setPropertyPic("a", "", pic, pic, pic, pic);
    QTestEventList e;
    QPoint point = b->pos();
    e.addMouseMove(point + QPoint(1, 1), 10);
    e.addMouseMove(point + QPoint(2, 2), 10);
    e.addMouseMove(point + QPoint(100, 100), 10);
    e.simulate(b);
    b->hide();
    ASSERT_TRUE(b != nullptr);
}

// 导入重复照片提示
//TEST(AlbumView, ImportDuplicatePhotos)
//{
//    TEST_CASE_NAME("load")
//    // albumview ImportDuplicatePhotos
//    AlbumCreateDialog *ad = new AlbumCreateDialog;
//    ad->show();
//    QTest::qWait(500);
//    QTest::keyClicks(ad->getEdit(), "ImportDuplicatePhotosAlbum");
//    emit ad->buttonClicked(1, "");
//    QTest::qWait(200);
//    QString testPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + "test";
//    MainWindow *w = dApp->getMainWindow();
//    w->albumBtnClicked();
//    AlbumView *a = w->m_pAlbumview;
//    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "ImportDuplicatePhotosAlbum", a, false);
//    QTest::qWait(200);
//    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "ImportDuplicatePhotosAlbum", a, false);
//    QTest::qWait(500);
//    // allpicview ImportDuplicatePhotos
//    w->allPicBtnClicked();
//    AllPicView *allpicview = w->m_pAllPicView;
//    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "", allpicview, true);
//    QTest::qWait(200);
//    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "", allpicview, true);
//    QTest::qWait(1000);
//    // timelineview ImportDuplicatePhotos
//    w->timeLineBtnClicked();
//    TimeLineView *timelineview = w->m_pTimeLineView;
//    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "", timelineview, true);
//    QTest::qWait(200);
//    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "", timelineview, true);
//    QTest::qWait(500);

//    QTestEventList e;
//    e.addMouseClick(Qt::MouseButton::LeftButton);
//    w->albumBtnClicked();
//    e.simulate(w->getButG()->button(2));
//    e.clear();

//    LeftListView *leftlist = a->m_pLeftListView;
//    DMenu *menu = leftlist->m_pMenu;
//    QPoint point = leftlist->pos();
////    QTestEventList e;
//    e.addMouseClick(Qt::MouseButton::RightButton, Qt::NoModifier, point, 10);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 10);
//    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 10);
//    e.addDelay(50);
//    e.simulate(menu);
//    e.clear();
//    QTest::qWait(100);

//    e.addMouseClick(Qt::MouseButton::RightButton, Qt::NoModifier, point, 10);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 10);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 10);
//    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 10);
//    e.addDelay(50);
//    e.simulate(menu);
//    e.clear();
//    QTest::qWait(100);

//    e.addMouseClick(Qt::MouseButton::RightButton, Qt::NoModifier, point, 10);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 10);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 10);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 10);
//    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 10);
//    e.addDelay(50);
//    e.simulate(menu);
//    e.clear();
//    QTest::qWait(100);

//    e.addMouseClick(Qt::MouseButton::RightButton, Qt::NoModifier, point, 10);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 10);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 10);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 10);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 10);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 10);
//    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 10);
//    e.addDelay(50);
//    e.simulate(menu);
//    e.clear();
//    QTest::qWait(100);
//    a->hide();
//    leftlist->hide();
//}

TEST(LeftListView, update)
{
    TEST_CASE_NAME("load")
    LeftListView *leftlist = new LeftListView;
    leftlist->updatePhotoListView();
    leftlist->updateCustomizeListView();
    leftlist->show();
    QPoint point = leftlist->pos();
    leftlist->showMenu(point + QPoint(2, 10));
    point = leftlist->m_pMenu->pos();
    QTestEventList e;
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, point - QPoint(1, 1), 10);
    e.simulate(leftlist);
    e.clear();
    leftlist->getNewAlbumName();

//    leftlist->onMenuClicked(act);
    e.addKeyClick(Qt::Key_Delete, Qt::ControlModifier, 100);
    e.simulate(leftlist);
    e.clear();

    QString jpgItemPath = testPath_test + "/2k9o1m.png";
    QString text = "xxxxxxxxxxxxxx";
    QIcon icon = QIcon(":/resources/images/other/deepin-album.svg");
    QIcon icon_hover = QIcon(":/resources/images/other/deepin-album.svg");
    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << text << icon << icon_hover;
    QMimeData mimedata;
    mimedata.setData(QStringLiteral("TestListView/text-icon-icon_hover"), itemData);
    QList<QUrl> li;
    li.append(QUrl::fromLocalFile(jpgItemPath));
    mimedata.setUrls(li);

    LeftListWidget *wid = new LeftListWidget;
    QPoint pos = wid->pos();
    e.addMousePress(Qt::LeftButton, Qt::ControlModifier, pos, 10);
    e.addKeyPress(Qt::Key_0, Qt::NoModifier, 10);
    e.addKeyRelease(Qt::Key_0, Qt::NoModifier, 10);
    e.addMouseMove(pos + QPoint(1, 1));
    e.addMouseMove(pos + QPoint(2, 2));
    e.simulate(wid);
    e.clear();
    QDragMoveEvent eMove(pos, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
    dApp->getDAppNew()->sendEvent(wid, &eMove);
    QTest::qWait(200);
    QDropEvent ed(pos, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
    dApp->getDAppNew()->sendEvent(wid, &ed);
    QTest::qWait(200);
}

//TEST(AlbumView, upDataAllAlbumView)
//{
//    TEST_CASE_NAME("upDataAllAlbumView")
//    MainWindow *w = dApp->getMainWindow();
//    w->albumBtnClicked();
//    QTest::qWait(500);
//    QString s1;
//    w->m_pAlbumview->updateRightNoTrashView();
//    w->m_pAlbumview->updateRightTrashView();
//    w->m_pAlbumview->updateRightImportView();
//    w->m_pAlbumview->updateRightMyFavoriteView();
//    w->m_pAlbumview->updateRightMountView();
//    w->m_pAlbumview->updateRightImportView();
//    w->m_pAlbumview->leftTabClicked();
//    w->m_pAlbumview->updateImportComboBox();
//    w->m_pAlbumview->importAllBtnClicked();
//    w->m_pAlbumview->importSelectBtnClicked();
//    w->m_pAlbumview->needUnMount(s1);
//    w->m_pAlbumview->onKeyDelete();
//    w->m_pAlbumview->onKeyF2();
//    w->m_pAlbumview->importDialog();
//    w->m_pAlbumview->onWaitDialogIgnore();
//    w->m_pAlbumview->openImage(0);
////    w->m_pAlbumview->onUpdataAlbumRightTitle("timelinesview");//coredump
//    w->m_pAlbumview->SearchReturnUpdate();
//    w->m_pAlbumview->onUnMountSignal("");

//    QString jpgItemPath = testPath_test + "/2k9o1m.png";
//    QString text = "xxxxxxxxxxxxxx";
//    QIcon icon = QIcon(":/resources/images/other/deepin-album.svg");
//    QIcon icon_hover = QIcon(":/resources/images/other/deepin-album.svg");
//    QByteArray itemData;
//    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
//    dataStream << text << icon << icon_hover;
//    QMimeData mimedata;
//    mimedata.setData(QStringLiteral("TestListView/text-icon-icon_hover"), itemData);
//    QList<QUrl> li;
//    li.append(QUrl::fromLocalFile(jpgItemPath));
//    mimedata.setUrls(li);

//    QPoint pos = w->m_pAlbumview->pos();
//    QDragMoveEvent eMove(pos, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
//    dApp->getDAppNew()->sendEvent(w->m_pAlbumview, &eMove);
//    QTest::qWait(200);
//    QDropEvent ed(pos, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
//    dApp->getDAppNew()->sendEvent(w->m_pAlbumview, &ed);
//    QTest::qWait(200);
//}

TEST(AlbumView, menuOpenImage_test)
{
    TEST_CASE_NAME("menuOpenImage_test")
    MainWindow *w = dApp->getMainWindow();
    w->albumBtnClicked();
    QTest::qWait(500);

    QStringList paths = DBManager::instance()->getAllPaths();
    QString path;
    if (paths.size() > 0) {
        path = paths.first();
    }
    bool isfullscr = false;
    bool isSlideShow = false;
    w->m_pAlbumview->menuOpenImage(path, paths, isfullscr, isSlideShow);
    QTest::qWait(500);
}

TEST(AlbumView, findPicturePathByPhone_test)
{
    TEST_CASE_NAME("findPicturePathByPhone_test")
    MainWindow *w = dApp->getMainWindow();
    w->albumBtnClicked();
    QTest::qWait(500);

    QString testPath = testPath_test;
    w->m_pAlbumview->findPicturePathByPhone(testPath);
    QTest::qWait(500);
}

TEST(AlbumView, onLeftListDropEvent_test)
{
    TEST_CASE_NAME("onLeftListDropEvent_test")
    MainWindow *w = dApp->getMainWindow();
    w->albumBtnClicked();
    w->m_pAlbumview->m_pLeftListView->m_pCustomizeListView->setCurrentRow(0);
    QModelIndex curIndex = w->m_pAlbumview->m_pLeftListView->m_pCustomizeListView->currentIndex();
    QTest::qWait(500);
    w->m_pAlbumview->onLeftListDropEvent(curIndex);
    QTest::qWait(500);
}

TEST(AlbumViewListWidget, viewlist)
{
    TEST_CASE_NAME("viewlist")
    AlbumViewListWidget *vi = new AlbumViewListWidget;
    vi->on_rangeChanged(1, 1);
}


TEST(AlbumView, albumView_other1_test)
{
    TEST_CASE_NAME("albumView_other1_test")
    MainWindow *w = dApp->getMainWindow();
    w->albumBtnClicked();

    QMimeData mimedata;
    QList<QUrl> li;
    QString lastImportPath =  QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).first();
    lastImportPath += "/test/4l6r5y.png";
    li.append(QUrl(lastImportPath));
    mimedata.setUrls(li);


    QPoint pos = QPoint(20, 20);
    QDragEnterEvent dee(pos, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
    w->m_pAlbumview->m_pLeftListView->m_pPhotoLibListView->dragEnterEvent(&dee);

    QDragMoveEvent dme(pos, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
    w->m_pAlbumview->m_pLeftListView->m_pPhotoLibListView->dragMoveEvent(&dme);

    QDropEvent de(pos, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
    w->m_pAlbumview->m_pLeftListView->m_pPhotoLibListView->dropEvent(&de);

    w->m_pAlbumview->m_pLeftListView->m_pPhotoLibListView->setViewMode(QListView::IconMode);
    w->m_pAlbumview->m_pLeftListView->m_pPhotoLibListView->viewOptions();

    w->m_pAlbumview->m_currentType = ALBUM_PATHTYPE_BY_PHONE;
    w->m_pAlbumview->updateRightView();

    w->m_pAlbumview->m_currentType = COMMON_STR_FAVORITES;
    w->m_pAlbumview->updateRightView();

    w->m_pAlbumview->updateAlbumView(w->m_pAlbumview->m_currentType);
    w->m_pAlbumview->updateRightMyFavoriteView();
    w->m_pAlbumview->updateRightNoTrashView();
//    w->m_pAlbumview->onTrashDeleteBtnClicked();

    w->m_pAlbumview->dragEnterEvent(&dee);
    w->m_pAlbumview->dragMoveEvent(&dme);

    w->m_pAlbumview->onKeyF2();

    w->m_pAlbumview->SearchReturnUpdate();
    QString str = "";
    w->m_pAlbumview->findPicturePathByPhone(str);
    w->m_pAlbumview->importAllBtnClicked();
    w->m_pAlbumview->needUnMount("");

    w->m_pAlbumview->m_currentType = COMMON_STR_RECENT_IMPORTED;
    w->m_pAlbumview->restorePicNum();

    w->m_pAlbumview->m_currentType = COMMON_STR_FAVORITES;
    w->m_pAlbumview->restorePicNum();

    w->m_pAlbumview->m_currentType = COMMON_STR_CUSTOM;
    w->m_pAlbumview->restorePicNum();

    w->m_pAlbumview->onThemeTypeChanged(DGuiApplicationHelper::DarkType);
    w->m_pAlbumview->onThemeTypeChanged(DGuiApplicationHelper::LightType);

    w->m_pAlbumview->onImportFailedToView();
    w->m_pAlbumview->onUpdateFavoriteNum();
    w->m_pAlbumview->onWaitDailogTimeout();


    QTest::qWait(500);
}

TEST(AlbumViewListWidget, albumViewList_other2_test)
{
    TEST_CASE_NAME("albumViewList_other2_test")

    AlbumViewListWidget *vi = new AlbumViewListWidget;
    vi->on_rangeChanged(1, 1);
}

TEST(AlbumViewList, albumViewList_other3_test)
{
    TEST_CASE_NAME("albumViewList_other3_test")

    MainWindow *w = dApp->getMainWindow();

    QEnterEvent ee(QPoint(10, 10), QPoint(10, 10), QPoint(10, 10));
    w->m_pAlbumview->m_pLeftListView->m_pAddListBtn->enterEvent(&ee);

}

TEST(AlbumViewList, albumViewList_other4_test)
{
    TEST_CASE_NAME("albumViewList_other4_test")

    MainWindow *w = dApp->getMainWindow();
    ThumbnailListView *pThumbnailListView = w->findChild<ThumbnailListView *>(ImportTime_pThumbnailListView);

    if (pThumbnailListView) {
        emit pThumbnailListView->openImage(0);
        emit pThumbnailListView->menuOpenImage("", QStringList(), false);
        QStringList stringList;
        emit pThumbnailListView->sigGetSelectedPaths(&stringList);
        emit pThumbnailListView->sigSelectAll();
        emit pThumbnailListView->sigMouseMove();
        emit pThumbnailListView->sigMouseRelease();
        emit pThumbnailListView->customContextMenuRequested(QPoint());
    }

    DCommandLinkButton *pChose = w->findChild<DCommandLinkButton *>(All_Picture_Thembnail);

    if (pChose) {
        emit pChose->clicked();
    }

    ImportTimeLineView *pImpTimeLineWidget = w->findChild<ImportTimeLineView *>(AlbumView_pImpTimeLineWidget);

    if (pImpTimeLineWidget) {
//        pImpTimeLineWidget->on_DelLabel();
        pImpTimeLineWidget->on_MoveLabel(1, "", "", "");


        QMimeData mimedata;
        QList<QUrl> li;
        QString lastImportPath =  QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).first();
        lastImportPath += "/test/4l6r5y.png";
        li.append(QUrl(lastImportPath));
        mimedata.setUrls(li);

        QPoint pos = QPoint(20, 20);
        QDragEnterEvent dee(pos, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
        pImpTimeLineWidget->dragEnterEvent(&dee);

        QDragMoveEvent dme(pos, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
        pImpTimeLineWidget->dragMoveEvent(&dme);

        QDropEvent de(pos, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
        pImpTimeLineWidget->dropEvent(&de);
    }
}

TEST(AlbumViewList, albumViewList_other5_test)
{
    TEST_CASE_NAME("albumViewList_other5_test")

    MainWindow *w = dApp->getMainWindow();

    QMimeData mimedata;
    QList<QUrl> li;
    QString lastImportPath =  QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).first();
    lastImportPath += "/test/4l6r5y.png";
    li.append(QUrl(lastImportPath));
    mimedata.setUrls(li);

    QPoint pos = QPoint(20, 20);
    QDragEnterEvent dee(pos, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
    w->m_pAlbumview->m_pImportView->dragEnterEvent(&dee);

    QDragMoveEvent dme(pos, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
    w->m_pAlbumview->m_pImportView->dragMoveEvent(&dme);

    QDropEvent de(pos, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
    w->m_pAlbumview->m_pImportView->dropEvent(&de);

    w->m_pAlbumview->m_pImportView->imageImported(true);
    w->m_pAlbumview->m_pImportView->imageImported(false);


    MainWidget *mainWidget = w->m_commandLine->findChild<MainWidget *>("MainWidget");

    if (mainWidget) {
        NavigationWidget *nw = mainWidget->m_viewPanel->m_nav.widget();
        nw->setAlwaysHidden(false);
        nw->setAlwaysHidden(true);
        nw->transImagePos(QPoint(0, 0));

        mainWidget->m_viewPanel->m_ttbc->stopLoadAndClear();
        mainWidget->m_viewPanel->m_ttbc->getAllFileList();
        mainWidget->m_viewPanel->m_ttbc->deleteImage();
        mainWidget->m_viewPanel->m_ttbc->onSilmoved();
        mainWidget->m_viewPanel->m_ttbc->onNeedContinueRequest();
        mainWidget->m_viewPanel->m_ttbc->setCurrentDir("");
    }
}


TEST(AlbumViewList, albumViewList_other6_test)
{
    TEST_CASE_NAME("albumViewList_other6_test")
//    MainWindow *w = dApp->getMainWindow();
    GraphicsMovieItem gmi("", "");
    ImageView *iv = new ImageView;
    iv->setImage("");
    iv->windowRelativeScale();
    iv->imageRelativeScale();
    iv->isWholeImageVisible();
    iv->onThemeTypeChanged();

    ImgInfoDialog iid("");
    iid.height();
}
