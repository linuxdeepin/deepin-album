// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

class SearchView : public QWidget
{
    Q_OBJECT

public:
    SearchView();
    void improtSearchResultsIntoThumbnailView(QString s, const QString &album, int UID, const QString& className);

public slots:
    void onSlideShowBtnClicked();
    //打开图片
    void onOpenImage(int row, const QString &path, bool bFullScreen);
    //幻灯片播放
    void onSlideShow(const QString &path);

private:
    void initConnections();
    void initNoSearchResultView();
    void initSearchResultView();
    void initMainStackWidget();
    void updateSearchResultsIntoThumbnailView();
    void changeTheme();
    void setSliderShowBtnEnable(bool enabled);
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
    DPushButton *m_pSlideShowBtn;
    DLabel *m_pSearchResultLabel;
    QString m_keywords;
    DLabel *pNoResult;
    DLabel *pLabel1;
    QString m_albumName;
    int m_UID;
    QString m_className;
    int m_currentFontSize;
public:
    int m_searchPicNum;
    ThumbnailListView *m_pThumbnailListView;
};

#endif // SEARCHVIEW_H
