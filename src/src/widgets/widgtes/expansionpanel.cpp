// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "expansionpanel.h"
#include "ddlog.h"

#include <QGraphicsDropShadowEffect>
#include <QVBoxLayout>
#include <iostream>

#include <DPalette>

DGUI_USE_NAMESPACE

ExpansionPanel::ExpansionPanel(QWidget *parent)
    : DBlurEffectWidget(parent)
{
    qCDebug(albumLog) << "ExpansionPanel::ExpansionPanel - Entry";
    this->setObjectName("ExpansionPanel");
    this->setAccessibleName("ExpansionPanel");
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
    qCDebug(albumLog) << "ExpansionPanel::ExpansionPanel - Exit";
}

void ExpansionPanel::changeTheme(Dtk::Gui::DGuiApplicationHelper::ColorType themeType)
{
    // qCDebug(albumLog) << "ExpansionPanel::changeTheme - Entry";
    if (themeType == 1) {
        // qCDebug(albumLog) << "ExpansionPanel::changeTheme - Entry, themeType is 1";
        //背景颜色及透明度
        auto effect = QColor("#EBEBEB");
        effect.setAlpha(80);
        setMaskColor(effect);
        shadow_effect->setColor(QColor(150, 150, 150));
    } else {
        // qCDebug(albumLog) << "ExpansionPanel::changeTheme - Entry, themeType is not 1";
        //背景颜色及透明度
        auto effect = QColor("#404040");
        setMaskColor(effect);
        shadow_effect->setColor(QColor("#404040"));
    }
    // qCDebug(albumLog) << "ExpansionPanel::changeTheme - Exit: theme change completed";
}

void ExpansionPanel::onButtonClicked(FilteData data)
{
    qCDebug(albumLog) << "ExpansionPanel::onButtonClicked - Entry";
    this->hide();
    emit currentItemChanged(data);
    qCDebug(albumLog) << "ExpansionPanel::onButtonClicked - Exit";
}

void ExpansionPanel::addNewButton(FilteData &data)
{
    qCDebug(albumLog) << "ExpansionPanel::addNewButton - Entry";
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
    qCDebug(albumLog) << "ExpansionPanel::addNewButton - Exit";
}

void ExpansionPanel::focusOutEvent(QFocusEvent *e)
{
    // qCDebug(albumLog) << "ExpansionPanel::focusOutEvent - Entry";
    hide();
    DBlurEffectWidget::focusOutEvent(e);
    // qCDebug(albumLog) << "ExpansionPanel::focusOutEvent - Exit";
}
