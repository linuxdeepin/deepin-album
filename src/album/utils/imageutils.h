// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IMGUTIL_H
#define IMGUTIL_H

#include "baseutils.h"
#include <QDateTime>
#include <QFileInfo>
#include <QPixmap>

namespace utils {

namespace image {

const int THUMBNAIL_MAX_SIZE = 291 * 2;
const int THUMBNAIL_NORMAL_SIZE = 128 * 2;

enum ThumbnailType {
    ThumbNormal,
    ThumbLarge,
    ThumbFail
};

const QFileInfoList getImagesAndVideoInfo(const QString &dir, bool recursive = true);
bool imageSupportRead(const QString &path);
QStringList supportedImageFormats();
QPixmap getDamagePixmap(bool bLight = true);
void getAllFileInDir(const QDir &dir, QFileInfoList &result);
void getAllDirInDir(const QDir &dir, QFileInfoList &result);
}  // namespace image

}  // namespace utils

#endif // IMGUTIL_H
