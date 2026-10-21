// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "imagesourcemodel.h"
#include "ddlog.h"
#include <QDebug>

/**
   @class ImageSourceModel
   @brief 图片数据模型，提供缩略图/图片展示的图片数据信息。

   @note 此数据模型仅存储需进行展示的图像文件 url 路径列表，
    详细的图像文件信息使用 ImageInfo 进行获取。
 */

ImageSourceModel::ImageSourceModel(QObject *parent)
    : QAbstractListModel(parent)
{
    qCDebug(albumLog) << "ImageSourceModel initialized";
}

ImageSourceModel::~ImageSourceModel() 
{
    qCDebug(albumLog) << "ImageSourceModel destroyed with" << imageUrlList.size() << "images";
}

/**
   @return 返回当前数据模型的数据类型及在QML中使用的别名
 */
QHash<int, QByteArray> ImageSourceModel::roleNames() const
{
    // qCDebug(albumLog) << "ImageSourceModel::roleNames - Entry";
    return {{Types::ImageUrlRole, "imageUrl"}};
}

/**
   @return 返回数据索引 \a index 和数据类型 \a role 所指向的数据
 */
QVariant ImageSourceModel::data(const QModelIndex &index, int role) const
{
    // qCDebug(albumLog) << "ImageSourceModel::data - Entry";
    if (!checkIndex(index, CheckIndexOption::ParentIsInvalid | CheckIndexOption::IndexIsValid)) {
        qCWarning(albumLog) << "Invalid index requested:" << index.row() << "role:" << role;
        return {};
    }

    switch (role) {
        case Types::ImageUrlRole:
            return imageUrlList.at(index.row());
        default:
            break;
    }

    // qCDebug(albumLog) << "ImageSourceModel::data - Exit";
    return {};
}

/**
   @brief 设置数据索引 \a index 和数据类型 \a role 所指向的数据为 \a value
   @return 是否设置数据成功
 */
bool ImageSourceModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    // qCDebug(albumLog) << "ImageSourceModel::setData - Entry";
    if (!checkIndex(index, CheckIndexOption::ParentIsInvalid | CheckIndexOption::IndexIsValid)) {
        qCWarning(albumLog) << "Invalid index for setData:" << index.row() << "role:" << role;
        return false;
    }

    switch (role) {
        case Types::ImageUrlRole:
            imageUrlList.replace(index.row(), value.toUrl());

            Q_EMIT dataChanged(index, index);
            return true;
        default:
            break;
    }

    // qCDebug(albumLog) << "ImageSourceModel::setData - Exit";
    return false;
}

/**
   @return 返回当前数据模型的总行数
 */
int ImageSourceModel::rowCount(const QModelIndex &parent) const
{
    // qCDebug(albumLog) << "ImageSourceModel::rowCount - Entry";
    Q_UNUSED(parent);
    return imageUrlList.count();
}

/**
   @return 返回传入文件路径 \a file 在数据模型中的索引，无此文件则返回 -1
 */
int ImageSourceModel::indexForImagePath(const QUrl &file) const
{
    // qCDebug(albumLog) << "ImageSourceModel::indexForImagePath - Entry";
    if (file.isEmpty()) {
        qCWarning(albumLog) << "Empty file URL requested for index";
        return -1;
    }

    // qCDebug(albumLog) << "ImageSourceModel::indexForImagePath - Exit";
    return imageUrlList.indexOf(file);
}

bool ImageSourceModel::containsImagePath(const QUrl &file) const
{
    return !file.isEmpty() && imageUrlList.contains(file);
}

QStringList ImageSourceModel::imageUrlStrings() const
{
    QStringList result;
    result.reserve(imageUrlList.size());
    for (const QUrl &url : imageUrlList)
        result << url.toString();
    return result;
}

/**
   @brief 设置图像文件列表，重置模型数据
 */
void ImageSourceModel::setImageFiles(const QList<QUrl> &files)
{
    // qCDebug(albumLog) << "ImageSourceModel::setImageFiles - Entry";
    beginResetModel();
    imageUrlList = files;
    endResetModel();
}

/**
   @brief 从数据模型中移除文件路径 \a fileName 指向的数据
 */
void ImageSourceModel::removeImage(const QUrl &fileName)
{
    // qCDebug(albumLog) << "ImageSourceModel::removeImage - Entry";
    int index = imageUrlList.indexOf(fileName);
    if (-1 != index) {
        qCDebug(albumLog) << "Removing image at index" << index << ":" << fileName;
        beginRemoveRows(QModelIndex(), index, index);
        imageUrlList.removeAt(index);
        endRemoveRows();
        qCDebug(albumLog) << "Image removed, new count:" << imageUrlList.size();
    } else {
        qCWarning(albumLog) << "Attempted to remove non-existent image:" << fileName;
    }
    // qCDebug(albumLog) << "ImageSourceModel::removeImage - Exit";
}
