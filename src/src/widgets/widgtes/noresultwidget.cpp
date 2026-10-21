// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "noresultwidget.h"
#include "ddlog.h"
#include <DGuiApplicationHelper>
#include <DPaletteHelper>
#include <DFontSizeManager>

#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QDebug>

NoResultWidget::NoResultWidget(QWidget *parent): QWidget(parent)
{
    qCDebug(albumLog) << "NoResultWidget::NoResultWidget - Entry";
    this->setObjectName("NoResultWidget");
    this->setAccessibleName("NoResultWidget");
    initNoSearchResultView();

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &NoResultWidget::changeTheme);
    qCDebug(albumLog) << "NoResultWidget::NoResultWidget - Exit";
}

NoResultWidget::~NoResultWidget()
{
    // qCDebug(albumLog) << "NoResultWidget::~NoResultWidget - Entry";
}

void NoResultWidget::showEvent(QShowEvent *ev)
{
    // qCDebug(albumLog) << "NoResultWidget::showEvent - Entry";
    changeTheme();
    QWidget::showEvent(ev);
    // qCDebug(albumLog) << "NoResultWidget::showEvent - Exit";
}

void NoResultWidget::initNoSearchResultView()
{
    qCDebug(albumLog) << "NoResultWidget::initNoSearchResultView - Entry";
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
    qCDebug(albumLog) << "NoResultWidget::initNoSearchResultView - Exit";
}

void NoResultWidget::changeTheme()
{
    qCDebug(albumLog) << "NoResultWidget::changeTheme - Entry";
    DPalette pale = DPaletteHelper::instance()->palette(pNoResult);
    pale.setBrush(DPalette::Text, pale.color(DPalette::ToolTipText));
    pNoResult->setPalette(pale);

    DPalette pa = DPaletteHelper::instance()->palette(pNoResult);
    QColor color_TTT = pa.color(DPalette::ToolTipText);
    DPalette pat = DPaletteHelper::instance()->palette(pNoResult);
    QColor color_BT = pat.color(DPalette::BrightText);

    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        qCDebug(albumLog) << "NoResultWidget::changeTheme - LightType";
        color_TTT.setAlphaF(0.3);
        pa.setBrush(DPalette::Text, color_TTT);
        pNoResult->setPalette(pa);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        qCDebug(albumLog) << "NoResultWidget::changeTheme - DarkType";
        color_TTT.setAlphaF(0.4);
        pa.setBrush(DPalette::Text, color_TTT);
        pNoResult->setPalette(pa);
    }
    qCDebug(albumLog) << "NoResultWidget::changeTheme - Exit";
}
