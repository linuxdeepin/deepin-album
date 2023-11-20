// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "classifyutils.h"
#include <QLibrary>
#include <QLibraryInfo>

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
    if (!imageClassifyFunc)
        return "";

    return imageClassifyFunc(path.toStdString().c_str());
}

bool Classifyutils::isLoaded()
{
    return imageClassifyFunc;
}

Classifyutils::Classifyutils()
{
    QLibrary library("libimageclassify.so");
    imageClassifyFunc = reinterpret_cast<const char* (*)(const char *)>(library.resolve("getImageClassification"));

    if (!imageClassifyFunc)
        return;
}
