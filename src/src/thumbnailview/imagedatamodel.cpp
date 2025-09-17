// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "imagedatamodel.h"
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
{
    qDebug() << "Initializing ImageDataModel";
    connect(AlbumControl::instance(), &AlbumControl::deviceAlbumInfoLoadFinished, this, &ImageDataModel::onDeviceDataLoaded);
}

QHash<int, QByteArray> ImageDataModel::roleNames() const
{
    qDebug() << "Getting role names for model";
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
    // qDebug() << "ImageDataModel::data - Entry";
    if (!index.isValid()) {
        qWarning() << "Invalid index requested:" << index.row() << "role:" << role;
        return {};
    }

    const DBImgInfo &info = m_infoList.at(index.row());

    switch (role) {
    case Qt::DisplayRole: {
        // qDebug() << "role is Qt::DisplayRole";
        return QVariant::fromValue(info);
    }

    case Roles::FileNameRole: {
        // qDebug() << "role is Roles::FileNameRole";
        return QUrl::fromLocalFile(info.filePath).fileName();
    }

    case Roles::UrlRole: {
        // qDebug() << "role is Roles::UrlRole";
        QString filePath = "";
        if (Types::RecentlyDeleted == m_modelType) {
            filePath = AlbumControl::instance()->getDeleteFullPath(LibUnionImage_NameSpace::hashByString(info.filePath), DBImgInfo::getFileNameFromFilePath(info.filePath));
            qDebug() << "Getting URL for deleted file:" << filePath;
        } else {
            filePath = info.filePath;
        }
        return QUrl::fromLocalFile(filePath).toString();
    }

    case Roles::FilePathRole: {
        // qDebug() << "role is Roles::FilePathRole";
        return info.filePath;
    }

    case Roles::PathHashRole: {
        // qDebug() << "role is Roles::PathHashRole";
        return info.pathHash;
    }

    case Roles::RemainDaysRole: {
        // qDebug() << "role is Roles::RemainDaysRole";
        return info.remainDays;
    }

    case Roles::ItemTypeRole: {
        // qDebug() << "role is Roles::ItemTypeRole";
        if (info.itemType == ItemTypePic) {
            return "picture";
        } else if (info.itemType == ItemTypeVideo) {
            return "video";
        } else {
            return "other";
        }
    }

    case Roles::ItemTypeFlagRole: {
        // qDebug() << "role is Roles::ItemTypeFlagRole";
        return info.itemType;
    }
    }

    // qWarning() << "Unknown role requested:" << role;
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
    qDebug() << "Setting model type from" << m_modelType << "to" << modelType;
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
        qDebug() << "Setting album ID from" << m_albumID << "to" << albumId;
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
        qDebug() << "Setting keyword from" << m_keyWord << "to" << keyWord;
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
        qDebug() << "Setting device path from" << m_devicePath << "to" << devicePath;
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
        qDebug() << "Setting day token from" << m_dayToken << "to" << dayToken;
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
        qDebug() << "Setting import title from" << m_importTitle << "to" << importTitle;
        emit importTitleChanged();
    }

    m_importTitle = importTitle;
}

DBImgInfo ImageDataModel::dataForIndex(const QModelIndex &index) const
{
    if (!index.isValid()) {
        qWarning() << "Invalid index requested for data";
        return DBImgInfo();
    }

    return m_infoList.at(index.row());
}

void ImageDataModel::loadData(Types::ItemType type)
{
    QElapsedTimer time;
    time.start();
    qDebug() << "Loading data for type:" << type << "model type:" << m_modelType;
    
    m_loadType = ItemTypeNull;
    if (type == Types::All)
        m_loadType = ItemTypeNull;
    else if (type == Types::Picture)
        m_loadType = ItemTypePic;
    else if (type == Types::Video)
        m_loadType = ItemTypeVideo;

    beginResetModel();
    if (m_modelType == Types::AllCollection) {
        qDebug() << "Loading all collection data";
        m_infoList = DBManager::instance()->getAllInfosSort(m_loadType);
    } else if (m_modelType == Types::CustomAlbum) {
        qDebug() << "Loading custom album data for album ID:" << m_albumID;
        m_infoList = DBManager::instance()->getInfosByAlbum(m_albumID, false, m_loadType);
    } else if (m_modelType == Types::RecentlyDeleted) {
        qDebug() << "Loading recently deleted data";
        m_infoList = AlbumControl::instance()->getTrashInfos2(m_loadType);
    } else if (m_modelType == Types::Device) {
        qDebug() << "Loading device data for path:" << m_devicePath;
        bool waiting = false;
        m_infoList = AlbumControl::instance()->getDeviceAlbumInfoList(m_devicePath, m_loadType, &waiting);
        if (waiting) {
            m_infoList.clear();
            qDebug() << "Device data not ready, refresh later";
        }
    } else if (m_modelType == Types::SearchResult) {
        qDebug() << "Loading search results for keyword:" << m_keyWord << "in album:" << m_albumID;
        m_infoList = AlbumControl::instance()->searchPicFromAlbum2(m_albumID, m_keyWord, false);
    } else if (m_modelType == Types::DayCollecttion) {
        qDebug() << "Loading day collection data for token:" << m_dayToken;
        m_infoList = DBManager::instance()->getInfosByDay(m_dayToken);
    } else if (m_modelType == Types::HaveImported) {
        qDebug() << "Loading imported data for title:" << m_importTitle;
        m_infoList = DBManager::instance()->getInfosByImportTimeline(QDateTime::fromString(m_importTitle, "yyyy/MM/dd hh:mm"), m_loadType);
    }
    endResetModel();
    qDebug() << QString("loadData modelType:[%1] cost [%2]ms, loaded [%3] items").arg(m_modelType).arg(time.elapsed()).arg(m_infoList.size());
}

void ImageDataModel::onDeviceDataLoaded(QString devicePath)
{
    if (devicePath != m_devicePath) {
        qDebug() << "Ignoring device data load for different path:" << devicePath;
        return;
    }

    qDebug() << "Refreshing model with device data for path:" << devicePath;
    beginResetModel();
    m_infoList = AlbumControl::instance()->getDeviceAlbumInfoList(m_devicePath, m_loadType);
    endResetModel();

    qDebug() << "Device data ready, refreshed model with" << m_infoList.size() << "items";
}
