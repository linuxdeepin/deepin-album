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
#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include "application.h"
#include "controller/signalmanager.h"
#include "dbmanager/dbmanager.h"
#include "thumbnail/thumbnaillistview.h"
#include "widgets/timelinelist.h"
#include "widgets/timelineitem.h"
#include "importview/importview.h"
#include "searchview/searchview.h"
#include "widgets/statusbar.h"
#include "allpicview/allpicview.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <DListWidget>
#include <DStackedWidget>
#include <DCommandLinkButton>
#include <DApplicationHelper>
#include <QGraphicsOpacityEffect>

class Title : public QWidget
{
public:
    Title() {}
protected:
    void paintEvent(QPaintEvent *event)
    {
        Q_UNUSED(event);
        qDebug() << "x is " << x();
        qDebug() << "pos.x is " << pos().x();
    }
    void moveEvent(QMoveEvent *event)
    {
        Q_UNUSED(event);
        qDebug() << "moveEvent x is " << x();
        qDebug() << "moveEvent pos.x is " << pos().x();
    }
};

class TimeLineView : public DWidget, public ImageEngineImportObject
{
public:
    TimeLineView();
    ~TimeLineView() override
    {
        clearAndStop();
    }

    bool imageImported(bool success) override
    {
        Q_UNUSED(success);
        emit dApp->signalM->closeWaitDialog();
        return true;
    }

    void updateStackedWidget();
    int getIBaseHeight();
    void updatePicNum();
    void updateChoseText();
    void restorePicNum();
    void themeChangeSlot(DGuiApplicationHelper::ColorType themeType);
    ThumbnailListView *getFirstListViewFromTimeline();
    //tab进入时清除其他所有选中
    void clearAllSelection();

public slots:
    void on_AddLabel(QString date, QString num);
//    void on_DelLabel();//未使用
    void on_DCommandLinkButton();
    void on_GetSelectedPaths(QStringList *pPaths);
#if 1
    void on_MoveLabel(int y, const QString &date, const QString &num, const QString &choseText);
#endif
    void on_KeyEvent(int key);
    void on_MoveScroll(int distance);

protected:
    void resizeEvent(QResizeEvent *ev) override;

private:
    void initTimeLineViewWidget();
    void initConnections();
    void sigImprotPicsIntoThumbnailView();
    void getImageInfos();
    void clearAndStop();
    void clearAndStartLayout();
    void addTimelineLayout();
    void initMainStackWidget();
    void onKeyDelete();
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
//    void keyPressEvent(QKeyEvent *e) override;
//    void keyReleaseEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;

public slots:
    //更新布局（旋转图片时）
    void updataLayout(QStringList updatePathList);
    void onFinishLoad();
    void onNewTime(const QString &date, const QString &num, int index);
    void onImportViewImportBtnClicked();
    void onImportFailedToView();
    void onRepeatImportingTheSamePhotos(QStringList importPaths, QStringList duplicatePaths, const QString &albumName);

private:
    TimelineListWidget *m_mainListWidget;
    QLayout *m_mainLayout;
    QList<QString> m_timelines;
    QWidget *m_dateItem;
    DCommandLinkButton *pSuspensionChose;

    int allnum;
    DLabel *m_pDate;
    DLabel *pNum_up;
    DLabel *pNum_dn;
    QList<ThumbnailListView *> m_allThumbnailListView;
    QList<DCommandLinkButton *> m_allChoseButton;
    QGraphicsOpacityEffect *m_oe;
    QGraphicsOpacityEffect *m_oet;
    bool m_ctrlPress;
    int lastClickedIndex;
    int lastRow;
    bool lastChanged;
    QWidget *fatherwidget;

public:
    QStackedWidget *m_pStackedWidget;
    StatusBar *m_pStatusBar;
    SearchView *pSearchView;
    ImportView *pImportView;
    QWidget *pTimeLineViewWidget;
    QWidget *m_pwidget;
    int m_index;
    int m_selPicNum;
    DSpinner *m_spinner;
    int currentTimeLineLoad;
    QString selectPrePaths = "";//跳转的上一图片位置
    int hasPicView = -1;//包含跳转图片的view索引
    bool isFindPic = false;
};

#endif // TIMELINEVIEW_H
