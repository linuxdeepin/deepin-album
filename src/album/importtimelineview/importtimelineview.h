// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IMPORTTIMELINEVIEW_H
#define IMPORTTIMELINEVIEW_H

#include "application.h"
#include "controller/signalmanager.h"
#include "dbmanager/dbmanager.h"
#include "thumbnail/thumbnaillistview.h"
#include "importview/importview.h"
#include "widgets/statusbar.h"
#include "allpicview/allpicview.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QGraphicsOpacityEffect>

#include <DListWidget>
#include <DCommandLinkButton>
#include <DApplicationHelper>

class BatchOperateWidget;
class ImportTimeLineView : public DWidget, public ImageEngineImportObject
{
    Q_OBJECT
public:
    explicit ImportTimeLineView(DWidget *parent);
    ~ImportTimeLineView() override
    {
        void clearAndStop();
    }

    bool imageImported(bool success) override;

    int getIBaseHeight();
public slots:
    //更新最上当悬浮标题时间与数量
    void slotTimeLineDataAndNum(QString data, QString num, QString text);
    //打开图片
    void onOpenImage(int row, const QString &path, bool bFullScreen);
    //幻灯片播放
    void onSlideShow(QString path);

    void clearAndStartLayout();
protected:
    void resizeEvent(QResizeEvent *ev) override;
    void showEvent(QShowEvent *ev) override;

private:
    void initTimeLineViewWidget();
    void initConnections();
    void updateDateNumLabel();

    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
//    void dragLeaveEvent(QDragLeaveEvent *e) override;
//    void keyPressEvent(QKeyEvent *e) override;
//    void keyReleaseEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;

public:
    void addTimelineLayout();
    void getFatherStatusBar(DSlider *s);
    void themeChangeSlot(DGuiApplicationHelper::ColorType themeType);
    ThumbnailListView *getListView();
    //tab进入时清除其他所有选中
    void clearAllSelection();
#if 1
    QStringList selectPaths();
#endif
private slots:
    /**
     * @brief updateSize
     * 调整已导入界面的整体大小
     */
    void updateSize();
    void onRepeatImportingTheSamePhotos(QStringList importPaths, QStringList duplicatePaths, int UID);
    void onSuspensionChoseBtnClicked();
    //进入批量状态
    void slotBatchSelectChanged(bool isBatchSelect);
    //筛选显示，当先列表中内容为无结果
    void slotNoPicOrNoVideo(bool isNoResult);
    //响应删除
    void onDelete();
signals:
    void sigUpdatePicNum();
    //筛选显示图片或者视频，无结果时
    void sigNoPicOrNoVideo(bool isNoResult);//1050
public:
    const static int title_HEIGHT = 60;
    const static int ChoseBtn_HEIGHT = 32;
private:
    QLayout *m_mainLayout;
    QList<QDateTime> m_timelines;
    DWidget *m_choseBtnItem = nullptr;
    DWidget *m_TitleItem;                               //title包裹窗口
    DCommandLinkButton *m_suspensionChoseBtn = nullptr;  //悬浮选择按钮
    DWidget *m_timeLineViewWidget = nullptr;
    BatchOperateWidget *m_batchOperateWidget = nullptr;
    DLabel *m_DateNumLabel = nullptr;                   //已导入悬浮日期数量
    QString dateFullStr;
    QString numFullStr;
    DLabel *m_pImportTitle = nullptr;                   //已导入悬浮标题
    DSlider *m_DSlider;
    ThumbnailListView *m_importTimeLineListView = nullptr;
    NoResultWidget *m_noResultWidget = nullptr;
    QGraphicsOpacityEffect *m_oe;
    QGraphicsOpacityEffect *m_oet;

    bool m_ctrlPress;
};

#endif // IMPORTTIMELINEVIEW_H
