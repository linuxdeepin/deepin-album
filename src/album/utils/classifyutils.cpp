// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "classifyutils.h"
#include "albumgloabl.h"

#include <QLibrary>
#include <QLibraryInfo>

const QString ImageClassifyDBusServicePath = "/usr/share/dbus-1/system-services/com.deepin.imageclassify.service";
Classifyutils *Classifyutils::m_pInstance = nullptr;
Classifyutils *Classifyutils::GetInstance()
{
    if (m_pInstance == nullptr) {
        m_pInstance  = new Classifyutils();
    }
    return m_pInstance;
}

QString Classifyutils::imageClassify(const QString &path)
{
    if (!isDBusExist())
        return "";

    QString className = m_dbus->imageClassify(path);
    // 一些无效类型名，统一归类为其他
    if (g_classList.indexOf(className) == -1)
        className = "Other";

    return className;
}

bool Classifyutils::isDBusExist()
{
    return m_bDBusExist;
}

Classifyutils::Classifyutils()
{
    m_dbus = new DaemonImageClassifyInterface(this);
    if (!m_dbus->isValid() && !m_dbus->lastError().message().isEmpty()) {
        qCritical() << "dbus com.deepin.logviewer isValid false error:" << m_dbus->lastError() << m_dbus->lastError().message();
    }

    QFileInfo fi(ImageClassifyDBusServicePath);
    m_bDBusExist = fi.exists();

    qDebug() << "dbus com.deepin.logviewer isValid true";
}
