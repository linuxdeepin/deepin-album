// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CLASSIFYUTILS_H
#define CLASSIFYUTILS_H
#include <QString>

class Classifyutils
{
public:
    static Classifyutils *GetInstance();
    QString imageClassify(const QString &path);
    bool isLoaded();
private :
    static Classifyutils *m_pInstance;
    Classifyutils();

    const char* (*imageClassifyFunc)(const char*) = nullptr;
};

#endif // CLASSIFYUTILS_H
