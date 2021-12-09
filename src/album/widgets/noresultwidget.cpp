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
#include "noresultwidget.h"
#include <DApplicationHelper>
#include "imageengine/imageengineapi.h"
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QDebug>

NoResultWidget::NoResultWidget(QWidget *parent): QWidget(parent)
{
    initNoSearchResultView();

    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this, &NoResultWidget::changeTheme);
}

NoResultWidget::~NoResultWidget()
{

}

void NoResultWidget::initNoSearchResultView()
{
    QHBoxLayout *pHBoxLayout = new QHBoxLayout(this);
    this->setLayout(pHBoxLayout);

    pNoResult = new DLabel();
    pNoResult->setText(tr("No results"));
    pNoResult->setAlignment(Qt::AlignCenter);
    pHBoxLayout->addWidget(pNoResult);

    pNoResult->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T4));
    DPalette palette = DApplicationHelper::instance()->palette(pNoResult);
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
}

void NoResultWidget::changeTheme()
{
    DPalette pale = DApplicationHelper::instance()->palette(pNoResult);
    pale.setBrush(DPalette::Text, pale.color(DPalette::ToolTipText));
    pNoResult->setPalette(pale);

    DPalette pa = DApplicationHelper::instance()->palette(pNoResult);
    QColor color_TTT = pa.color(DPalette::ToolTipText);
    DPalette pat = DApplicationHelper::instance()->palette(pNoResult);
    QColor color_BT = pat.color(DPalette::BrightText);

    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        color_TTT.setAlphaF(0.3);
        pa.setBrush(DPalette::Text, color_TTT);
        pNoResult->setPalette(pa);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_TTT.setAlphaF(0.4);
        pa.setBrush(DPalette::Text, color_TTT);
        pNoResult->setPalette(pa);
    }
}
