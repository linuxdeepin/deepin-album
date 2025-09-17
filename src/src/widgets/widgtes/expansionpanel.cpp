// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "expansionpanel.h"

#include <QGraphicsDropShadowEffect>
#include <QVBoxLayout>
#include <iostream>

#include <DPalette>

DGUI_USE_NAMESPACE

ExpansionPanel::ExpansionPanel(QWidget *parent)
    : DBlurEffectWidget(parent)
{
    qDebug() << "ExpansionPanel::ExpansionPanel - Entry";
    //wayland背景透明问题
    DPalette imgInfoDlgPl = this->palette();
    QColor imgInfoDlgColor("#F7F7F7");
    imgInfoDlgColor.setAlphaF(0.8);
    imgInfoDlgPl.setColor(DPalette::Window, imgInfoDlgColor);
    this->setBackgroundRole(DPalette::Window);

    this->setWindowFlag(Qt::Popup);
    layout = new QVBoxLayout;

    setBlurEnabled(true);
    setMode(DBlurEffectWidget::GaussianBlur);

    //设置边距
    layout->setContentsMargins(0, 12, 0, 12);

    setLayout(layout);
    qDebug() << "ExpansionPanel::ExpansionPanel - Exit";
}

void ExpansionPanel::changeTheme(Dtk::Gui::DGuiApplicationHelper::ColorType themeType)
{
    // qDebug() << "ExpansionPanel::changeTheme - Entry";
    if (themeType == 1) {
        // qDebug() << "ExpansionPanel::changeTheme - Entry, themeType is 1";
        //背景颜色及透明度
        auto effect = QColor("#EBEBEB");
        effect.setAlpha(80);
        setMaskColor(effect);
        shadow_effect->setColor(QColor(150, 150, 150));
    } else {
        // qDebug() << "ExpansionPanel::changeTheme - Entry, themeType is not 1";
        //背景颜色及透明度
        auto effect = QColor("#404040");
        setMaskColor(effect);
        shadow_effect->setColor(QColor("#404040"));
    }
    // qDebug() << "ExpansionPanel::changeTheme - Exit: theme change completed";
}

void ExpansionPanel::onButtonClicked(FilteData data)
{
    qDebug() << "ExpansionPanel::onButtonClicked - Entry";
    this->hide();
    emit currentItemChanged(data);
    qDebug() << "ExpansionPanel::onButtonClicked - Exit";
}

void ExpansionPanel::addNewButton(FilteData &data)
{
    qDebug() << "ExpansionPanel::addNewButton - Entry";
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
    qDebug() << "ExpansionPanel::addNewButton - Exit";
}

void ExpansionPanel::focusOutEvent(QFocusEvent *e)
{
    // qDebug() << "ExpansionPanel::focusOutEvent - Entry";
    hide();
    DBlurEffectWidget::focusOutEvent(e);
    // qDebug() << "ExpansionPanel::focusOutEvent - Exit";
}
