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
#ifndef SEARCHVIEW_H
#define SEARCHVIEW_H

#include "application.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "controller/configsetter.h"
#include "controller/signalmanager.h"
#include "dbmanager/dbmanager.h"
#include "thumbnail/thumbnaillistview.h"


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
#include <DWidget>
#include <DGuiApplicationHelper>
#include <QMouseEvent>
#include <DBlurEffectWidget>

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE

class SlideShowButton : public DPushButton
{
    Q_OBJECT
public:
    explicit SlideShowButton(DWidget *parent = nullptr);
    void mSetText(QString text);
protected:
    void paintEvent(QPaintEvent *event) override;
    void  enterEvent(QEvent *e) override;
    void  leaveEvent(QEvent *e) override;
    void  mouseEvent(QMouseEvent *e);
    void  mousePressEvent(QMouseEvent *event) override;
    void  mouseReleaseEvent(QMouseEvent *event) override;

private:
    qreal m_filletradii;
    bool  israised;
    bool ispressed = false;
};

class SearchView : public QWidget
{
    Q_OBJECT

public:
    SearchView();
    void improtSearchResultsIntoThumbnailView(QString s, const QString &album);

public slots:
    void onSlideShowBtnClicked();
    //打开图片
    void onOpenImage(int row, const QString &path, bool bFullScreen);
    //幻灯片播放
    void onSlideShow(const QString &path);
    void onFinishLoad();

private:
    void initConnections();
    void initNoSearchResultView();
    void initSearchResultView();
    void initMainStackWidget();
    void updateSearchResultsIntoThumbnailView();
    void changeTheme();
    void onKeyDelete();
    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

private:
    DStackedWidget *m_stackWidget;
    DWidget *m_pNoSearchResultView;
    DLabel *m_pNoSearchResultLabel;
    DWidget *m_pSearchResultWidget = nullptr;
    DWidget *m_searchResultViewbody;
    DWidget *m_searchResultViewTop;
    SlideShowButton *m_pSlideShowBtn;
    DLabel *m_pSearchResultLabel;
    QString m_keywords;
    DLabel *pNoResult;
    DLabel *pLabel1;
    QString m_albumName;
    int m_currentFontSize;
public:
    int m_searchPicNum;
    ThumbnailListView *m_pThumbnailListView;
};

#endif // SEARCHVIEW_H
