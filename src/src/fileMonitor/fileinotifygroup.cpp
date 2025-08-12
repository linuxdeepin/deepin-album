// Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fileinotifygroup.h"
#include "fileinotify.h"

#include <QDir>
#include <QFileSystemWatcher>
#include <QDebug>

FileInotifyGroup::FileInotifyGroup(QObject *parent) : QObject(parent)
{
    qDebug() << "Initializing FileInotifyGroup";
}

void FileInotifyGroup::startWatch(const QStringList &paths, const QString &album, int UID)
{
    qDebug() << "Starting watch for album:" << album << "UID:" << UID << "with paths:" << paths;
    //启动监控
    auto watcher = new FileInotify;
    watcher->addWather(paths, album, UID);

    //原cpp为signal发送的全局信号,现在改为传递的方法,发送回albumControl
    connect(watcher, &FileInotify::sigMonitorChanged, this, &FileInotifyGroup::sigMonitorChanged);
    connect(watcher, &FileInotify::pathDestroyed, this, &FileInotifyGroup::sigMonitorDestroyed);

    //加进监控list，后面方便销毁
    watchers.push_back(watcher);
    qDebug() << "Added new watcher for album:" << album << "UID:" << UID << "Total watchers:" << watchers.size();
}

FileInotifyGroup::~FileInotifyGroup()
{
    qDebug() << "Cleaning up FileInotifyGroup with" << watchers.size() << "watchers";
    for (auto &watcher : watchers) {
        watcher->deleteLater();
    }
}
