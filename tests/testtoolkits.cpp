/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     WangZhengYang <wangzhengyang@uniontech.com>
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

#define protected public
#define private public
#include "testtoolkits.h"
#include <QTestEventList>
#include <iostream>
#include "albumcreatedialog.h"
#include "imageengineapi.h"
#include "application.h"

void clickToAllPictureView()
{
    auto w = dApp->getMainWindow();
    auto allPicButton = w->m_pAllPicBtn;

    QTestEventList e;
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier, QPoint(10, 10), 100);
    e.addDelay(500);
    e.simulate(allPicButton);
}

void clickToTimelineView()
{
    auto w = dApp->getMainWindow();
    auto allPicButton = w->m_pTimeBtn;

    QTestEventList e;
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier, QPoint(10, 10), 100);
    e.addDelay(500);
    e.simulate(allPicButton);
}

void clickToAlbumView()
{
    auto w = dApp->getMainWindow();
    auto allPicButton = w->m_pAlbumBtn;

    QTestEventList e;
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier, QPoint(10, 10), 100);
    e.addDelay(500);
    e.simulate(allPicButton);
}

void dClickTitleBar()
{
    auto w = dApp->getMainWindow();
    auto titleBar = w->titlebar();
    QPoint maxButtonPos(titleBar->width() - 75, 10);

    QTestEventList e;
    e.addMouseDClick(Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier, maxButtonPos, 100);
    e.addDelay(500);
    e.simulate(titleBar);
}

//index需要去实现文件里面查，然后同步过来
const int VIEW_ALLPIC = 0;
const int VIEW_TIMELINE = 1;
const int VIEW_ALBUM = 2;
const int VIEW_SEARCH = 3;
const int VIEW_IMAGE = 4;
const int VIEW_SLIDE = 5;

void exitImageView()
{
    auto w = dApp->getMainWindow();
    if (w->m_iCurrentView == VIEW_IMAGE) {
        QTestEventList e;
        e.addKeyClick(Qt::Key::Key_Escape);
        e.addDelay(300);
//        e.simulate(w->m_commandLine);
    }
}

void checkAndSwitchTo(int index)
{
    exitImageView();
    switch (index) {
    default:
        break;
    case VIEW_ALBUM:
        clickToAlbumView();
        break;
    case VIEW_ALLPIC:
        clickToAllPictureView();
        break;
    case VIEW_TIMELINE:
        clickToTimelineView();
        break;
    }
}

void clickToImportPage()
{
    checkAndSwitchTo(VIEW_ALBUM);

    auto leftList = dApp->getMainWindow()->m_pAlbumview->m_pLeftListView->m_pPhotoLibListView;

    QTestEventList e;
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier, QPoint(90, 10), 100);
    e.addDelay(500);
    e.simulate(leftList->viewport());
}

void clickToDeletePage()
{
    checkAndSwitchTo(VIEW_ALBUM);

    auto leftList = dApp->getMainWindow()->m_pAlbumview->m_pLeftListView->m_pPhotoLibListView;

    QTestEventList e;
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier, QPoint(90, 65), 100);
    e.addDelay(500);
    e.simulate(leftList->viewport());
}

void clickToFavoritePage()
{
    checkAndSwitchTo(VIEW_ALBUM);

    auto leftList = dApp->getMainWindow()->m_pAlbumview->m_pLeftListView->m_pPhotoLibListView;

    QTestEventList e;
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier, QPoint(90, 110), 100);
    e.addDelay(500);
    e.simulate(leftList->viewport());
}

void clickNewAlbumInAlbumPage(const QString &albumName)
{
    checkAndSwitchTo(VIEW_ALBUM);

    //下面这块是从老版本那边移植的
    MainWindow *w = dApp->getMainWindow();
    QList<QAction *> actions = w->actions();
    AlbumImageButton *btn = w->m_pAlbumview->m_pLeftListView->m_pAddListBtn;

    //QPoint pointbtn = btn->pos();
    QTestEventList event;
    //event.addMouseMove(pointbtn + QPoint(1, 1), 10);
    //event.addMouseMove(pointbtn + QPoint(2, 2), 10);
    event.addMouseClick(Qt::LeftButton, Qt::NoModifier);
    //event.addMouseMove(pointbtn + QPoint(200, 200), 10);
    event.simulate(btn);
    event.clear();
    QTest::qWait(500);
    QList<QWidget *> widgets = w->findChildren<QWidget *>("");
    foreach (auto widget, widgets) {
        if (!strcmp(widget->metaObject()->className(), "AlbumCreateDialog")) {
            AlbumCreateDialog *temp = static_cast<AlbumCreateDialog *>(widget);
            dynamic_cast<DLineEdit *>(temp->m_allTabOrder[0])->setText(albumName);
            QPoint pos(10, 10);
            event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pos);
            event.simulate(temp->getButton(1));
            event.clear();
            break;
        }
    }
    QTest::qWait(200);
}

void clickToCustomAlbum(int index)
{
    checkAndSwitchTo(VIEW_ALBUM);

    //要预先建立好相册，否则可能会出问题
    auto leftList = dApp->getMainWindow()->m_pAlbumview->m_pLeftListView->m_pCustomizeListView;

    QTestEventList e;
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier, QPoint(90, 20 + 40 * index), 100);
    e.addDelay(500);
    e.simulate(leftList->viewport());
}

QMenu *runContextMenu(QWidget *w, const QPoint &pos)
{
    QTest::qWait(1000); //调用的时候主动等待环境稳定

    bool ret = QTest::qWaitFor([ = ]() {
        QContextMenuEvent event(QContextMenuEvent::Mouse, pos);
        qApp->sendEvent(w, &event);
        QTest::qWait(200);
        QMenu *pMenu = qobject_cast<QMenu *>(qApp->activePopupWidget());
        if (pMenu != nullptr) {
            return true;
        } else {
            return false;
        }
    }, 10000);

    if (!ret) {
        std::cout << "Warning: bad menu pointer have returned" << std::endl;
    }

    QMenu *pMenu = qobject_cast<QMenu *>(qApp->activePopupWidget());

    if (pMenu) {
        pMenu->hide();
    }

    return pMenu;
}

void runActionFromMenu(QMenu *menu, const QString &actionName)
{
    QAction *result = nullptr;

    if (menu) {
        for (auto pAction : menu->actions()) {
            if (pAction->text() == actionName) {
                result = pAction;
                break;
            }
        }

        //CI崩溃后给个提示信息
        if (result == nullptr) {
            std::cout << "Error Action Name: " << actionName.toStdString() << std::endl;
            for (auto pAction : menu->actions()) {
                std::cout << pAction->text().toStdString() << std::endl;
            }
        }

        if (result == nullptr) {
            return;
        }

        emit result->triggered();
        QTest::qWait(500);
    }
}

bool checkIfInAlbum(const QString &path, int UID)
{
    QTest::qWait(1000); //调用的时候主动等待环境稳定
    return DBManager::instance()->isImgExistInAlbum(UID, path);
}

asynchronousObject::asynchronousObject(QObject *parent): QObject(parent)
{

}

void asynchronousObject::asynchronousRunActionFromMenu(QMenu *menu, QString actionName)
{
    qDebug() << __FUNCTION__ << "---";
    runActionFromMenu(menu, actionName);
}
