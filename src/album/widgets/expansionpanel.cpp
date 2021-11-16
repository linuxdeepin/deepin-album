/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co.,Ltd.
 *
 * Author:     Ji XiangLong <jixianglong@uniontech.com>
 *
 * Maintainer: WangYu <wangyu@uniontech.com>
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
#include "expansionpanel.h"

#include <QGraphicsDropShadowEffect>
#include <QVBoxLayout>
#include <iostream>

#include <DPalette>

DGUI_USE_NAMESPACE

ExpansionPanel::ExpansionPanel(QWidget *parent)
    : DBlurEffectWidget(parent)
{
    //wayland背景透明问题
    DPalette imgInfoDlgPl = this->palette();
    QColor imgInfoDlgColor("#F7F7F7");
    imgInfoDlgColor.setAlphaF(0.8);
    imgInfoDlgPl.setColor(DPalette::Window, imgInfoDlgColor);
    this->setBackgroundRole(DPalette::Window);

    this->setWindowFlag(Qt::Popup);
    layout = new QVBoxLayout;

    //设置圆角
    setBlurRectXRadius(18);
    setBlurRectYRadius(18);
    setBlurEnabled(true);
    setMode(DBlurEffectWidget::GaussianBlur);

    //绘制背景阴影
    setAttribute(Qt::WA_TranslucentBackground);
    const int nMargin = 10;     // 设置阴影宽度
    shadow_effect = new QGraphicsDropShadowEffect(this);
    shadow_effect->setOffset(0, 0);
    shadow_effect->setBlurRadius(nMargin);
    this->setGraphicsEffect(shadow_effect); //最外层的Frame

    //设置边距
    layout->setContentsMargins(0, 12, 0, 12);

    setLayout(layout);
}

void ExpansionPanel::changeTheme(Dtk::Gui::DGuiApplicationHelper::ColorType themeType)
{
    if (themeType == 1) {
        //背景颜色及透明度
        auto effect = QColor("#EBEBEB");
        effect.setAlpha(80);
        setMaskColor(effect);
        shadow_effect->setColor(QColor(150, 150, 150));
    } else {
        //背景颜色及透明度
        auto effect = QColor("#404040");
        setMaskColor(effect);
        shadow_effect->setColor(QColor("#404040"));
    }
}

void ExpansionPanel::onButtonClicked(FilteData data)
{
    emit currentItemChanged(data);
    this->hide();
}

void ExpansionPanel::addNewButton(FilteData &data)
{
    auto button = new ToolButton(this);
    buttons[buttonCount++] = button;
    button->setText(data.text);
    button->setLIcon(data.icon_l_light, data.icon_l_dark);
    button->setRIcon(data.icon_r_light, data.icon_r_dark);
    button->setRWIcon((data.icon_r_path + "_hover"));
    button->setFixedSize(190, 34);
    layout->addWidget(button);
    connect(button, &ToolButton::clicked, [data, this]() {
        onButtonClicked(data);
    });
}

void ExpansionPanel::focusOutEvent(QFocusEvent *e)
{
    hide();
    DBlurEffectWidget::focusOutEvent(e);
}
