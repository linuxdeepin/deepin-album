// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "rubberband.h"

#include <QApplication>
#include <QStyleOptionRubberBand>
#include <QDebug>

RubberBand::RubberBand(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    qDebug() << "RubberBand::RubberBand - Constructor entry";
}

RubberBand::~RubberBand()
{
    // qDebug() << "RubberBand::~RubberBand - Destructor entry";
}

void RubberBand::paint(QPainter *painter)
{
    // qDebug() << "RubberBand::paint - Function entry";
    if (!qApp || !qApp->style()) {
        // qDebug() << "RubberBand::paint - Branch: no app or style, returning";
        return;
    }

    QStyleOptionRubberBand opt;
    opt.state = QStyle::State_None;
    opt.direction = qApp->layoutDirection();
    opt.styleObject = this;
    opt.palette = qApp->palette();
    opt.shape = QRubberBand::Rectangle;
    opt.opaque = false;
    opt.rect = contentsBoundingRect().toRect();
    qApp->style()->drawControl(QStyle::CE_RubberBand, &opt, painter);
    // qDebug() << "RubberBand::paint - Function exit";
}

bool RubberBand::intersects(const QRectF &rect) const
{
    // qDebug() << "RubberBand::intersects - Function entry, rect:" << rect;
    return m_geometry.intersects(rect);
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void RubberBand::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
#else
void RubberBand::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
#endif
{
    // qDebug() << "RubberBand::geometryChanged/Change - Function entry, newGeometry:" << newGeometry << "oldGeometry:" << oldGeometry;
    Q_UNUSED(oldGeometry);

    m_geometry = newGeometry;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
#else
    QQuickItem::geometryChange(newGeometry, oldGeometry);
#endif
    // qDebug() << "RubberBand::geometryChanged/Change - Function exit";
}
