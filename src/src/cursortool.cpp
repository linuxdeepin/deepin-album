// SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cursortool.h"
#include "ddlog.h"

#include <QTimer>
#include <QCursor>
#include <DGuiApplicationHelper>

static const int sc_ESampleInterval = 50;  // 采样间隔 50ms

CursorTool::CursorTool(QObject *parent)
    : QObject(parent)
{
    qCDebug(albumLog) << "CursorTool::CursorTool - Function entry";
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
    qCDebug(albumLog) << "CursorTool::CursorTool - Function exit";
}

/**
   @return 返回当前鼠标光标在屏幕的位置
 */
QPoint CursorTool::currentCursorPos() const
{
    // qCDebug(albumLog) << "CursorTool::currentCursorPos - Function entry";
    return QCursor::pos();
}

/**
 * @param b 设置是否启动定时查询光标位置
 *  默认间隔20ms
 */
void CursorTool::setCaptureCursor(bool b)
{
    qCDebug(albumLog) << "CursorTool::setCaptureCursor - Function entry, b:" << b;
    if (b) {
        qCDebug(albumLog) << "CursorTool::setCaptureCursor - Branch: starting timer";
        m_CaptureTimer->start();
    } else {
        qCDebug(albumLog) << "CursorTool::setCaptureCursor - Branch: stopping timer";
        m_CaptureTimer->stop();
    }
    qCDebug(albumLog) << "CursorTool::setCaptureCursor - Function exit";
}

QColor CursorTool::activeColor()
{
    // qCDebug(albumLog) << "CursorTool::activeColor - Function entry";
    QColor color = Dtk::Gui::DGuiApplicationHelper::instance()->applicationPalette().highlight().color();
    // qCDebug(albumLog) << "CursorTool::activeColor - Function exit, returning color:" << color;
    return color;
}
