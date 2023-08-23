/*
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "positioner.h"
#include "thumbnailmodel.h"

#include <QTimer>
#include <QPoint>
#include <QUrl>

#include <cstdlib>

Positioner::Positioner(QObject *parent)
    : QAbstractItemModel(parent)
    , m_enabled(false)
    , m_thumbnialModel(nullptr)
    , m_ignoreNextTransaction(false)
    , m_deferApplyPositions(false)
{
}

Positioner::~Positioner()
{
}

bool Positioner::enabled() const
{
    return m_enabled;
}

void Positioner::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;

        beginResetModel();

        if (enabled && m_thumbnialModel) {
            initMaps();
        }

        endResetModel();

        Q_EMIT enabledChanged();
    }
}

ThumbnailModel *Positioner::thumbnailModel() const
{
    return m_thumbnialModel;
}

void Positioner::setThumbnailModel(ThumbnailModel *thumbnailModel)
{
    if (m_thumbnialModel != thumbnailModel) {
        beginResetModel();

        if (m_thumbnialModel) {
            disconnectSignals(m_thumbnialModel);
        }

        m_thumbnialModel = thumbnailModel;

        if (m_thumbnialModel) {
            connectSignals(m_thumbnialModel);

            if (m_enabled) {
                initMaps();
            }
        }

        endResetModel();

        Q_EMIT sortModelChanged();
    }
}

int Positioner::map(int row) const
{
    if (m_enabled && m_thumbnialModel) {
        return m_proxyToSource.value(row, -1);
    }

    return row;
}

bool Positioner::isBlank(int row) const
{
//    if (!m_enabled && m_thumbnailModel) {
//        return m_thumbnailModel->isBlank(row);
//    }

//    if (m_proxyToSource.contains(row) && m_thumbnailModel && !m_thumbnailModel->isBlank(m_proxyToSource.value(row))) {
//        return false;
//    }

    return false;
}

int Positioner::indexForUrl(const QUrl &url) const
{
    if (!m_thumbnialModel) {
        return -1;
    }

    const QString &name = url.fileName();

    int sourceIndex = -1;

    // TODO Optimize.
    for (int i = 0; i < m_thumbnialModel->rowCount(); ++i) {
        if (m_thumbnialModel->data(m_thumbnialModel->index(i, 0), Roles::FileNameRole).toString() == name) {
            sourceIndex = i;

            break;
        }
    }

    return m_sourceToProxy.value(sourceIndex, -1);
}

void Positioner::setRangeSelected(int anchor, int to)
{
    if (!m_thumbnialModel) {
        return;
    }

    if (m_enabled) {
        QVariantList indices;

        for (int i = qMin(anchor, to); i <= qMax(anchor, to); ++i) {
            if (m_proxyToSource.contains(i)) {
                indices.append(m_proxyToSource.value(i));
            }
        }

        if (!indices.isEmpty()) {
            m_thumbnialModel->updateSelection(indices, false);
        }
    } else {
        m_thumbnialModel->setRangeSelected(anchor, to);
    }
}

QHash<int, QByteArray> Positioner::roleNames() const
{
    return m_thumbnialModel->roleNames();
}

QModelIndex Positioner::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return QModelIndex();
    }

    return createIndex(row, column);
}

QModelIndex Positioner::parent(const QModelIndex &index) const
{
    if (m_thumbnialModel) {
        m_thumbnialModel->parent(index);
    }

    return QModelIndex();
}

QVariant Positioner::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (m_thumbnialModel) {
        if (m_enabled) {
            if (m_proxyToSource.contains(index.row())) {
                return m_thumbnialModel->data(m_thumbnialModel->index(m_proxyToSource.value(index.row()), 0), role);
            } else if (role == Roles::BlankRole) {
                return true;
            }
        } else {
            return m_thumbnialModel->data(m_thumbnialModel->index(index.row(), 0), role);
        }
    }

    return QVariant();
}

int Positioner::rowCount(const QModelIndex &parent) const
{
    if (m_thumbnialModel) {
        if (m_enabled) {
            if (parent.isValid()) {
                return 0;
            } else {
                return lastRow() + 1;
            }
        } else {
            return m_thumbnialModel->rowCount(parent);
        }
    }

    return 0;
}

int Positioner::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    if (m_thumbnialModel) {
        return 1;
    }

    return 0;
}

void Positioner::reset()
{
    beginResetModel();

    initMaps();

    endResetModel();

    Q_EMIT positionsChanged();
}

void Positioner::sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    if (m_enabled) {
        int start = topLeft.row();
        int end = bottomRight.row();

        for (int i = start; i <= end; ++i) {
            if (m_sourceToProxy.contains(i)) {
                const QModelIndex &idx = index(m_sourceToProxy.value(i), 0);

                Q_EMIT dataChanged(idx, idx);
            }
        }
    } else {
        Q_EMIT dataChanged(topLeft, bottomRight, roles);
    }
}

void Positioner::sourceModelAboutToBeReset()
{
    beginResetModel();
}

void Positioner::sourceModelReset()
{
    if (m_enabled) {
        initMaps();
    }

    endResetModel();
}

void Positioner::sourceRowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
    if (m_enabled) {
        // Don't insert yet if we're waiting for listing to complete to apply
        // initial positions;
        if (m_deferApplyPositions) {
            return;
        } else if (m_proxyToSource.isEmpty()) {
            beginInsertRows(parent, start, end);
            m_beginInsertRowsCalled = true;

            initMaps(end + 1);

            return;
        }

        // When new rows are inserted, they might go in the beginning or in the middle.
        // In this case we must update first the existing proxy->source and source->proxy
        // mapping, otherwise the proxy items will point to the wrong source item.
        int count = end - start + 1;
        m_sourceToProxy.clear();
        for (auto it = m_proxyToSource.begin(); it != m_proxyToSource.end(); ++it) {
            int sourceIdx = *it;
            if (sourceIdx >= start) {
                *it += count;
            }
            m_sourceToProxy[*it] = it.key();
        }

        int free = -1;
        int rest = -1;

        for (int i = start; i <= end; ++i) {
            free = firstFreeRow();

            if (free != -1) {
                updateMaps(free, i);
                m_pendingChanges << createIndex(free, 0);
            } else {
                rest = i;
                break;
            }
        }

        if (rest != -1) {
            int firstNew = lastRow() + 1;
            int remainder = (end - rest);

            beginInsertRows(parent, firstNew, firstNew + remainder);
            m_beginInsertRowsCalled = true;

            for (int i = 0; i <= remainder; ++i) {
                updateMaps(firstNew + i, rest + i);
            }
        } else {
            m_ignoreNextTransaction = true;
        }
    } else {
        beginInsertRows(parent, start, end);
        beginInsertRows(parent, start, end);
        m_beginInsertRowsCalled = true;
    }
}

void Positioner::sourceRowsAboutToBeMoved(const QModelIndex &sourceParent,
                                          int sourceStart,
                                          int sourceEnd,
                                          const QModelIndex &destinationParent,
                                          int destinationRow)
{
    beginMoveRows(sourceParent, sourceStart, sourceEnd, destinationParent, destinationRow);
}

void Positioner::sourceRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last)
{
    if (m_enabled) {
        int oldLast = lastRow();

        for (int i = first; i <= last; ++i) {
            int proxyRow = m_sourceToProxy.take(i);
            m_proxyToSource.remove(proxyRow);
            m_pendingChanges << createIndex(proxyRow, 0);
        }

        QHash<int, int> newProxyToSource;
        QHash<int, int> newSourceToProxy;
        QHashIterator<int, int> it(m_sourceToProxy);
        int delta = std::abs(first - last) + 1;

        while (it.hasNext()) {
            it.next();

            if (it.key() > last) {
                newProxyToSource.insert(it.value(), it.key() - delta);
                newSourceToProxy.insert(it.key() - delta, it.value());
            } else {
                newProxyToSource.insert(it.value(), it.key());
                newSourceToProxy.insert(it.key(), it.value());
            }
        }

        m_proxyToSource = newProxyToSource;
        m_sourceToProxy = newSourceToProxy;

        int newLast = lastRow();

        if (oldLast > newLast) {
            int diff = oldLast - newLast;
            beginRemoveRows(QModelIndex(), ((oldLast - diff) + 1), oldLast);
        } else {
            m_ignoreNextTransaction = true;
        }
    } else {
        beginRemoveRows(parent, first, last);
    }
}

void Positioner::sourceLayoutAboutToBeChanged(const QList<QPersistentModelIndex> &parents, QAbstractItemModel::LayoutChangeHint hint)
{
    Q_UNUSED(parents)

    Q_EMIT layoutAboutToBeChanged(QList<QPersistentModelIndex>(), hint);
}

void Positioner::sourceRowsInserted(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)

    if (!m_ignoreNextTransaction) {
        if (m_beginInsertRowsCalled) {
            endInsertRows();
            m_beginInsertRowsCalled = false;
        }
    } else {
        m_ignoreNextTransaction = false;
    }

    flushPendingChanges();
}

void Positioner::sourceRowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow)
{
    Q_UNUSED(sourceParent)
    Q_UNUSED(sourceStart)
    Q_UNUSED(sourceEnd)
    Q_UNUSED(destinationParent)
    Q_UNUSED(destinationRow)

    endMoveRows();
}

void Positioner::sourceRowsRemoved(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)

    if (!m_ignoreNextTransaction) {
        Q_EMIT endRemoveRows();
    } else {
        m_ignoreNextTransaction = false;
    }

    flushPendingChanges();
}

void Positioner::sourceLayoutChanged(const QList<QPersistentModelIndex> &parents, QAbstractItemModel::LayoutChangeHint hint)
{
    Q_UNUSED(parents)

    if (m_enabled) {
        initMaps();
    }

    Q_EMIT layoutChanged(QList<QPersistentModelIndex>(), hint);
}

void Positioner::initMaps(int size)
{
    m_proxyToSource.clear();
    m_sourceToProxy.clear();

    if (size == -1) {
        size = m_thumbnialModel->rowCount();
    }

    if (!size) {
        return;
    }

    for (int i = 0; i < size; ++i) {
        updateMaps(i, i);
    }
}

void Positioner::updateMaps(int proxyIndex, int sourceIndex)
{
    m_proxyToSource.insert(proxyIndex, sourceIndex);
    m_sourceToProxy.insert(sourceIndex, proxyIndex);
}

int Positioner::firstRow() const
{
    if (!m_proxyToSource.isEmpty()) {
        QList<int> keys(m_proxyToSource.keys());
        std::sort(keys.begin(), keys.end());

        return keys.first();
    }

    return -1;
}

int Positioner::lastRow() const
{
    if (!m_proxyToSource.isEmpty()) {
        QList<int> keys(m_proxyToSource.keys());
        std::sort(keys.begin(), keys.end());
        return keys.last();
    }

    return 0;
}

int Positioner::firstFreeRow() const
{
    if (!m_proxyToSource.isEmpty()) {
        int last = lastRow();

        for (int i = 0; i <= last; ++i) {
            if (!m_proxyToSource.contains(i)) {
                return i;
            }
        }
    }

    return -1;
}

void Positioner::flushPendingChanges()
{
    if (m_pendingChanges.isEmpty()) {
        return;
    }

    int last = lastRow();

    for (const QModelIndex &index : m_pendingChanges) {
        if (index.row() <= last) {
            Q_EMIT dataChanged(index, index);
        }
    }

    m_pendingChanges.clear();
}

void Positioner::connectSignals(ThumbnailModel *model)
{
    connect(model, &QAbstractItemModel::dataChanged, this, &Positioner::sourceDataChanged, Qt::UniqueConnection);
    connect(model, &QAbstractItemModel::rowsAboutToBeInserted, this, &Positioner::sourceRowsAboutToBeInserted, Qt::UniqueConnection);
    connect(model, &QAbstractItemModel::rowsAboutToBeMoved, this, &Positioner::sourceRowsAboutToBeMoved, Qt::UniqueConnection);
    connect(model, &QAbstractItemModel::rowsAboutToBeRemoved, this, &Positioner::sourceRowsAboutToBeRemoved, Qt::UniqueConnection);
    connect(model, &QAbstractItemModel::layoutAboutToBeChanged, this, &Positioner::sourceLayoutAboutToBeChanged, Qt::UniqueConnection);
    connect(model, &QAbstractItemModel::rowsInserted, this, &Positioner::sourceRowsInserted, Qt::UniqueConnection);
    connect(model, &QAbstractItemModel::rowsMoved, this, &Positioner::sourceRowsMoved, Qt::UniqueConnection);
    connect(model, &QAbstractItemModel::rowsRemoved, this, &Positioner::sourceRowsRemoved, Qt::UniqueConnection);
    connect(model, &QAbstractItemModel::layoutChanged, this, &Positioner::sourceLayoutChanged, Qt::UniqueConnection);
    connect(m_thumbnialModel, &ThumbnailModel::srcModelReseted, this, &Positioner::reset, Qt::UniqueConnection);
}

void Positioner::disconnectSignals(ThumbnailModel *model)
{
    disconnect(model, &QAbstractItemModel::dataChanged, this, &Positioner::sourceDataChanged);
    disconnect(model, &QAbstractItemModel::rowsAboutToBeInserted, this, &Positioner::sourceRowsAboutToBeInserted);
    disconnect(model, &QAbstractItemModel::rowsAboutToBeMoved, this, &Positioner::sourceRowsAboutToBeMoved);
    disconnect(model, &QAbstractItemModel::rowsAboutToBeRemoved, this, &Positioner::sourceRowsAboutToBeRemoved);
    disconnect(model, &QAbstractItemModel::layoutAboutToBeChanged, this, &Positioner::sourceLayoutAboutToBeChanged);
    disconnect(model, &QAbstractItemModel::rowsInserted, this, &Positioner::sourceRowsInserted);
    disconnect(model, &QAbstractItemModel::rowsMoved, this, &Positioner::sourceRowsMoved);
    disconnect(model, &QAbstractItemModel::rowsRemoved, this, &Positioner::sourceRowsRemoved);
    disconnect(model, &QAbstractItemModel::layoutChanged, this, &Positioner::sourceLayoutChanged);
    disconnect(m_thumbnialModel, &ThumbnailModel::srcModelReseted, this, &Positioner::reset);
}
