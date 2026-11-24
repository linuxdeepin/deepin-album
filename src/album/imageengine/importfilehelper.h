// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IMPORTFILEHELPER_H
#define IMPORTFILEHELPER_H

#include <QStringList>

namespace ImportFileHelper {
QString prepareFile(const QString &sourcePath, const QStringList &mountPaths,
                    const QString &destinationDir, const QStringList &importedPaths);
}

#endif // IMPORTFILEHELPER_H
