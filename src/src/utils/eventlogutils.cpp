// SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "eventlogutils.h"
#include "ddlog.h"
#include <QLibrary>
#include <QDir>
#include <QLibraryInfo>
#include <QJsonDocument>
#include <QDebug>

Eventlogutils *Eventlogutils::m_pInstance = nullptr;
Eventlogutils *Eventlogutils::GetInstance()
{
    // qCDebug(albumLog) << "Eventlogutils::GetInstance - Function entry";
    if (m_pInstance == nullptr) {
        // qCDebug(albumLog) << "Eventlogutils::GetInstance - Branch: creating new instance";
        m_pInstance  = new Eventlogutils();
    }
    // qCDebug(albumLog) << "Eventlogutils::GetInstance - Function exit, returning:" << m_pInstance;
    return m_pInstance;
}

void Eventlogutils::writeLogs(QJsonObject &data)
{
    // qCDebug(albumLog) << "Eventlogutils::writeLogs - Function entry, data keys:" << data.keys();
    if (!writeEventLogFunc) {
        // qCDebug(albumLog) << "Eventlogutils::writeLogs - Branch: no write function available, returning";
        return;
    }

    writeEventLogFunc(QJsonDocument(data).toJson(QJsonDocument::Compact).toStdString());
    // qCDebug(albumLog) << "Eventlogutils::writeLogs - Function exit";
}

Eventlogutils::Eventlogutils()
{
    // qCDebug(albumLog) << "Eventlogutils::Eventlogutils - Constructor entry";
    QLibrary library("libdeepin-event-log.so");
    initFunc = reinterpret_cast<bool (*)(const std::string &, bool)>(library.resolve("Initialize"));
    writeEventLogFunc = reinterpret_cast<void (*)(const std::string &)>(library.resolve("WriteEventLog"));

    if (!initFunc) {
        // qCDebug(albumLog) << "Eventlogutils::Eventlogutils - Branch: init function not available, returning";
        return;
    }

    initFunc("deepin-image-viewer", true);
    // qCDebug(albumLog) << "Eventlogutils::Eventlogutils - Constructor exit";
}
