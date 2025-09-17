// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "toolbutton.h"
#include <QPainter>
#include <QStylePainter>
#include <QSvgRenderer>
#include <QStyleOptionButton>
#include <qtoolbutton.h>
#include <DGuiApplicationHelper>

ToolButton::ToolButton(QWidget *parent) : DPushButton(parent)
{
    // qDebug() << "ToolButton::ToolButton - Entry";
    setMinimumSize(200, 11);
    // qDebug() << "ToolButton::ToolButton - Exit";
}

void ToolButton::setText(const QString &text)
{
    // qDebug() << "ToolButton::setText - Entry";
    //控件设置需要绘制的文字
    m_text = text;
    update();
    // qDebug() << "ToolButton::setText - Exit";
}

void ToolButton::setLIcon(const QIcon &icon_light, const QIcon &icon_dark)
{
    // qDebug() << "ToolButton::setLIcon - Entry";
    //控件设置需要绘制的图片
    m_Licon_light = icon_light;
    m_Licon_dark = icon_dark;
    update();
    // qDebug() << "ToolButton::setLIcon - Exit";
}

void ToolButton::setRIcon(const QIcon &icon_light, const QIcon &icon_dark)
{
    // qDebug() << "ToolButton::setRIcon - Entry";
    //控件设置需要绘制的图片
    m_Ricon_light = icon_light;
    m_Ricon_dark = icon_dark;
    update();
    // qDebug() << "ToolButton::setRIcon - Exit";
}

void ToolButton::setRWIcon(const QString &path)
{
    // qDebug() << "ToolButton::setRWIcon - Entry";
    m_RiconWhite = QIcon::fromTheme(path);
    update();
    // qDebug() << "ToolButton::setRWIcon - Exit";
}

void ToolButton::paintEvent(QPaintEvent *e)
{
    // qDebug() << "ToolButton::paintEvent - Entry";
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
    // qDebug() << "ToolButton::paintEvent - Exit";
}

void ToolButton::focusInEvent(QFocusEvent *e)
{
    // qDebug() << "ToolButton::focusInEvent - Entry";
    emit focusStatusChanged(true);
    DPushButton::focusInEvent(e);
    // qDebug() << "ToolButton::focusInEvent - Exit";
}

void ToolButton::focusOutEvent(QFocusEvent *e)
{
    // qDebug() << "ToolButton::focusOutEvent - Entry";
    emit focusStatusChanged(false);
    DPushButton::focusOutEvent(e);
    // qDebug() << "ToolButton::focusOutEvent - Exit";
}

void ToolButton::enterEvent(QEnterEvent *event)
{
    // qDebug() << "ToolButton::enterEvent - Entry";
    currentStatus = btnHover;
    DPushButton::enterEvent(event);
    // qDebug() << "ToolButton::enterEvent - Exit";
}

void ToolButton::leaveEvent(QEvent *event)
{
    // qDebug() << "ToolButton::leaveEvent - Entry";
    currentStatus = btnNormal;
    DPushButton::leaveEvent(event);
    // qDebug() << "ToolButton::leaveEvent - Exit";
}

void ToolButton::hideEvent(QHideEvent *event)
{
    // qDebug() << "ToolButton::hideEvent - Entry";
    currentStatus = btnNormal;
    DPushButton::hideEvent(event);
    // qDebug() << "ToolButton::hideEvent - Exit";
}
