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

