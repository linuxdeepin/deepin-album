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
#ifndef ALLPICVIEW_H
#define ALLPICVIEW_H

#include "application.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "controller/configsetter.h"
#include "controller/signalmanager.h"
#include "dbmanager/dbmanager.h"
#include "thumbnail/thumbnaillistview.h"
#include "importview/importview.h"
#include "searchview/searchview.h"
#include "widgets/statusbar.h"
#include "widgets/statusbar.h"

#include <QWidget>
#include <QVBoxLayout>
#include <DLabel>
#include <QPixmap>
#include <QStandardPaths>
#include <QImageReader>
#include <DPushButton>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <DStackedWidget>
#include <DSlider>
#include <DSpinner>
#include <DCommandLinkButton>

DWIDGET_USE_NAMESPACE

class ImageViewer;
class NoResultWidget;
class BatchOperateWidget;
class AllPicView : public DWidget, public ImageEngineImportObject
{
    Q_OBJECT

public:
    AllPicView();

    bool imageImported(bool success) override;
    void restorePicNum();
    void updatePicNum();
    const ThumbnailListView *getAllPicThumbnailListViewModel();

private:
    void initConnections();
    //初始化悬浮框
    void initSuspensionWidget();
    void initStackedWidget();
    void updatePicsIntoThumbnailView();
    void onUpdateAllpicsNumLabel();
    void onKeyDelete();

    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

public slots:
    //筛选显示，当先列表中内容为无结果
    void slotNoPicOrNoVideo(bool isNoResult);
    void updateStackedWidget();
    // 监控到新文件
    void monitorHaveNewFile(QStringList list);

private slots:
    //所有照片

    void updatePicsIntoThumbnailViewWithCache();
    void onRepeatImportingTheSamePhotos(QStringList importPaths, QStringList duplicatePaths, const QString &albumName);
    //打开图片
    void onOpenImage(int row, const QString &path, bool bFullScreen);
    //幻灯片播放
    void onSlideShow(const QString &path);
    void onImportViewImportBtnClicked();
    void onImportFailedToView();
    //图片刷新后更新
    void onImgRemoved(const DBImgInfoList &infos);
    //缩略图选中项改变
    void sltSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

public:
    DStackedWidget *m_pStackedWidget = nullptr;
    StatusBar *m_pStatusBar = nullptr;
    QWidget *m_pwidget = nullptr;
    ImportView *m_pImportView = nullptr;
    int step;
    const static int SUSPENSION_WIDGET_HEIGHT = 40;//悬浮控件高度

private:
    DWidget *m_thumbnailListViewWidget = nullptr;
    ThumbnailListView *m_pThumbnailListView = nullptr;
    NoResultWidget *m_noResultWidget = nullptr;
    SearchView *m_pSearchView = nullptr;
//    DSpinner *m_spinner;暂时废弃控件
    DWidget *fatherwidget = nullptr;
    //悬浮时间栏控件
    QWidget *m_SuspensionWidget = nullptr;
    BatchOperateWidget *m_batchOperateWidget = nullptr;
    DCommandLinkButton *m_suspensionChose = nullptr;
//    ImageViewer *m_ImageViewer = nullptr;
public:
    ThumbnailListView *getThumbnailListView();
    /**
     * @brief updatePicsIntoThumbnailView 针对单个或多个图片刷新
     * @param strpath 图片路径队列
     */
    void updatePicsThumbnailView(QStringList strpath = QStringList());
};

#endif // ALLPICVIEW_H
