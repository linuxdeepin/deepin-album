// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FILEINOTIFYGROUP_H
#define FILEINOTIFYGROUP_H

#include <QObject>

class FileInotify;

class FileInotifyGroup : public QObject
{
    Q_OBJECT
public:
    explicit FileInotifyGroup(QObject *parent = nullptr);
    ~FileInotifyGroup() override;

    //添加：path下的改动会实时同步到对应的album中
    void startWatch(const QStringList &paths, const QString &album, int UID);

private:
    QList<FileInotify *> watchers;
};

#endif // FILEINOTIFYGROUP_H
