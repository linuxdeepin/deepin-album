/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     WangZhengYang <wangzhengyang@uniontech.com>
 *
 * Maintainer: ZhangYong <ZhangYong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "fileinotifygroup.h"
#include "fileinotify.h"

#include <QDir>
#include <QFileSystemWatcher>

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

    //原cpp为signal发送的全局信号,现在改为传递的方法,发送回albumControl
    connect(watcher, &FileInotify::sigMonitorChanged, this, &FileInotifyGroup::sigMonitorChanged);
    connect(watcher, &FileInotify::pathDestroyed, this, &FileInotifyGroup::sigMonitorDestroyed);

    //加进监控list，后面方便销毁
    watchers.push_back(watcher);
}

FileInotifyGroup::~FileInotifyGroup()
{
    for (auto &watcher : watchers) {
        watcher->deleteLater();
    }
}
