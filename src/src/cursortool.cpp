// SPDX-FileCopyrightText: 2022 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cursortool.h"

#include <QTimer>
#include <QCursor>
#include <DGuiApplicationHelper>

static const int sc_ESampleInterval = 50;  // 采样间隔 50ms

CursorTool::CursorTool(QObject *parent)
    : QObject(parent)
{
    qDebug() << "CursorTool::CursorTool - Function entry";
    m_CaptureTimer = new QTimer(this);
    m_CaptureTimer->setInterval(sc_ESampleInterval);

    connect(m_CaptureTimer, &QTimer::timeout, this, [this]() {
        QPoint pos = QCursor::pos();
        if (pos != m_lastPos) {
            m_lastPos = pos;
            // 发送当前光标的全局位置
            Q_EMIT this->cursorPosChanged(pos.x(), pos.y());
        }
    });

    connect(Dtk::Gui::DGuiApplicationHelper::instance(), &Dtk::Gui::DGuiApplicationHelper::applicationPaletteChanged, [this]() {
        auto newColor = Dtk::Gui::DGuiApplicationHelper::instance()->applicationPalette().highlight().color();
        emit activeColorChanged(newColor);
    });
    qDebug() << "CursorTool::CursorTool - Function exit";
}

/**
   @return 返回当前鼠标光标在屏幕的位置
 */
QPoint CursorTool::currentCursorPos() const
{
    // qDebug() << "CursorTool::currentCursorPos - Function entry";
    return QCursor::pos();
}

/**
 * @param b 设置是否启动定时查询光标位置
 *  默认间隔20ms
 */
void CursorTool::setCaptureCursor(bool b)
{
    qDebug() << "CursorTool::setCaptureCursor - Function entry, b:" << b;
    if (b) {
        qDebug() << "CursorTool::setCaptureCursor - Branch: starting timer";
        m_CaptureTimer->start();
    } else {
        qDebug() << "CursorTool::setCaptureCursor - Branch: stopping timer";
        m_CaptureTimer->stop();
    }
    qDebug() << "CursorTool::setCaptureCursor - Function exit";
}

QColor CursorTool::activeColor()
{
    // qDebug() << "CursorTool::activeColor - Function entry";
    QColor color = Dtk::Gui::DGuiApplicationHelper::instance()->applicationPalette().highlight().color();
    // qDebug() << "CursorTool::activeColor - Function exit, returning color:" << color;
    return color;
}
