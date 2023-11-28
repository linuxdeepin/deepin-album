// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SIGNALMANAGER_H
#define SIGNALMANAGER_H

#include "dbmanager/dbmanager.h"
#include <QObject>
#include <dgiomount.h>
#include <thumbnail/thumbnaillistview.h>

class SignalManager : public QObject
{
    Q_OBJECT
public:
    static SignalManager *instance();

    // For view images
    struct ViewInfo {
        static constexpr bool inDatabase = false;
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

    void showInfoDlg(const QString &path, ItemType type, bool isTrash);

    static std::atomic_bool inAutoImport;

signals:
    void hideImageView();
    void showSlidePanel(int index);
    void hideSlidePanel();
    void extensionPanelHeight(int height, bool immediately = false);
    void sendPathlist(QStringList pathlist);

    void imagesInserted(/*const DBImgInfoList infos*/);
    void imagesRemoved();
    void imagesRemovedPar(const QStringList &paths);
    void imagesTrashInserted(/*const DBImgInfoList infos*/);
    void imagesTrashRemoved(/*const DBImgInfoList &infos*/);
    void startSlideShow(const ViewInfo &vinfo);

    void sigViewImage(const ViewInfo &info, OpenImgAdditionalOperation operation, bool isCustom = false, const QString &album = "", int UID = -1);

    void exportImage(const QStringList &paths);
    void sigAlbDelToast(const QString &albname);
    void sigAddToAlbToast(const QString &album);
    void sigAddDuplicatePhotos();
    void updateStatusBarImportLabel(const QStringList &paths, int count, QString album = "");
    void ImportSuccess();
    void SearchEditClear();
    void ImportFailed();
    void ImportInterrupted();
    void ImportSomeFailed(int successful, int failed);
    void ImportDonotFindPicOrVideo();
    void RepeatImportingTheSamePhotos(QStringList importPaths, QStringList duplicatePaths, int UID);
    void ImgExportSuccess();
    void ImgExportFailed();
    void AlbExportSuccess();
    void AlbExportFailed();
    void DeviceImageLoadEnd();
    void sigExporting(const QString &path);
    void sigRestoreStatus();

    // 图片分类已完成信号
    void sigImageClassifyDone();
    // 打开分类详情页
    void sigOpenClassDetail(const QString &className);

    // Handle by album
    void createAlbum(QStringList imgPath = QStringList());
    void viewCreateAlbum(QString path, bool bmodel = true);
    void sigCreateNewAlbumFrom(const QString &albumname, int UID);
    void insertedIntoAlbum(int UID, const QStringList &paths);
    void removedFromAlbum(int UID, const QStringList &paths);
    void sigSendKeywordsIntoALLPic(QString keywords, QString album = nullptr, int UID = -1, const QString& className = "");
    void sigCreateNewAlbumFromDialog(const QString &albumname, int UID);
    void sigMainwindowSliderValueChg(int value);
//    void sigESCKeyActivated();
//    void sigESCKeyStopSlide();
    void sigUpdataAlbumRightTitle(const QString &titlename);
    void sigUpdateTrashImageLoader();
    void sigUpdateImageLoader(QStringList pathlist = QStringList());
    void sigLoadMountImagesEnd(QString mountname);
    void sigLoadOnePhoto();
    void sigImportFailedToView();
    void sigShortcutKeyDelete();
    void sigShortcutKeyF2();
    void sigRestoreFailed(const QStringList &);

    void startImprot();
    void popupWaitDialog(QString waittext, bool bneedprogress = true);
    void closeWaitDialog();
    void progressOfWaitDialog(int allfiles, int completefiles);
    void waitDevicescan();

    //lmh0426设备退出，信号通知线程退出
    void sigDevStop(QString devName);

    //对于外设后台加载是否暂停，在查看界面使用  -xiaolong
    void sigPauseOrStart(bool bpause);

    //监控到改变
    void sigMonitorChanged(QStringList fileAdd, QStringList fileDelete, QString album, int UID);

    //自动导入路径被删除
    void sigMonitorDestroyed(int UID);

    //需要重新加载缩略图
    void needReflushThumbnail(const QStringList &paths);

private:
    explicit SignalManager(QObject *parent = nullptr);

private:
    static SignalManager *m_signalManager;

    int m_sliderValue;
};

#endif // SIGNALMANAGER_H
