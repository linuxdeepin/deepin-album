// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NORESULTWIDGET_H
#define NORESULTWIDGET_H

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

class NoResultWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NoResultWidget(QWidget *parent);
    ~NoResultWidget()override;

private:
    void initNoSearchResultView();
    void changeTheme();

private:
    DLabel *pNoResult;
};

#endif // NORESULTWIDGET_H
