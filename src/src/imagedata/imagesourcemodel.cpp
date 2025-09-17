// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "imagesourcemodel.h"
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
    qDebug() << "ImageSourceModel initialized";
}

ImageSourceModel::~ImageSourceModel() 
{
    qDebug() << "ImageSourceModel destroyed with" << imageUrlList.size() << "images";
}

/**
   @return 返回当前数据模型的数据类型及在QML中使用的别名
 */
QHash<int, QByteArray> ImageSourceModel::roleNames() const
{
    // qDebug() << "ImageSourceModel::roleNames - Entry";
    return {{Types::ImageUrlRole, "imageUrl"}};
}

/**
   @return 返回数据索引 \a index 和数据类型 \a role 所指向的数据
 */
QVariant ImageSourceModel::data(const QModelIndex &index, int role) const
{
    // qDebug() << "ImageSourceModel::data - Entry";
    if (!checkIndex(index, CheckIndexOption::ParentIsInvalid | CheckIndexOption::IndexIsValid)) {
        qWarning() << "Invalid index requested:" << index.row() << "role:" << role;
        return {};
    }

    switch (role) {
        case Types::ImageUrlRole:
            return imageUrlList.at(index.row());
        default:
            break;
    }

    // qDebug() << "ImageSourceModel::data - Exit";
    return {};
}

/**
   @brief 设置数据索引 \a index 和数据类型 \a role 所指向的数据为 \a value
   @return 是否设置数据成功
 */
bool ImageSourceModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    // qDebug() << "ImageSourceModel::setData - Entry";
    if (!checkIndex(index, CheckIndexOption::ParentIsInvalid | CheckIndexOption::IndexIsValid)) {
        qWarning() << "Invalid index for setData:" << index.row() << "role:" << role;
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

    // qDebug() << "ImageSourceModel::setData - Exit";
    return false;
}

/**
   @return 返回当前数据模型的总行数
 */
int ImageSourceModel::rowCount(const QModelIndex &parent) const
{
    // qDebug() << "ImageSourceModel::rowCount - Entry";
    Q_UNUSED(parent);
    return imageUrlList.count();
}

/**
   @return 返回传入文件路径 \a file 在数据模型中的索引，无此文件则返回 -1
 */
int ImageSourceModel::indexForImagePath(const QUrl &file)
{
    // qDebug() << "ImageSourceModel::indexForImagePath - Entry";
    if (file.isEmpty()) {
        qWarning() << "Empty file URL requested for index";
        return -1;
    }

    // qDebug() << "ImageSourceModel::indexForImagePath - Exit";
    return imageUrlList.indexOf(file);
}

/**
   @brief 设置图像文件列表，重置模型数据
 */
void ImageSourceModel::setImageFiles(const QList<QUrl> &files)
{
    // qDebug() << "ImageSourceModel::setImageFiles - Entry";
    beginResetModel();
    imageUrlList = files;
    endResetModel();
}

/**
   @brief 从数据模型中移除文件路径 \a fileName 指向的数据
 */
void ImageSourceModel::removeImage(const QUrl &fileName)
{
    // qDebug() << "ImageSourceModel::removeImage - Entry";
    int index = imageUrlList.indexOf(fileName);
    if (-1 != index) {
        qDebug() << "Removing image at index" << index << ":" << fileName;
        beginRemoveRows(QModelIndex(), index, index);
        imageUrlList.removeAt(index);
        endRemoveRows();
        qDebug() << "Image removed, new count:" << imageUrlList.size();
    } else {
        qWarning() << "Attempted to remove non-existent image:" << fileName;
    }
    // qDebug() << "ImageSourceModel::removeImage - Exit";
}
