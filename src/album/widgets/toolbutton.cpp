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

#include "toolbutton.h"
#include <QPainter>
#include <QStylePainter>
#include <QSvgRenderer>
#include <QStyleOptionButton>
#include <qtoolbutton.h>
#include <DGuiApplicationHelper>

ToolButton::ToolButton(QWidget *parent) : DPushButton(parent)
{
    setMinimumSize(200, 11);
}

void ToolButton::setText(const QString &text)
{
    //控件设置需要绘制的文字
    m_text = text;
    update();
}

void ToolButton::setLIcon(const QIcon &icon_light, const QIcon &icon_dark)
{
    //控件设置需要绘制的图片
    m_Licon_light = icon_light;
    m_Licon_dark = icon_dark;
    update();
}

void ToolButton::setRIcon(const QIcon &icon_light, const QIcon &icon_dark)
{
    //控件设置需要绘制的图片
    m_Ricon_light = icon_light;
    m_Ricon_dark = icon_dark;
    update();
}

void ToolButton::setRWIcon(const QString &path)
{
    m_RiconWhite = QIcon::fromTheme(path);
    update();
}

void ToolButton::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)

    QStylePainter p(this);
    QStyleOptionButton option;
    initStyleOption(&option);

    QPainter painter(this);
    //绘制背景
    painter.save();
    //非禁用状态绘制背景
    if (option.state & QStyle::State_Enabled) {
        if (currentStatus == btnHover) {
            painter.setPen(Qt::NoPen);
            //获取系统主题颜色
            QColor hovertColor(option.palette.highlight().color());
            painter.setBrush(hovertColor);
            painter.drawRect(this->rect());
        }
    }
    painter.restore();

    if (Dtk::Gui::DGuiApplicationHelper::instance()->themeType() == Dtk::Gui::DGuiApplicationHelper::LightType) {
        //控件禁用样式
        if (!(option.state & QStyle::State_Enabled)) {
            painter.setPen(QColor("#9C9C9C"));
        }

        //鼠标悬停画笔颜色
        else if (currentStatus == btnHover) {
            painter.setPen(QColor(Qt::white));
        }

        //鼠标按下画笔颜色
        else if (option.state & QStyle::State_Sunken) {
            painter.setPen(QColor("#99cdff"));
        } else {
            painter.setPen(QColor("#343434"));
        }

        //图片
        painter.drawPixmap(QRect(10, -6, 32, 32), m_Licon_light.pixmap(QSize(32, 32)));
    } else {
        //控件禁用样式
        if (!(option.state & QStyle::State_Enabled)) {
            painter.setPen(QColor("#8D8D8D"));
        }

        //鼠标悬停画笔颜色
        else if (currentStatus == btnHover) {
            painter.setPen(QColor(Qt::white));
        }

        //鼠标按下画笔颜色
        else if (option.state & QStyle::State_Sunken) {
            painter.setPen(QColor("#99cdff"));
        } else {
            painter.setPen(QColor(Qt::white));
        }

        //图片
        painter.drawPixmap(QRect(10, -6, 32, 32), m_Licon_dark.pixmap(QSize(32, 32)));
    }

    //绘制文字
    painter.save();
    QFont ft;
    ft.setPixelSize(14);
    painter.setFont(ft);
    painter.drawText(QPoint(40, 22), m_text);
    painter.restore();

    // 绘制图片
    if (currentStatus == btnHover) {
        painter.drawPixmap(QRect(130, 10, 16, 16), m_RiconWhite.pixmap(QSize(16, 16)));
    } else {
        if (Dtk::Gui::DGuiApplicationHelper::instance()->themeType() == Dtk::Gui::DGuiApplicationHelper::LightType) {
            painter.drawPixmap(QRect(130, 10, 16, 16), m_Ricon_light.pixmap(QSize(16, 16)));
        } else {
            painter.drawPixmap(QRect(130, 10, 16, 16), m_Ricon_dark.pixmap(QSize(16, 16)));
        }
    }
}

void ToolButton::focusInEvent(QFocusEvent *e)
{
    emit focusStatusChanged(true);
    DPushButton::focusInEvent(e);
}

void ToolButton::focusOutEvent(QFocusEvent *e)
{
    emit focusStatusChanged(false);
    DPushButton::focusOutEvent(e);
}

void ToolButton::enterEvent(QEvent *event)
{
    currentStatus = btnHover;
    DPushButton::enterEvent(event);
}

void ToolButton::leaveEvent(QEvent *event)
{
    currentStatus = btnNormal;
    DPushButton::leaveEvent(event);
}

void ToolButton::hideEvent(QHideEvent *event)
{
    currentStatus = btnNormal;
    DPushButton::hideEvent(event);
}
