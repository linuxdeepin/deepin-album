// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ocrinterface.h"
#include <QDBusMetaType>
#include <QDebug>

OcrInterface::OcrInterface(const QString &serviceName, const QString &ObjectPath,
                           const QDBusConnection &connection, QObject *parent)
    : QDBusAbstractInterface(serviceName, ObjectPath, staticInterfaceName(), connection, parent)
{
    // qDebug() << "OcrInterface::OcrInterface - Function entry";
}

OcrInterface::~OcrInterface()
{
    // qDebug() << "OcrInterface::~OcrInterface - Function entry";
}
