/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
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
#include "viewpanel.h"
#include "application.h"
#include "contents/imageinfowidget.h"
#include "controller/configsetter.h"
#include "controller/wallpapersetter.h"
#include "navigationwidget.h"
#include "scen/imageview.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "widgets/dialogs/filedeletedialog.h"
#include "widgets/printhelper.h"
#include <DMenu>
#include <QKeySequence>
#include <QJsonArray>
#include <QJsonDocument>
#include <QShortcut>
#include <QStyleFactory>
#include <QFileInfo>
#include "widgets/dialogs/imgdeletedialog.h"

namespace {

const int SWITCH_IMAGE_DELAY = 300;
const QString SHORTCUTVIEW_GROUP = "SHORTCUTVIEW";
const int VIEW_MAINWINDOW_POPVIEW = 4;

QString ss(const QString &text, const QString &defaultValue)
{
    QString str = dApp->setter->value(SHORTCUTVIEW_GROUP, text, defaultValue).toString();
    str.replace(" ", "");
    return str;
}

enum MenuItemId {
    IdFullScreen,
    IdExitFullScreen,
    IdStartSlideShow,
    IdPrint,
    IdAddToAlbum,
    IdExport,
    IdCopy,
    IdMoveToTrash,
    IdRemoveFromTimeline,
    IdRemoveFromAlbum,
    IdAddToFavorites,
    IdRemoveFromFavorites,
    IdShowNavigationWindow,
    IdHideNavigationWindow,
    IdRotateClockwise,
    IdRotateCounterclockwise,
    IdSetAsWallpaper,
    IdDisplayInFileManager,
    IdImageInfo,
    IdSubMenu,
};

}  // namespace

void ViewPanel::initPopupMenu()
{
    m_menu = new DMenu;
    connect(this, &ViewPanel::customContextMenuRequested, this, [ = ] {
        if (! m_infos.isEmpty()
#ifdef LITE_DIV
                && !m_infos.at(m_current).filePath.isEmpty()
#endif
           )
        {
            updateMenuContent();
            this->setCursor(Qt::ArrowCursor);
            m_menu->popup(QCursor::pos());
        }
    });
    connect(m_menu, &DMenu::aboutToHide, this, [ = ] {
        dApp->restoreOverrideCursor();
    });
    connect(m_menu, &DMenu::triggered, this, &ViewPanel::onMenuItemClicked);
    connect(dApp->setter, &ConfigSetter::valueChanged, this, [ = ] {
        if (this && this->isVisible())
        {
            updateMenuContent();
        }
    });
//    QShortcut *sc = new QShortcut(QKeySequence("Alt+Enter"), this);
//    sc->setContext(Qt::WindowShortcut);
//    connect(sc, &QShortcut::activated, this, [ = ] {
//        if (m_isInfoShowed)
//            emit dApp->signalM->hideExtensionPanel();
//        else
//            emit dApp->signalM->showExtensionPanel();
//        // Update panel info
//        m_info->setImagePath(m_infos.at(m_current).filePath);
//    });
}

void ViewPanel::appendAction(int id, const QString &text, const QString &shortcut)
{
    QAction *ac = new QAction(m_menu);
    addAction(ac);
    ac->setText(text);
    ac->setProperty("MenuID", id);
    ac->setShortcut(QKeySequence(shortcut));
    m_menu->addAction(ac);
}
void ViewPanel::appendAction_darkmenu(int id, const QString &text, const QString &shortcut)
{
    QAction *ac = new QAction(m_menu);
    addAction(ac);
    ac->setDisabled(true);
    ac->setText(text);
    ac->setProperty("MenuID", id);
    ac->setShortcut(QKeySequence(shortcut));
    m_menu->addAction(ac);
}

void ViewPanel::onMenuItemClicked(QAction *action)
{
    using namespace utils::base;
    using namespace utils::image;

    const QString path = m_infos.at(m_current).filePath;
    const int id = action->property("MenuID").toInt();

    switch (MenuItemId(id)) {
    case IdFullScreen:
    case IdExitFullScreen:
        toggleFullScreen();
        break;
    case IdStartSlideShow: {
        auto vinfo = m_vinfo;
        vinfo.fullScreen = window()->isFullScreen();
        vinfo.lastPanel = this;
        vinfo.path = path;
        vinfo.paths = paths();
        vinfo.viewMainWindowID = VIEW_MAINWINDOW_POPVIEW;

        QStringList pathlist;
        pathlist.clear();
        for(auto path: m_vinfo.paths)
        {
            if (QFileInfo(path).exists())
            {
                pathlist<<path;
            }
        }

        m_vinfo.paths = pathlist;
        emit dApp->signalM->startSlideShow(vinfo, m_vinfo.inDatabase);
        emit dApp->signalM->showSlidePanel(VIEW_MAINWINDOW_POPVIEW);
        break;
    }
    case IdPrint: {
        PrintHelper::showPrintDialog(QStringList(path), this);
        break;
    }

#if 1
    //添加到相册
    case IdAddToAlbum: {
        const QString album = action->data().toString();
        if (album != "Add to new album") {
            if (! DBManager::instance()->isImgExistInAlbum(album, path)) {
                emit dApp->signalM->sigAddToAlbToast(album);
            }
            DBManager::instance()->insertIntoAlbum(album, QStringList(path));
        } else {
            emit dApp->signalM->viewModeCreateAlbum(path);
        }
    }
    break;
    //收藏
    case IdAddToFavorites: {
        DBManager::instance()->insertIntoAlbum(COMMON_STR_FAVORITES, QStringList(path));
    }
    break;
    //取消收藏
    case IdRemoveFromFavorites: {
        DBManager::instance()->removeFromAlbum(COMMON_STR_FAVORITES, QStringList(path));
    }
    break;
    //导出
    case IdExport:
        emit dApp->signalM->exportImage(QStringList(path));
        break;
#endif
    case IdCopy:
        copyImageToClipboard(QStringList(path));
        break;
    case IdMoveToTrash: {
//        if (utils::common::VIEW_ALLPIC_SRN != m_viewType
//                && utils::common::VIEW_TIMELINE_SRN != m_viewType
//                && utils::common::VIEW_SEARCH_SRN != m_viewType
//                && COMMON_STR_RECENT_IMPORTED != m_viewType
//                && COMMON_STR_TRASH != m_viewType
//                && COMMON_STR_FAVORITES != m_viewType)
//        {
//            DBManager::instance()->removeFromAlbum(m_vinfo.viewType, QStringList(m_infos.at(m_current).filePath));
//            removeCurrentImage();
//        }
//        else if (COMMON_STR_TRASH == m_viewType)
//        {
//            ImgDeleteDialog *dialog = new ImgDeleteDialog(1);
//            dialog->show();
//            connect(dialog,&ImgDeleteDialog::imgdelete,this,[=]
//            {
//                dApp->m_imagetrashmap.remove(m_infos.at(m_current).filePath);
//                DBManager::instance()->removeTrashImgInfos(QStringList(m_infos.at(m_current).filePath));
//                removeCurrentImage();
//            });
//        }
//        else
//        {
        DBImgInfoList infos;
        DBImgInfo info;
        info = DBManager::instance()->getInfoByPath(m_infos.at(m_current).filePath);
#if 1
//        info.time = QDateTime::currentDateTime();
        info.changeTime = QDateTime::currentDateTime();
#endif
        infos << info;
        dApp->m_imageloader->addTrashImageLoader(QStringList(m_infos.at(m_current).filePath));
//        dApp->m_imagemap.remove(m_infos.at(m_current).filePath);
        DBManager::instance()->insertTrashImgInfos(infos);
        DBManager::instance()->removeImgInfos(QStringList(m_infos.at(m_current).filePath));
        removeCurrentImage();
//        }
    }
    break;
    case IdRemoveFromAlbum:
        DBManager::instance()->removeFromAlbum(m_vinfo.viewType, QStringList(m_infos.at(m_current).filePath));
        removeCurrentImage();
        break;
    case IdShowNavigationWindow:
        m_nav->setAlwaysHidden(false);
        break;
    case IdHideNavigationWindow:
        m_nav->setAlwaysHidden(true);
        break;
    case IdRotateClockwise:
        rotateImage(true);
        break;
    case IdRotateCounterclockwise:
        rotateImage(false);
        break;
    case IdSetAsWallpaper:
        dApp->wpSetter->setWallpaper(path);
        break;
    case IdDisplayInFileManager:
        emit dApp->signalM->showInFileManager(path);
        break;
    case IdImageInfo:
//        if (m_isInfoShowed)
//            emit dApp->signalM->hideExtensionPanel();
//        else
            emit dApp->signalM->showExtensionPanel();
        // Update panel info
        m_info->setImagePath(path);
        break;
    default:
        break;
    }

    updateMenuContent();
}

void ViewPanel::updateMenuContent()
{
    m_menu->clear();
    qDeleteAll(this->actions());

    if (m_vinfo.viewType.compare(COMMON_STR_TRASH) == 0) {
        return;
    }

    if (m_infos.isEmpty()) {
        return;
    }

    // 如果该图片原文件不存在,则不显示右键菜单
    if (!QFileInfo(m_infos.at(m_current).filePath).exists())
    {
        return;
    }

    if (window()->isFullScreen()) {

        appendAction(IdExitFullScreen, tr("Exit fullscreen"), ss("Fullscreen", "F11"));
    } else {

        appendAction(IdFullScreen, tr("Fullscreen"), ss("Fullscreen", "F11"));
    }
    appendAction(IdStartSlideShow, tr("Slide show"), ss("Slide show", "F5"));
#ifndef LITE_DIV
    appendAction(IdStartSlideShow, tr("Slide show"), ss("Slide show"));
#endif
//    appendAction(IdPrint, tr("Print"), ss("Print", "Ctrl+P"));
#ifndef LITE_DIV
    if (m_vinfo.inDatabase) {
        DMenu *am = createAlbumMenu();
        if (am) {
            m_menu->addMenu(am);
        }
    }
#endif
    m_menu->addSeparator();
    /**************************************************************************/
#if 1
    m_menu->addMenu(createAblumMenu());                                         //添加到相册
    appendAction(IdExport, tr("Export"), ss("Export", "Ctrl+E"));    //导出
#endif
    appendAction(IdCopy, tr("Copy"), ss("Copy", "Ctrl+C"));
    if (COMMON_STR_TRASH == m_viewType) {
//        appendAction(IdMoveToTrash, tr("Delete"), ss("Throw to trash", "Delete"));
    } else {
        appendAction(IdMoveToTrash, tr("Delete"), ss("Throw to trash", "Delete"));
    }

    if (utils::common::VIEW_ALLPIC_SRN != m_viewType
            && utils::common::VIEW_TIMELINE_SRN != m_viewType
            && utils::common::VIEW_SEARCH_SRN != m_viewType
            && COMMON_STR_RECENT_IMPORTED != m_viewType
            && COMMON_STR_TRASH != m_viewType
            && COMMON_STR_FAVORITES != m_viewType) {
        appendAction(IdRemoveFromAlbum, tr("Remove from album"), ss("Remove from album", ""));
    }
    m_menu->addSeparator();
    /**************************************************************************/
#if 1
    if (DBManager::instance()->isImgExistInAlbum(COMMON_STR_FAVORITES, m_infos.at(m_current).filePath)) {
        appendAction(IdRemoveFromFavorites, tr("Unfavorite"), ss("Unfavorite", "Ctrl+Shift+K"));    //取消收藏
    } else {
        appendAction(IdAddToFavorites, tr("favorite"), ss("favorite", "Ctrl+K"));       //收藏
    }

    m_menu->addSeparator();
#endif
    if (! m_viewB->isWholeImageVisible() && m_nav->isAlwaysHidden()) {
        appendAction(IdShowNavigationWindow,
                     tr("Show navigation window"), ss("Show navigation window", ""));
    } else if (! m_viewB->isWholeImageVisible() && !m_nav->isAlwaysHidden()) {
        appendAction(IdHideNavigationWindow,
                     tr("Hide navigation window"), ss("Hide navigation window", ""));
    }
    /**************************************************************************/
    if (utils::image::imageSupportSave(m_infos.at(m_current).filePath)) {
        m_menu->addSeparator();
        if (QFileInfo(m_infos.at(m_current).filePath).isReadable() &&
                !QFileInfo(m_infos.at(m_current).filePath).isWritable()) {


            appendAction_darkmenu(IdRotateClockwise,
                                  tr("Rotate clockwise"), ss("Rotate clockwise", "Ctrl+R"));
            appendAction_darkmenu(IdRotateCounterclockwise,
                                  tr("Rotate counterclockwise"), ss("Rotate counterclockwise", "Ctrl+Shift+R"));
        } else {
            appendAction(IdRotateClockwise,
                         tr("Rotate clockwise"), ss("Rotate clockwise", "Ctrl+R"));
            appendAction(IdRotateCounterclockwise,
                         tr("Rotate counterclockwise"), ss("Rotate counterclockwise", "Ctrl+Shift+R"));
        }
    }
    /**************************************************************************/
    if (utils::image::imageSupportSave(m_infos.at(m_current).filePath))  {
        appendAction(IdSetAsWallpaper,
                     tr("Set as wallpaper"), ss("Set as wallpaper", "Ctrl+F8"));
    }


    appendAction(IdDisplayInFileManager, tr("Display in file manager"), ss("Display in file manager", "Ctrl+D"));
    appendAction(IdImageInfo, tr("Photo info"), ss("Photo info", "Alt+Return"));
}
#if 1
QMenu *ViewPanel::createAblumMenu()
{
    QMenu *am = new QMenu(tr("Add to album"));

    QStringList albums = DBManager::instance()->getAllAlbumNames();
    albums.removeAll(COMMON_STR_FAVORITES);
    albums.removeAll(COMMON_STR_TRASH);
    albums.removeAll(COMMON_STR_RECENT_IMPORTED);

    QAction *ac = new QAction(am);
    ac->setProperty("MenuID", IdAddToAlbum);
    ac->setText(tr("New Album"));
    ac->setData("Add to new album");
    ac->setShortcut(QKeySequence("Ctrl+Shift+N"));
    am->addAction(ac);
    am->addSeparator();

    for (QString album : albums) {
        QAction *ac = new QAction(am);
        ac->setProperty("MenuID", IdAddToAlbum);
        ac->setText(fontMetrics().elidedText(QString(album).replace("&", "&&"), Qt::ElideMiddle, 200));
        ac->setData(album);
        am->addAction(ac);
    }

    return am;
}
#endif

void ViewPanel::initShortcut()
{
    // Delay image toggle
    QTimer *dt = new QTimer(this);
    dt->setSingleShot(true);
    dt->setInterval(SWITCH_IMAGE_DELAY);
    QShortcut *sc = nullptr;
    // Previous
    sc = new QShortcut(QKeySequence(Qt::Key_Left), this);
    sc->setContext(Qt::WindowShortcut);
    connect(sc, &QShortcut::activated, this, [ = ] {
        if (! dt->isActive())
        {
            dt->start();
            showPrevious();
        }
    });
    // Next
    sc = new QShortcut(QKeySequence(Qt::Key_Right), this);
    sc->setContext(Qt::WindowShortcut);
    connect(sc, &QShortcut::activated, this, [ = ] {
        if (! dt->isActive())
        {
            dt->start();
            showNext();
        }
    });

    // Zoom out (Ctrl++ Not working, This is a confirmed bug in Qt 5.5.0)
    connect(dApp->signalM, &SignalManager::sigCtrlADDKeyActivated, this, [ = ] {
        m_viewB->setScaleValue(1.1);
    });

    connect(dApp->signalM, &SignalManager::sigCtrlSubtractKeyActivated, this, [ = ] {
        m_viewB->setScaleValue(0.9);
    });

//    sc = new QShortcut(QKeySequence(Qt::Key_Up), this);
//    sc->setContext(Qt::WindowShortcut);
//    connect(sc, &QShortcut::activated, this, [=] {
//        qDebug() << "Qt::Key_Up:";
//        m_viewB->setScaleValue(1.1);
//    });
//    sc = new QShortcut(QKeySequence("Ctrl+="), this);
//    sc->setContext(Qt::WidgetShortcut);
//    connect(sc, &QShortcut::activated, this, [=] {
//        m_viewB->setScaleValue(1.1);
//    });
    // Zoom in
//    sc = new QShortcut(QKeySequence(Qt::Key_Down), this);
//    sc->setContext(Qt::WindowShortcut);
//    connect(sc, &QShortcut::activated, this, [=] {
//        qDebug() << "Qt::Key_Down:";
//        m_viewB->setScaleValue(0.9);
//    });
//    sc = new QShortcut(QKeySequence("Ctrl+-"), this);
//    sc->setContext(Qt::WindowShortcut);
//    connect(sc, &QShortcut::activated, this, [=] {
//        m_viewB->setScaleValue(0.9);
//    });
    // Esc
//    QShortcut *esc = new QShortcut(QKeySequence(Qt::Key_Escape), this);
//    esc->setContext(Qt::WindowShortcut);
//    connect(esc, &QShortcut::activated, this, [=] {
//        if (window()->isFullScreen()) {
//            toggleFullScreen();
//        }
//        else {
//            if (m_vinfo.inDatabase) {
//                backToLastPanel();
//            }
//            else {
//                dApp->quit();
//            }
//        }
//        emit dApp->signalM->hideExtensionPanel(true);
//    });
    //1:1 size
    QShortcut *adaptImage = new QShortcut(QKeySequence("Ctrl+0"), this);
    adaptImage->setContext(Qt::WindowShortcut);
    connect(adaptImage, &QShortcut::activated, this, [ = ] {
        m_viewB->fitImage();
    });
}

void ViewPanel::popupDelDialog(const QString path)
{
#ifndef LITE_DIV
    const QStringList paths(path);
    FileDeleteDialog *fdd = new FileDeleteDialog(paths);
    fdd->showInCenter(window());
#else
    Q_UNUSED(path)
#endif
}
