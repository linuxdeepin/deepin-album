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
#include "timelinedatewidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>

#include <DFontSizeManager>
#include <DApplicationHelper>

TimeLineDateWidget::TimeLineDateWidget(QStandardItem *item, const QString &time, const QString &num)
    :  m_chooseBtn(nullptr), m_pDate(nullptr), m_pNum(nullptr), m_currentItem(item)
{
    this->setContentsMargins(0, 0, 0, 0);
    this->setFixedHeight(90);
    //时间线日期
    m_pDate = new DLabel(this);
    DFontSizeManager::instance()->bind(m_pDate, DFontSizeManager::T3, QFont::DemiBold);
    QFont ft1 = DFontSizeManager::instance()->get(DFontSizeManager::T3);
    ft1.setFamily("SourceHanSansSC");
    ft1.setWeight(QFont::DemiBold);
    DPalette color = DApplicationHelper::instance()->palette(m_pDate);
    color.setBrush(DPalette::Text, color.color(DPalette::ToolTipText));
    m_pDate->setFont(ft1);
    m_pDate->setForegroundRole(DPalette::Text);
    m_pDate->setPalette(color);
    m_pDate->setText(time);

    //数量
    m_pNum = new DLabel(this);
    //TODO：维语适配
//    if (QLocale::system().language() == QLocale::Uighur) {
//        m_pNum->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
//    }
    DFontSizeManager::instance()->bind(m_pNum, DFontSizeManager::T6, QFont::Medium);
    QFont ft2 = DFontSizeManager::instance()->get(DFontSizeManager::T6);
    ft2.setFamily("SourceHanSansSC");
    ft2.setWeight(QFont::Medium);
    DPalette pal = DApplicationHelper::instance()->palette(m_pNum);
    QColor color_BT = pal.color(DPalette::BrightText);

    m_pNum->setFont(ft2);
    m_pNum->setForegroundRole(DPalette::Text);
    m_pNum->setPalette(pal);
    m_pNum->setText(num);

    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        color_BT.setAlphaF(0.5);
        pal.setBrush(DPalette::Text, color_BT);
        m_pNum->setForegroundRole(DPalette::Text);
        m_pNum->setPalette(pal);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_BT.setAlphaF(0.75);
        pal.setBrush(DPalette::Text, color_BT);
        m_pNum->setForegroundRole(DPalette::Text);
        m_pNum->setPalette(pal);
    }

    //选择按钮
    m_chooseBtn = new DCommandLinkButton(QObject::tr("Select"));
    connect(m_chooseBtn, &DCommandLinkButton::clicked, this, &TimeLineDateWidget::onChooseBtnCliked);
    DFontSizeManager::instance()->bind(m_chooseBtn, DFontSizeManager::T5);
    m_chooseBtn->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
    m_chooseBtn->setFocusPolicy(Qt::NoFocus);
    m_chooseBtn->setVisible(false);
    //占位btn，防止显影选择按钮时，ui变化
    m_pbtn = new DCommandLinkButton(" ");
    m_pbtn->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
    m_pbtn->setFocusPolicy(Qt::NoFocus);

    QHBoxLayout *NumandBtnLayout = new QHBoxLayout();
    NumandBtnLayout->setContentsMargins(0, 0, 0, 0);
    NumandBtnLayout->addWidget(m_pNum);
    NumandBtnLayout->addStretch();
    NumandBtnLayout->addWidget(m_pbtn);
    NumandBtnLayout->addWidget(m_chooseBtn);

    QVBoxLayout *TitleViewLayout = new QVBoxLayout(this);
    TitleViewLayout->setContentsMargins(6, 0, 23, 0);
    TitleViewLayout->addWidget(m_pDate);
    TitleViewLayout->addStretch();
    TitleViewLayout->addLayout(NumandBtnLayout);
    this->setLayout(TitleViewLayout);
}

void TimeLineDateWidget::onChooseBtnCliked()
{
    if (QObject::tr("Select") == m_chooseBtn->text()) {
        m_chooseBtn->setText(QObject::tr("Unselect"));
        emit sigIsSelectCurrentDatePic(true, m_currentItem);
    } else {
        m_chooseBtn->setText(QObject::tr("Select"));
        emit sigIsSelectCurrentDatePic(false, m_currentItem);
    }
}

void TimeLineDateWidget::onChangeChooseBtnVisible(bool visible)
{
    m_chooseBtn->setVisible(visible);
}

void TimeLineDateWidget::onTimeLinePicSelectAll(bool selectall)
{
    if (selectall) { //已经全选，btn变为取消全选
        m_chooseBtn->setText(QObject::tr("Unselect"));
    } else {         //非全选状态，btn变为全选
        m_chooseBtn->setText(QObject::tr("Select"));
    }
}

QString TimeLineDateWidget::onGetBtnStatus()
{
    return m_chooseBtn->text();
}

importTimeLineDateWidget::importTimeLineDateWidget(QStandardItem *item, const QString &time, const QString &num)
    : m_chooseBtn(nullptr), m_pDateandNum(nullptr), m_currentItem(item)
{
    this->setContentsMargins(0, 0, 0, 0);
    this->setFixedHeight(35);

    //时间+照片数量
    m_pDateandNum = new DLabel(this);
    DFontSizeManager::instance()->bind(m_pDateandNum, DFontSizeManager::T6, QFont::Medium);
    QFont ft1 = DFontSizeManager::instance()->get(DFontSizeManager::T6);
    ft1.setFamily("SourceHanSansSC");
    ft1.setWeight(QFont::Medium);
    DPalette pal = DApplicationHelper::instance()->palette(m_pDateandNum);
    QColor color_BT = pal.color(DPalette::BrightText);
    m_pDateandNum->setFont(ft1);
    m_pDateandNum->setForegroundRole(DPalette::Text);
    m_pDateandNum->setPalette(pal);
    QString tempTimeAndNumber = time + " " + num;
    m_pDateandNum->setText(tempTimeAndNumber);

    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        color_BT.setAlphaF(0.5);
        pal.setBrush(DPalette::Text, color_BT);
        m_pDateandNum->setForegroundRole(DPalette::Text);
        m_pDateandNum->setPalette(pal);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_BT.setAlphaF(0.75);
        pal.setBrush(DPalette::Text, color_BT);
        m_pDateandNum->setForegroundRole(DPalette::Text);
        m_pDateandNum->setPalette(pal);
    }

    //选择按钮
    m_chooseBtn = new DCommandLinkButton(QObject::tr("Select"));
    connect(m_chooseBtn, &DCommandLinkButton::clicked, this, &importTimeLineDateWidget::onChooseBtnCliked);
    DFontSizeManager::instance()->bind(m_chooseBtn, DFontSizeManager::T5);
    m_chooseBtn->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
    m_chooseBtn->setFocusPolicy(Qt::NoFocus);
    m_chooseBtn->setVisible(false);

    //占位btn，防止显影选择按钮时，ui变化
    m_pbtn = new DCommandLinkButton(" ");
    m_pbtn->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
    m_pbtn->setFocusPolicy(Qt::NoFocus);

    //开始布局
    QHBoxLayout *TitleViewLayout = new QHBoxLayout(this);
    TitleViewLayout->setContentsMargins(0, 0, 25, 0);
    TitleViewLayout->addWidget(m_pDateandNum);
    TitleViewLayout->addStretch();
    TitleViewLayout->addWidget(m_pbtn);
    TitleViewLayout->addWidget(m_chooseBtn);
    this->setLayout(TitleViewLayout);
}

void importTimeLineDateWidget::onChooseBtnCliked()
{
    if (QObject::tr("Select") == m_chooseBtn->text()) {
        m_chooseBtn->setText(QObject::tr("Unselect"));
        emit sigIsSelectCurrentDatePic(true, m_currentItem);
    } else {
        m_chooseBtn->setText(QObject::tr("Select"));
        emit sigIsSelectCurrentDatePic(false, m_currentItem);
    }
}

void importTimeLineDateWidget::onChangeChooseBtnVisible(bool visible)
{
    m_chooseBtn->setVisible(visible);
}

void importTimeLineDateWidget::onTimeLinePicSelectAll(bool selectall)
{
    if (selectall) { //已经全选，btn变为取消全选
        m_chooseBtn->setText(QObject::tr("Unselect"));
    } else {         //非全选状态，btn变为全选
        m_chooseBtn->setText(QObject::tr("Select"));
    }
}

QString importTimeLineDateWidget::onGetBtnStatus()
{
    return m_chooseBtn->text();
}
