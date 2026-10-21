// SPDX-FileCopyrightText: 2020-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ocrinterface.h"
#include "ddlog.h"
#include <QDBusMetaType>
#include <QDebug>

OcrInterface::OcrInterface(const QString &serviceName, const QString &ObjectPath,
                           const QDBusConnection &connection, QObject *parent)
    : QDBusAbstractInterface(serviceName, ObjectPath, staticInterfaceName(), connection, parent)
{
    // qCDebug(albumLog) << "OcrInterface::OcrInterface - Function entry";
}

OcrInterface::~OcrInterface()
{
    // qCDebug(albumLog) << "OcrInterface::~OcrInterface - Function entry";
}
