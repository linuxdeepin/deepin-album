// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "noresultwidget.h"
#include <DGuiApplicationHelper>
#include <DPaletteHelper>
#include <DFontSizeManager>

#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QDebug>

NoResultWidget::NoResultWidget(QWidget *parent): QWidget(parent)
{
    qDebug() << "NoResultWidget::NoResultWidget - Entry";
    initNoSearchResultView();

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &NoResultWidget::changeTheme);
    qDebug() << "NoResultWidget::NoResultWidget - Exit";
}

NoResultWidget::~NoResultWidget()
{
    // qDebug() << "NoResultWidget::~NoResultWidget - Entry";
}

void NoResultWidget::showEvent(QShowEvent *ev)
{
    // qDebug() << "NoResultWidget::showEvent - Entry";
    changeTheme();
    QWidget::showEvent(ev);
    // qDebug() << "NoResultWidget::showEvent - Exit";
}

void NoResultWidget::initNoSearchResultView()
{
    qDebug() << "NoResultWidget::initNoSearchResultView - Entry";
    QHBoxLayout *pHBoxLayout = new QHBoxLayout(this);
    this->setLayout(pHBoxLayout);

    pNoResult = new DLabel();
    pNoResult->setText(tr("No results"));
    pNoResult->setAlignment(Qt::AlignCenter);
    pHBoxLayout->addWidget(pNoResult);

    pNoResult->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T4));
    DPalette palette = DPaletteHelper::instance()->palette(pNoResult);
    QColor color_TTT = palette.color(DPalette::ToolTipText);
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        color_TTT.setAlphaF(0.3);
        palette.setBrush(DPalette::Text, color_TTT);
        pNoResult->setForegroundRole(DPalette::Text);
        pNoResult->setPalette(palette);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_TTT.setAlphaF(0.4);
        palette.setBrush(DPalette::Text, color_TTT);
        pNoResult->setForegroundRole(DPalette::Text);
        pNoResult->setPalette(palette);
    }
    qDebug() << "NoResultWidget::initNoSearchResultView - Exit";
}

void NoResultWidget::changeTheme()
{
    qDebug() << "NoResultWidget::changeTheme - Entry";
    DPalette pale = DPaletteHelper::instance()->palette(pNoResult);
    pale.setBrush(DPalette::Text, pale.color(DPalette::ToolTipText));
    pNoResult->setPalette(pale);

    DPalette pa = DPaletteHelper::instance()->palette(pNoResult);
    QColor color_TTT = pa.color(DPalette::ToolTipText);
    DPalette pat = DPaletteHelper::instance()->palette(pNoResult);
    QColor color_BT = pat.color(DPalette::BrightText);

    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        qDebug() << "NoResultWidget::changeTheme - LightType";
        color_TTT.setAlphaF(0.3);
        pa.setBrush(DPalette::Text, color_TTT);
        pNoResult->setPalette(pa);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        qDebug() << "NoResultWidget::changeTheme - DarkType";
        color_TTT.setAlphaF(0.4);
        pa.setBrush(DPalette::Text, color_TTT);
        pNoResult->setPalette(pa);
    }
    qDebug() << "NoResultWidget::changeTheme - Exit";
}
