// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

//#include "application.h"
//#include "controller/signalmanager.h"
//#include "dbmanager/dbmanager.h"
#include "widgets/thumbnail/thumbnaillistview.h"
//#include "importview/importview.h"
//#include "searchview/searchview.h"
//#include "widgets/statusbar.h"
//#include "allpicview/allpicview.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <DListWidget>
#include <DStackedWidget>
#include <DCommandLinkButton>
#include <DGuiApplicationHelper>
#include <QGraphicsOpacityEffect>
#include <QQuickPaintedItem>

class NoResultWidget;
class BatchOperateWidget;
class TimeLineView : public DWidget
{
    friend class QmlWidget;
    Q_OBJECT

public:
    TimeLineView(QQuickPaintedItem *parent = nullptr);
    ~TimeLineView() override
    {
    }

    bool imageImported(bool success)/* override*/;

    void updateStackedWidget();
    void updatePicNum();
    void updateChoseText();
    void restorePicNum();
    void themeChangeSlot(DGuiApplicationHelper::ColorType themeType);
    ThumbnailListView *getThumbnailListView();
    //tab进入时清除其他所有选中
    void clearAllSelection();

public slots:
    void on_AddLabel(QString date, QString num);
    void onCheckBoxClicked();
    void clearAndStartLayout();
    //更新最上当悬浮标题时间与数量
    void slotTimeLineDataAndNum(QString data, QString num, QString text);

protected:
    void resizeEvent(QResizeEvent *ev) override;
    QPaintEngine * paintEngine() const override;
    void paintEvent(QPaintEvent * event) override;
private:
    void initTimeLineViewWidget();
    void initConnections();
    void addTimelineLayout();
    void onKeyDelete();
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;

public slots:
    //筛选显示，当先列表中内容为无结果
    void slotNoPicOrNoVideo(bool isNoResult);
    //更新布局（旋转图片时）
    void updataLayout(QStringList updatePathList);
    void onImportViewImportBtnClicked();
    void onImportFailedToView();
    void onRepeatImportingTheSamePhotos(QStringList importPaths, QStringList duplicatePaths, int UID);
    //打开图片
    void onOpenImage(int row, const QString &path, bool bFullScreen);
    //幻灯片播放
    void onSlideShow(QString path);

    //进入批量状态
    void slotBatchSelectChanged(bool isBatchSelect);
    //缩略图选中项改变
    void sltSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    void onShowCheckBox(bool bShow);
private:
    QLayout *m_mainLayout;
    QList<QString> m_timelines;
    //悬浮时间栏控件
    QWidget *m_dateNumItemWidget = nullptr;
    BatchOperateWidget *m_batchOperateWidget = nullptr;
    DCommandLinkButton *m_suspensionChose = nullptr;
    DLabel *m_dateLabel = nullptr;
    DLabel *m_numLabel = nullptr;
    DCheckBox *m_numCheckBox = nullptr;

    int allnum;
    ThumbnailListView *m_timeLineThumbnailListView = nullptr;//时间线缩略图列表，含时间项
    QGraphicsOpacityEffect *m_oe;
    QGraphicsOpacityEffect *m_oet;
    QWidget *fatherwidget;

public:
    QStackedWidget *m_pStackedWidget;
    NoResultWidget *m_noResultWidget = nullptr;
//    StatusBar *m_pStatusBar;
//    SearchView *pSearchView;
//    ImportView *pImportView;
    QWidget *pTimeLineViewWidget;
    QWidget *m_pwidget;
    int m_selPicNum;
//    DSpinner *m_spinner;

    QQuickPaintedItem *m_qquickContainer;
};

#endif // TIMELINEVIEW_H
