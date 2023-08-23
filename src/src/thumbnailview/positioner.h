/*
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef POSITIONER_H
#define POSITIONER_H

#include <QAbstractItemModel>

class ThumbnailModel;
class QTimer;

class Positioner : public QAbstractItemModel
{
    Q_OBJECT

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(ThumbnailModel *thumbnailModel READ thumbnailModel WRITE setThumbnailModel NOTIFY sortModelChanged)

public:
    explicit Positioner(QObject *parent = nullptr);
    ~Positioner() override;

    bool enabled() const;
    void setEnabled(bool enabled);

    ThumbnailModel *thumbnailModel() const;
    void setThumbnailModel(ThumbnailModel *thumbnailModel);

    Q_INVOKABLE int map(int row) const;

    Q_INVOKABLE bool isBlank(int row) const;
    Q_INVOKABLE int indexForUrl(const QUrl &url) const;

    Q_INVOKABLE void setRangeSelected(int anchor, int to);

    Q_INVOKABLE void reset();

    QHash<int, QByteArray> roleNames() const override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

#ifdef BUILD_TESTING
    QHash<int, int> proxyToSourceMapping() const
    {
        return m_proxyToSource;
    }
    QHash<int, int> sourceToProxyMapping() const
    {
        return m_sourceToProxy;
    }
#endif

Q_SIGNALS:
    void enabledChanged() const;
    void sortModelChanged() const;
    void positionsChanged() const;

private Q_SLOTS:
    void sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);
    void sourceModelAboutToBeReset();
    void sourceModelReset();
    void sourceRowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void sourceRowsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow);
    void sourceRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void sourceLayoutAboutToBeChanged(const QList<QPersistentModelIndex> &parents, QAbstractItemModel::LayoutChangeHint hint);
    void sourceRowsInserted(const QModelIndex &parent, int first, int last);
    void sourceRowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow);
    void sourceRowsRemoved(const QModelIndex &parent, int first, int last);
    void sourceLayoutChanged(const QList<QPersistentModelIndex> &parents, QAbstractItemModel::LayoutChangeHint hint);

private:
    void initMaps(int size = -1);
    void updateMaps(int proxyIndex, int sourceIndex);
    int firstRow() const;
    int lastRow() const;
    int firstFreeRow() const;
    void flushPendingChanges();
    void connectSignals(ThumbnailModel *model);
    void disconnectSignals(ThumbnailModel *model);

    bool m_enabled;
    ThumbnailModel *m_thumbnialModel;

    int m_lastRow;

    QModelIndexList m_pendingChanges;
    bool m_ignoreNextTransaction;

    bool m_deferApplyPositions;
    QVariantList m_deferMovePositions;

    QHash<int, int> m_proxyToSource;
    QHash<int, int> m_sourceToProxy;
    bool m_beginInsertRowsCalled = false; // used to sync the amount of begin/endInsertRows calls
};

#endif
