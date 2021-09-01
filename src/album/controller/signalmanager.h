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
#ifndef SIGNALMANAGER_H
#define SIGNALMANAGER_H

#include "dbmanager/dbmanager.h"
#include <QObject>
#include <dgiomount.h>
#include <thumbnail/thumbnaillistview.h>

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
        QList<DBImgInfo> dBImgInfos;
        QString viewType = QString();
    };

    void emitSliderValueChg(int value);
    int getSliderValue();
signals:
    void enableMainMenu(bool enable);
    void updateTopToolbarLeftContent(QWidget *content);
    void updateTopToolbarMiddleContent(QWidget *content);
    void updateBottomToolbarContent(QWidget *content, bool wideMode = false);
    void updateTopToolbar();
    void updateBottomToolbar(bool wideMode = false);
    void updateExtensionPanelContent(QWidget *content);
    void updatePicView(int i);
    void showTopToolbar();
    void hideTopToolbar(bool immediately = false);
    void showBottomToolbar();
    void hideBottomToolbar(bool immediately = false);
    void showExtensionPanel();
    void hideExtensionPanel(bool immediately = false);

    void showImageView(int index);
    void hideImageView();
    void showSlidePanel(int index);
    void hideSlidePanel();
    void extensionPanelHeight(int height, bool immediately = false);
    void sendPathlist(QStringList pathlist);
    void enterView(bool immediately = false);
    void enterScaledMode(bool immediately = false);

    void gotoTimelinePanel();
    void gotoSearchPanel(const QString &keyWord = "");
    void gotoPanel(ModulePanel *panel);
    void backToMainPanel();
    void activeWindow();

    void imagesInserted(/*const DBImgInfoList infos*/);
    void imagesRemoved();
    void imagesRemovedPar(const DBImgInfoList &infos);
    void imagesTrashInserted(/*const DBImgInfoList infos*/);
    void imagesTrashRemoved(/*const DBImgInfoList &infos*/);
    void editImage(const QString &path);
    void showImageInfo(const QString &path);
    void showInFileManager(const QString &path);
    void startSlideShow(const ViewInfo &vinfo, bool inDB = true);
    void viewImage(const ViewInfo &vinfo);
    void viewImageNoNeedReload(int &fileindex);
    void exportImage(const QStringList &paths);
    void updateFileName(const QString &fileName);
    void resizeFileName();
    void sigAlbDelToast(const QString &albname);
    void sigAddToAlbToast(const QString &album);
    void sigAddDuplicatePhotos();
    void updateStatusBarImportLabel(const QStringList &paths, int count, QString album = "");
    void updateIcon();
    void ImportSuccess();
    void SearchEditClear();
    void TransmitAlbumName(const QString &name);
    void ImportFailed();
    void ImportSomeFailed(int successful, int failed);
    void ImportDonotFindPicOrVideo();
    void RepeatImportingTheSamePhotos(QStringList importPaths, QStringList duplicatePaths, const QString &albumName);
    void ImgExportSuccess();
    void ImgExportFailed();
    void AlbExportSuccess();
    void AlbExportFailed();
    void DeviceImageLoadEnd();
    void sigExporting(const QString &path);
    void sigRestoreStatus();

    void updateButton();
    void sigStartTimer();
    void updatePauseButton();
    void initSlideShowButton();

    void updateFavoriteNum();

    // Handle by album
    void gotoAlbumPanel(const QString &album = "");
    void createAlbum(QStringList imgPath = QStringList());

    void sigDrawingBoard(QStringList Paths);//lmh0407
#if 1
    void viewCreateAlbum(QString path, bool bmodel = true);
    void sigCreateNewAlbumFrom(const QString &albumname);
#endif
    void importDir(const QString &dir);
    void insertedIntoAlbum(const QString &album, const QStringList &paths);
    void removedFromAlbum(const QString &album, const QStringList &paths);
    void sigSendKeywordsIntoALLPic(QString keywords, QString album = nullptr);
    void sigCreateNewAlbumFromDialog(const QString &albumname);
    void sigMainwindowSliderValueChg(int value);
    void sigMouseMove(bool show);
    void sigShowFullScreen();
    void sigESCKeyActivated();
    void sigESCKeyStopSlide();
    void sigUpdataAlbumRightTitle(const QString &titlename);
    void sigUpdateTrashImageLoader();
    void sigUpdateImageLoader(QStringList pathlist = QStringList());
    void sigLoadMountImagesEnd(QString mountname);
    void sigCtrlADDKeyActivated();
    void sigCtrlSubtractKeyActivated();
    void sigDeletePhotos(int num);
//    void trashDelete();
    void sigLoadOnePhoto();
    void sigImportFailedToView();
    void sigShortcutKeyDelete();
    void sigShortcutKeyF2();
    void picNotExists(bool immediately = false);

    void deleteByMenu();
    void startImprot();
    void popupWaitDialog(QString waittext, bool bneedprogress = true);
    void closeWaitDialog();
    void progressOfWaitDialog(int allfiles, int completefiles);
    void waitDevicescan();
    void cacheThreadStop();
    void sigSyncListviewModelData(QStringList paths, QString albumName, int actionType);

    //lmh0426设备退出，信号通知线程退出
    void sigDevStop(QString devName);

    //对于外设后台加载是否暂停，在查看界面使用  -xiaolong
    void sigPauseOrStart(bool bpause);

    //监控到改变
    void sigMonitorChanged(QStringList newfile);

private:
    explicit SignalManager(QObject *parent = nullptr);

private:
    static SignalManager *m_signalManager;

    int m_sliderValue;
};

#endif // SIGNALMANAGER_H
