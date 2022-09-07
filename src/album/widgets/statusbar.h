// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QWidget>
#include <DLabel>
#include <DSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <DFontSizeManager>
#include <QStackedWidget>
#include <DSpinner>
#include <DBlurEffectWidget>

#include "application.h"
#include "controller/signalmanager.h"
#include "dbmanager/dbmanager.h"
#include "thumbnail/thumbnaillistview.h"

DWIDGET_USE_NAMESPACE


class StatusBar : public DBlurEffectWidget
{
    Q_OBJECT

public:
    explicit StatusBar(QWidget *parent = nullptr);

public:
    void initUI();
    void initConnections();
    void onUpdateAllpicsNumLabel();
    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *event) override;
    //根据已选择照片与视频数量刷新提示
    void resetSelectedStatue(int photosCount, int videosCount);
    //显示照片与视频总数
    void resetUnselectedStatue(int photosCount, int videosCount);
public:
    DLabel *m_pAllPicNumLabel;
    DSlider *m_pSlider;
    DLabel *m_pstacklabel;
    DWidget *m_pimporting;
    DLabel *TextLabel;
    QStringList imgpaths;
    QStackedWidget *m_pStackedWidget;
    DSpinner *loadingicon;

    int m_allPicNum;
    int interval;
    int pic_count;
private:
    int m_index;
    bool m_bcustalbum = false;
    bool m_baddDuplicatePhotos = false;
    QString m_alubm;


protected:
    void timerEvent(QTimerEvent *e) Q_DECL_OVERRIDE;

};



#endif // STATUSBAR_H

