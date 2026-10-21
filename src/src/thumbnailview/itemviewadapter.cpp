/*
    SPDX-FileCopyrightText: 2008 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemviewadapter.h"
#include "ddlog.h"
#include <QDebug>

#include <QModelIndex>
#include <QPalette>
#include <QSize>

ItemViewAdapter::ItemViewAdapter(QObject *parent)
    : QObject(parent)
    , m_adapterView(nullptr)
    , m_adapterModel(nullptr)
    , m_adapterIconSize(-1)
{
    // qCDebug(albumLog) << "Initializing ItemViewAdapter";
}

QAbstractItemModel *ItemViewAdapter::model() const
{
    // qCDebug(albumLog) << "Getting model, current model:" << (m_adapterModel ? "set" : "null");
    return m_adapterModel;
}

QSize ItemViewAdapter::iconSize() const
{
    // qCDebug(albumLog) << "Getting icon size:" << m_adapterIconSize;
    return QSize(m_adapterIconSize, m_adapterIconSize);
}

QPalette ItemViewAdapter::palette() const
{
    // qCDebug(albumLog) << "Getting default palette";
    return QPalette();
}

QRect ItemViewAdapter::visibleArea() const
{
    // qCDebug(albumLog) << "Getting visible area:" << m_adapterVisibleArea;
    return m_adapterVisibleArea;
}

QRect ItemViewAdapter::visualRect(const QModelIndex &index) const
{
    // qCDebug(albumLog) << "ItemViewAdapter::visualRect - Function entry, index:" << index;
    // FIXME TODO: Implemented on DND branch.

    Q_UNUSED(index)

    // qCDebug(albumLog) << "ItemViewAdapter::visualRect - Function exit, returning empty QRect";
    return QRect();
}

void ItemViewAdapter::connect(Signal signal, QObject *receiver, const char *slot)
{
    // qCDebug(albumLog) << "ItemViewAdapter::connect - Function entry, signal:" << static_cast<int>(signal) << "receiver:" << receiver << "slot:" << slot;
    if (signal == ScrollBarValueChanged) {
        // qCDebug(albumLog) << "ItemViewAdapter::connect - Branch: connecting ScrollBarValueChanged signal";
        QObject::connect(this, SIGNAL(viewScrolled()), receiver, slot);
    } else if (signal == IconSizeChanged) {
        // qCDebug(albumLog) << "ItemViewAdapter::connect - Branch: connecting IconSizeChanged signal";
        QObject::connect(this, SIGNAL(adapterIconSizeChanged()), receiver, slot);
    }
    // qCDebug(albumLog) << "ItemViewAdapter::connect - Function exit";
}

QAbstractItemModel *ItemViewAdapter::adapterModel() const
{
    // qCDebug(albumLog) << "ItemViewAdapter::adapterModel - Function entry, returning model:" << (m_adapterModel ? "set" : "null");
    return m_adapterModel;
}

QObject *ItemViewAdapter::adapterView() const
{
    // qCDebug(albumLog) << "ItemViewAdapter::adapterView - Function entry, returning view:" << (m_adapterView ? "set" : "null");
    return m_adapterView;
}

void ItemViewAdapter::setAdapterView(QObject *view)
{
    // qCDebug(albumLog) << "ItemViewAdapter::setAdapterView - Function entry, view:" << view;
    if (m_adapterView != view) {
        // qCDebug(albumLog) << "ItemViewAdapter::setAdapterView - Branch: view changed, emitting signal";
        m_adapterView = view;

        Q_EMIT adapterViewChanged();
    }
    // qCDebug(albumLog) << "ItemViewAdapter::setAdapterView - Function exit";
}

void ItemViewAdapter::setAdapterModel(QAbstractItemModel *model)
{
    // qCDebug(albumLog) << "ItemViewAdapter::setAdapterModel - Function entry, model:" << model;
    if (m_adapterModel != model) {
        // qCDebug(albumLog) << "ItemViewAdapter::setAdapterModel - Branch: model changed, emitting signal";
        m_adapterModel = model;

        Q_EMIT adapterModelChanged();
    }
    // qCDebug(albumLog) << "ItemViewAdapter::setAdapterModel - Function exit";
}

int ItemViewAdapter::adapterIconSize() const
{
    // qCDebug(albumLog) << "Getting adapter icon size:" << m_adapterIconSize;
    return m_adapterIconSize;
}

void ItemViewAdapter::setAdapterIconSize(int size)
{
    // qCDebug(albumLog) << "ItemViewAdapter::setAdapterIconSize - Function entry, size:" << size;
    if (m_adapterIconSize != size) {
        qCDebug(albumLog) << "Setting adapter icon size from" << m_adapterIconSize << "to" << size;
        m_adapterIconSize = size;

        Q_EMIT adapterIconSizeChanged();
    }
}

QRect ItemViewAdapter::adapterVisibleArea() const
{
    // qCDebug(albumLog) << "Getting adapter visible area:" << m_adapterVisibleArea;
    return m_adapterVisibleArea;
}

void ItemViewAdapter::setAdapterVisibleArea(QRect rect)
{
    // qCDebug(albumLog) << "ItemViewAdapter::setAdapterVisibleArea - Function entry, rect:" << rect;
    if (m_adapterVisibleArea != rect) {
        qCDebug(albumLog) << "Setting adapter visible area from" << m_adapterVisibleArea << "to" << rect;
        m_adapterVisibleArea = rect;

        Q_EMIT adapterVisibleAreaChanged();
    }
}
