// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "applicationadpator.h"
#include "ddlog.h"

#include <QUrl>
#include <QDebug>

ApplicationAdaptor::ApplicationAdaptor(FileControl *controller)
    : QDBusAbstractAdaptor(controller)
    , fileControl(controller)
{
    qCDebug(albumLog) << "ApplicationAdaptor::ApplicationAdaptor - Function entry";
}

/**
 * @brief 打开传入的图片文件
 * @param fileName 文件路径
 * @return 是否允许打开图片文件
 */
bool ApplicationAdaptor::openImageFile(const QString &fileName)
{
    qCDebug(albumLog) << "ApplicationAdaptor::openImageFile - Function entry, fileName:" << fileName;
    if (fileControl) {
        qCDebug(albumLog) << "ApplicationAdaptor::openImageFile - Branch: fileControl is valid";
        QString urlPath = QUrl::fromLocalFile(fileName).toString();
        if (fileControl->isCanReadable(urlPath)) {
            qCDebug(albumLog) << "ApplicationAdaptor::openImageFile - Branch: file is readable, emitting signal";
            Q_EMIT fileControl->openImageFile(urlPath);
            return true;
        }
    }

    qCDebug(albumLog) << "ApplicationAdaptor::openImageFile - Function exit, returning false";
    return false;
}
