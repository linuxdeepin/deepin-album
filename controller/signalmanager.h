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
#ifndef SIGNALMANAGER_H
#define SIGNALMANAGER_H

#include "dbmanager/dbmanager.h"
#include <QObject>
#include <dgiomount.h>
#include <widgets/thumbnaillistview.h>

class ModulePanel;
class SignalManager : public QObject
{
    Q_OBJECT
public:
    static SignalManager *instance();

    // For view images
    struct ViewInfo {
        ModulePanel *lastPanel;                 // For back to the last panel
#ifndef LITE_DIV
        bool inDatabase = true;
#else
        static constexpr bool inDatabase = false;
#endif
        bool fullScreen = false;
        bool slideShow = false;
        int viewMainWindowID = 0;               // 0:all picture    1:time line     2:album   4:view
        QString album = QString();
        QString path;                           // Specific current open one
        QStringList paths = QStringList();      // Limit the view range
        QString viewType = QString();
    };

signals:
    void enableMainMenu(bool enable);
    void updateTopToolbarLeftContent(QWidget *content);
    void updateTopToolbarMiddleContent(QWidget *content);
    void updateBottomToolbarContent(QWidget *content, bool wideMode = false);
    void updateTopToolbar();
    void updateBottomToolbar(bool wideMode = false);
    void updateExtensionPanelContent(QWidget *content);
    void showTopToolbar();
    void hideTopToolbar(bool immediately = false);
    void showBottomToolbar();
    void hideBottomToolbar(bool immediately = false);
    void showExtensionPanel();
    void hideExtensionPanel(bool immediately = false);

    void showImageView(int index);
    void hideImageView();
	void extensionPanelHeight(int height, bool immediately = false);
    void sendPathlist(QStringList pathlist);
    void enterView(bool immediately = false);

    void gotoTimelinePanel();
    void gotoSearchPanel(const QString &keyWord = "");
    void gotoPanel(ModulePanel* panel);
    void backToMainPanel();
    void activeWindow();

    void imagesInserted(const DBImgInfoList infos);
    void imagesRemoved(const DBImgInfoList &infos);
    void imagesTrashInserted(const DBImgInfoList infos);
    void imagesTrashRemoved(const DBImgInfoList &infos);
    void editImage(const QString &path);
    void showImageInfo(const QString &path);
    void showInFileManager(const QString &path);
    void startSlideShow(const ViewInfo &vinfo, bool inDB = true);
    void viewImage(const ViewInfo &vinfo);
    void exportImage(const QStringList &paths);
    void updateFileName(const QString &fileName);
    void resizeFileName();
    void sigAlbDelToast(const QString &albname);
    void sigAddToAlbToast(const QString &album);
    void updateStatusBarImportLabel(const QStringList paths, int count);
    void updateIcon();
    void ImportSuccess();
    void SearchEditClear();
    void TransmitAlbumName(const QString &name);
    void ImportFailed();
    void ImgExportSuccess();
    void ImgExportFailed();
    void AlbExportSuccess();
    void AlbExportFailed();
    void sigExporting(const QString &path);
    void sigRestoreStatus();

    // Handle by album
    void gotoAlbumPanel(const QString &album = "");
    void createAlbum(QStringList imgPath = QStringList());
#if 1
    void viewModeCreateAlbum(QString path);
    void sigCreateNewAlbumFrom(QString albumname);
#endif
    void importDir(const QString &dir);
    void insertedIntoAlbum(const QString &album, const QStringList &paths);
    void removedFromAlbum(const QString &album, const QStringList &paths);
    void sigSendKeywordsIntoALLPic(QString keywords, QString album = nullptr);
    void sigCreateNewAlbumFromDialog(QString albumname);
    void sigMainwindowSliderValueChg(int value);
    void sigMouseMove();
    void sigShowFullScreen();
    void sigESCKeyActivated();
    void sigUpdataAlbumRightTitle(QString titlename);
	void sigUpdateTrashImageLoader();
	void sigUpdateImageLoader();
    void sigLoadMountImagesEnd(QString mountname);
    void sigCtrlADDKeyActivated();
    void sigCtrlSubtractKeyActivated();

private:
    explicit SignalManager(QObject *parent = 0);

private:
    static SignalManager *m_signalManager;
};

#endif // SIGNALMANAGER_H
