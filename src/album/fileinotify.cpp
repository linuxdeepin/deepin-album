/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     ZhangYong <zhangyong@uniontech.com>
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
#include "fileinotify.h"
#include "application.h"
#include "imageengineapi.h"
#include "signalmanager.h"
#include "utils/unionimage.h"

#include <sys/inotify.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDir>

enum {MASK = IN_MODIFY | IN_CREATE | IN_DELETE};

FileInotify::FileInotify(QObject *parent): QThread(parent)
{
    m_allPic.clear();
    m_handleId = inotify_init();
    if (m_handleId == -1) {
        qDebug() << "init inotify fialed ";
    }
    m_Supported = UnionImage_NameSpace::unionImageSupportFormat();

    m_timer = new QTimer();
    connect(m_timer, &QTimer::timeout, this, &FileInotify::onNeedSendPictures);
    m_timer->start(1500);
}

FileInotify::~FileInotify()
{
    clear();
}

bool FileInotify::isVaild()
{
    return (m_handleId != -1);
}

void FileInotify::addWather(const QString &paths)
{
    if (m_handleId < 0)
        return;

    QMutexLocker loker(&m_mutex);
    if (!isVaild())
        return;
    QFileInfo info(paths);
    if (!info.exists() || !info.isDir()) {
        return;
    }
    m_currentDir = paths + "/";

    std::string str = m_currentDir.toStdString();
    const char *path = str.data();

    m_wd = inotify_add_watch(m_handleId, path, IN_MODIFY);
    if (m_wd == -1) {
        qDebug() << " inotify_add_watch failed";
        return;
    }
    watchedDirId.insert(paths, m_wd);

    if (!m_running) {
        m_running = true;
        start();
    }
    pathLoadOnce();
}

//void FileInotify::removeWatcher(const QString &path) //暂未使用
//{
//    QMutexLocker loker(&m_mutex);
//    if (!isVaild())
//        return;
//    auto it = watchedDirId.find(path);
//    if (it != watchedDirId.end()) {
//        inotify_rm_watch(m_handleId, it.value());
//        watchedDirId.erase(it);
//    }
//}

void FileInotify::clear()
{
    m_allPic.clear();
    m_Supported.clear();
    m_newFile.clear();
    m_Supported.clear();
    QMutexLocker loker(&m_mutex);
    for (auto it : watchedDirId) {
        inotify_rm_watch(m_handleId, it);
    }
    watchedDirId.clear();
    if (m_timer->isActive()) {
        m_timer->stop();
        delete m_timer;
        m_timer = nullptr;
    }
}

void FileInotify::getAllPicture(bool isFirst)
{
    QDir dir(m_currentDir);
    if (!dir.exists()) {
        return;
    }
    dir.setFilter(QDir::Files); //设置类型过滤器，只为文件格式
    unsigned int dir_count = dir.count();
    if (dir_count <= 0) {
        return;
    }
    QFileInfoList list = dir.entryInfoList();

    //筛选出新增图片文件
    for (auto fileinfo : list) {
        if (m_Supported.contains(fileinfo.suffix().toUpper())) {
            QString filename = fileinfo.fileName();
            filename = m_currentDir + filename;
            if (!m_allPic.contains(filename)) {
                m_allPic << filename;
                m_newFile << filename;
            }
        }
    }
    //初次导入，判重
    if (isFirst) {
        QStringList allpaths = DBManager::instance()->getAllPaths();
        for (int i = 0; i < m_allPic.size(); i++) {
            if (allpaths.contains(m_allPic.at(i))) {
                m_newFile.removeOne(m_allPic.at(i));
            }
        }
    }
}

//void FileInotify::fileNumChange()
//{
//    QDir dir(m_currentDir);
//    if (!dir.exists()) {
//        return;
//    }
//    dir.setFilter(QDir::Files | QDir::NoSymLinks); //设置类型过滤器，只为文件格式
//    dir.setNameFilters(m_Supported);
//    unsigned int dir_count = dir.count();
//    if (dir_count <= 0) {
//        return;
//    }
//    QFileInfoList list = dir.entryInfoList();
//    for (auto fileinfo : list) {
//        if (m_Supported.contains(fileinfo.suffix().toUpper())) {
//            QString filename = fileinfo.fileName();
//            filename += m_currentDir;
//            if (!m_allPic.contains(filename)) {
//                qDebug() << "filename add: " << filename;
//                m_allPic << filename;
//            }
//        }
//    }
//}

void FileInotify::pathLoadOnce()
{
    getAllPicture(true);
}

void FileInotify::onNeedSendPictures()
{
    //发送导入
    if (m_newFile.size() > 0) {
        emit dApp->signalM->sigMonitorChanged(m_newFile);
        m_newFile.clear();
    }
}

void FileInotify::run()
{
    int len, nread;
    char buf[1024];
    inotify_event *event;
    len = static_cast<int>(read(m_handleId, buf, sizeof(buf) - 1));
    while (len > 0) {
        nread = 0;
        while (len > 0) {
            event = (inotify_event *)&buf[nread];
            nread = nread + static_cast<int>(sizeof(inotify_event) + event->len);
            len = len - static_cast<int>(sizeof(inotify_event) + event->len);
            qDebug() << " need to get new file" <<  event->mask;
            getAllPicture(false);
        }
        len = static_cast<int>(read(m_handleId, buf, sizeof(buf) - 1));
    }
}
