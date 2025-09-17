// SPDX-FileCopyrightText: 2022 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "eventlogutils.h"
#include <QLibrary>
#include <QDir>
#include <QLibraryInfo>
#include <QJsonDocument>
#include <QDebug>

Eventlogutils *Eventlogutils::m_pInstance = nullptr;
Eventlogutils *Eventlogutils::GetInstance()
{
    // qDebug() << "Eventlogutils::GetInstance - Function entry";
    if (m_pInstance == nullptr) {
        // qDebug() << "Eventlogutils::GetInstance - Branch: creating new instance";
        m_pInstance  = new Eventlogutils();
    }
    // qDebug() << "Eventlogutils::GetInstance - Function exit, returning:" << m_pInstance;
    return m_pInstance;
}

void Eventlogutils::writeLogs(QJsonObject &data)
{
    // qDebug() << "Eventlogutils::writeLogs - Function entry, data keys:" << data.keys();
    if (!writeEventLogFunc) {
        // qDebug() << "Eventlogutils::writeLogs - Branch: no write function available, returning";
        return;
    }

    writeEventLogFunc(QJsonDocument(data).toJson(QJsonDocument::Compact).toStdString());
    // qDebug() << "Eventlogutils::writeLogs - Function exit";
}

Eventlogutils::Eventlogutils()
{
    // qDebug() << "Eventlogutils::Eventlogutils - Constructor entry";
    QLibrary library("libdeepin-event-log.so");
    initFunc = reinterpret_cast<bool (*)(const std::string &, bool)>(library.resolve("Initialize"));
    writeEventLogFunc = reinterpret_cast<void (*)(const std::string &)>(library.resolve("WriteEventLog"));

    if (!initFunc) {
        // qDebug() << "Eventlogutils::Eventlogutils - Branch: init function not available, returning";
        return;
    }

    initFunc("deepin-image-viewer", true);
    // qDebug() << "Eventlogutils::Eventlogutils - Constructor exit";
}
