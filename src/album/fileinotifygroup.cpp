// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fileinotifygroup.h"
#include "fileinotify.h"

#include <QDir>
#include <QFileSystemWatcher>
#include "application.h"
#include "signalmanager.h"

FileInotifyGroup::FileInotifyGroup(QObject *parent) : QObject(parent)
{

}

void FileInotifyGroup::startWatch(const QStringList &paths, const QString &album, int UID)
{
    //去除不存在的路径
    QStringList watchPaths;
    for (const auto &path : paths) {
        QFileInfo info(path);
        if (info.exists() && info.isDir()) {
            watchPaths.push_back(path);
        }
    }
    if (watchPaths.isEmpty()) {
        return;
    }

    //启动监控
    auto watcher = new FileInotify;
    watcher->addWather(watchPaths, album, UID);

    //设置销毁链接
    connect(watcher, &FileInotify::pathDestroyed, dApp->signalM, &SignalManager::sigMonitorDestroyed);

    //加进监控list，后面方便销毁
    watchers.push_back(watcher);
}

FileInotifyGroup::~FileInotifyGroup()
{
    for (auto &watcher : watchers) {
        watcher->deleteLater();
    }
}
