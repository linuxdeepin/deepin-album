// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A minimal, dependency-free source model that mimics the parts of
// ImageDataModel that ThumbnailModel / Positioner rely on. It stands in for
// the real model so unit tests never need the DB / image-engine stack.
//
//  - Qt::DisplayRole returns an EMPTY value here, which keeps
//    QSortFilterProxyModel::sort(0) (sort role = DisplayRole) from reordering:
//    every key is equal, so the source row order is preserved.
//  - UrlRole / FilePathRole / ItemTypeFlagRole / FileNameRole expose the same
//    Roles::* enum names that the production data()/roleNames() use.
#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QString>
#include <QUrl>
#include <QVariant>

#include "types.h"        // ItemTypePic / ItemTypeVideo (global, via unionimage_global.h)
#include "thumbnailview/roles.h"

class FakeSourceModel : public QAbstractListModel
{
public:
    struct Item {
        QString url;
        QString path;
        int itemTypeFlag = ItemTypePic;   // ItemTypePic by default
        QString fileName;
    };

    explicit FakeSourceModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : m_items.size();
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
            return QVariant();

        const Item &item = m_items.at(index.row());
        switch (role) {
        case Qt::DisplayRole:
            return QString();                          // keep proxy sort stable
        case Roles::UrlRole:
            return item.url;
        case Roles::FilePathRole:
            return item.path;
        case Roles::ItemTypeFlagRole:
            return item.itemTypeFlag;
        case Roles::FileNameRole:
            return item.fileName;
        default:
            return QVariant();
        }
    }

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> hash;
        hash.insert(Roles::UrlRole, "url");
        hash.insert(Roles::FilePathRole, "filePath");
        hash.insert(Roles::ItemTypeFlagRole, "itemTypeFlag");
        hash.insert(Roles::FileNameRole, "fileName");
        return hash;
    }

    // ---- mutations that emit proper model signals ----
    void setItems(const QList<Item> &items)
    {
        beginResetModel();
        m_items = items;
        endResetModel();
    }

    void insertItem(int row, const Item &item)
    {
        beginInsertRows(QModelIndex(), row, row);
        m_items.insert(row, item);
        endInsertRows();
    }

    void appendItem(const Item &item) { insertItem(m_items.size(), item); }

    const Item &itemAt(int row) const { return m_items.at(row); }

private:
    QList<Item> m_items;
};

// Helpers to build fake items quickly.
inline FakeSourceModel::Item utFakePic(const QString &url, const QString &path)
{
    FakeSourceModel::Item it;
    it.url = url;
    it.path = path;
    it.itemTypeFlag = ItemTypePic;
    it.fileName = QUrl(url).fileName();
    return it;
}

inline FakeSourceModel::Item utFakeVideo(const QString &url, const QString &path)
{
    FakeSourceModel::Item it;
    it.url = url;
    it.path = path;
    it.itemTypeFlag = ItemTypeVideo;
    it.fileName = QUrl(url).fileName();
    return it;
}

inline QString utFakeUrl(int i)
{
    return QStringLiteral("file:///p%1.jpg").arg(i);
}
