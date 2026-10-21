// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "imagedatamodel.h"
#include "ddlog.h"
#include "roles.h"
#include "dbmanager/dbmanager.h"
#include "albumControl.h"
#include <QDebug>

#include <QUrl>

ImageDataModel::ImageDataModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_modelType(Types::ModelType::Normal)
    , m_albumID(-1)
    , m_keyWord("")
    , m_dayToken("")
    , m_className("")
{
    qCDebug(albumLog) << "Initializing ImageDataModel";
    connect(AlbumControl::instance(), &AlbumControl::deviceAlbumInfoLoadFinished, this, &ImageDataModel::onDeviceDataLoaded);
}

QHash<int, QByteArray> ImageDataModel::roleNames() const
{
    qCDebug(albumLog) << "Getting role names for model";
    auto hash = QAbstractItemModel::roleNames();
    // the url role returns the url of the cover image of the collection
    hash.insert(Roles::ItemTypeRole, "itemType");

    hash.insert(Roles::FileNameRole, "fileName");
    hash.insert(Roles::UrlRole, "url");
    hash.insert(Roles::FilePathRole, "filePath");
    hash.insert(Roles::PathHashRole, "pathHash");
    hash.insert(Roles::RemainDaysRole, "remainDays");

    return hash;
}

QVariant ImageDataModel::data(const QModelIndex &index, int role) const
{
    // qCDebug(albumLog) << "ImageDataModel::data - Entry";
    if (!index.isValid()) {
        qCWarning(albumLog) << "Invalid index requested:" << index.row() << "role:" << role;
        return {};
    }

    const DBImgInfo &info = m_infoList.at(index.row());

    switch (role) {
    case Qt::DisplayRole: {
        // qCDebug(albumLog) << "role is Qt::DisplayRole";
        return QVariant::fromValue(info);
    }

    case Roles::FileNameRole: {
        // qCDebug(albumLog) << "role is Roles::FileNameRole";
        return QUrl::fromLocalFile(info.filePath).fileName();
    }

    case Roles::UrlRole: {
        // qCDebug(albumLog) << "role is Roles::UrlRole";
        QString filePath = "";
        if (Types::RecentlyDeleted == m_modelType) {
            filePath = AlbumControl::instance()->getDeleteFullPath(LibUnionImage_NameSpace::hashByString(info.filePath), DBImgInfo::getFileNameFromFilePath(info.filePath));
            qCDebug(albumLog) << "Getting URL for deleted file:" << filePath;
        } else {
            filePath = info.filePath;
        }
        return QUrl::fromLocalFile(filePath).toString();
    }

    case Roles::FilePathRole: {
        // qCDebug(albumLog) << "role is Roles::FilePathRole";
        return info.filePath;
    }

    case Roles::PathHashRole: {
        // qCDebug(albumLog) << "role is Roles::PathHashRole";
        return info.pathHash;
    }

    case Roles::RemainDaysRole: {
        // qCDebug(albumLog) << "role is Roles::RemainDaysRole";
        return info.remainDays;
    }

    case Roles::ItemTypeRole: {
        // qCDebug(albumLog) << "role is Roles::ItemTypeRole";
        if (info.itemType == ItemTypePic) {
            return "picture";
        } else if (info.itemType == ItemTypeVideo) {
            return "video";
        } else {
            return "other";
        }
    }

    case Roles::ItemTypeFlagRole: {
        // qCDebug(albumLog) << "role is Roles::ItemTypeFlagRole";
        return info.itemType;
    }
    }

    // qCWarning(albumLog) << "Unknown role requested:" << role;
    return {};
}

int ImageDataModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_infoList.size();
}

Types::ModelType ImageDataModel::modelType() const
{
    return m_modelType;
}

void ImageDataModel::setModelType(Types::ModelType modelType)
{
    qCDebug(albumLog) << "Setting model type from" << m_modelType << "to" << modelType;
    beginResetModel();
    m_modelType = modelType;
    endResetModel();

    emit modelTypeChanged();
}

int ImageDataModel::albumId() const
{
    return m_albumID;
}

void ImageDataModel::setAlbumId(int albumId)
{
    if (albumId != m_albumID) {
        qCDebug(albumLog) << "Setting album ID from" << m_albumID << "to" << albumId;
        emit albumIdChanged();
    }

    m_albumID = albumId;
}

QString ImageDataModel::keyWord()
{
    return m_keyWord;
}

void ImageDataModel::setKeyWord(QString keyWord)
{
    if (keyWord != m_keyWord) {
        qCDebug(albumLog) << "Setting keyword from" << m_keyWord << "to" << keyWord;
        emit keyWordChanged();
    }

    m_keyWord = keyWord;
}

QString ImageDataModel::devicePath()
{
    return m_devicePath;
}

void ImageDataModel::setDevicePath(QString devicePath)
{
    if (m_devicePath != devicePath) {
        qCDebug(albumLog) << "Setting device path from" << m_devicePath << "to" << devicePath;
        emit devicePathChanged();
    }

    m_devicePath = devicePath;
}

QString ImageDataModel::dayToken()
{
    return m_dayToken;
}

void ImageDataModel::setDayToken(QString dayToken)
{
    if (m_dayToken != dayToken) {
        qCDebug(albumLog) << "Setting day token from" << m_dayToken << "to" << dayToken;
        emit dayTokenChanged();
    }

    m_dayToken = dayToken;
}

QString ImageDataModel::importTitle()
{
    return m_importTitle;
}

void ImageDataModel::setImportTitle(QString importTitle)
{
    if (m_importTitle != importTitle) {
        qCDebug(albumLog) << "Setting import title from" << m_importTitle << "to" << importTitle;
        emit importTitleChanged();
    }

    m_importTitle = importTitle;
}

QString ImageDataModel::className()
{
    return m_className;
}

void ImageDataModel::setClassName(QString className)
{
    if (m_className != className) {
        qCDebug(albumLog) << "Setting class name from" << m_className << "to" << className;
        emit classNameChanged();
    }

    m_className = className;
}

DBImgInfo ImageDataModel::dataForIndex(const QModelIndex &index) const
{
    if (!index.isValid()) {
        qCWarning(albumLog) << "Invalid index requested for data";
        return DBImgInfo();
    }

    return m_infoList.at(index.row());
}

void ImageDataModel::loadData(Types::ItemType type)
{
    QElapsedTimer time;
    time.start();
    qCDebug(albumLog) << "Loading data for type:" << type << "model type:" << m_modelType;

    m_loadType = ItemTypeNull;
    if (type == Types::All)
        m_loadType = ItemTypeNull;
    else if (type == Types::Picture)
        m_loadType = ItemTypePic;
    else if (type == Types::Video)
        m_loadType = ItemTypeVideo;

    beginResetModel();
    if (m_modelType == Types::AllCollection) {
        qCDebug(albumLog) << "Loading all collection data";
        m_infoList = DBManager::instance()->getAllInfosSort(m_loadType);
    } else if (m_modelType == Types::CustomAlbum) {
        qCDebug(albumLog) << "Loading custom album data for album ID:" << m_albumID;
        m_infoList = DBManager::instance()->getInfosByAlbum(m_albumID, false, m_loadType);
    } else if (m_modelType == Types::RecentlyDeleted) {
        qCDebug(albumLog) << "Loading recently deleted data";
        m_infoList = AlbumControl::instance()->getTrashInfos2(m_loadType);
    } else if (m_modelType == Types::Device) {
        qCDebug(albumLog) << "Loading device data for path:" << m_devicePath;
        bool waiting = false;
        m_infoList = AlbumControl::instance()->getDeviceAlbumInfoList(m_devicePath, m_loadType, &waiting);
        if (waiting) {
            m_infoList.clear();
            qCDebug(albumLog) << "Device data not ready, refresh later";
        }
    } else if (m_modelType == Types::SearchResult) {
        qCDebug(albumLog) << "Loading search results for keyword:" << m_keyWord << "in album:" << m_albumID;
        m_infoList = AlbumControl::instance()->searchPicFromAlbum2(m_albumID, m_keyWord, false);
    } else if (m_modelType == Types::DayCollecttion) {
        qCDebug(albumLog) << "Loading day collection data for token:" << m_dayToken;
        m_infoList = DBManager::instance()->getInfosByDay(m_dayToken);
    } else if (m_modelType == Types::HaveImported) {
        qCDebug(albumLog) << "Loading imported data for title:" << m_importTitle;
        m_infoList = DBManager::instance()->getInfosByImportTimeline(QDateTime::fromString(m_importTitle, "yyyy/MM/dd hh:mm"), m_loadType);
    } else if (m_modelType == Types::ClassificationDetail) {
        qCDebug(albumLog) << "Loading classification detail data for " << m_className;
        m_infoList = DBManager::instance()->getInfosForClass(m_className);
    }
    endResetModel();
    qCDebug(albumLog) << QString("loadData modelType:[%1] cost [%2]ms, loaded [%3] items").arg(m_modelType).arg(time.elapsed()).arg(m_infoList.size());
}

int ImageDataModel::itemCount() const
{
    return m_infoList.size();
}

QString ImageDataModel::urlAt(int row) const
{
    if (row < 0 || row >= m_infoList.size())
        return {};

    return QUrl::fromLocalFile(m_infoList.at(row).filePath).toString();
}

void ImageDataModel::onDeviceDataLoaded(QString devicePath)
{
    if (devicePath != m_devicePath) {
        qCDebug(albumLog) << "Ignoring device data load for different path:" << devicePath;
        return;
    }

    qCDebug(albumLog) << "Refreshing model with device data for path:" << devicePath;
    beginResetModel();
    m_infoList = AlbumControl::instance()->getDeviceAlbumInfoList(m_devicePath, m_loadType);
    endResetModel();

    qCDebug(albumLog) << "Device data ready, refreshed model with" << m_infoList.size() << "items";
}
