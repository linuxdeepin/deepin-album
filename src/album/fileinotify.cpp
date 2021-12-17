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
#include "baseutils.h"

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

FileInotify::FileInotify(QObject *parent): QObject(parent)
{
    m_allPic.clear();

    m_Supported = UnionImage_NameSpace::unionImageSupportFormat(); //图片
    m_Supported.append(utils::base::m_videoFiletypes); //视频
    for (auto &eachSupported : m_Supported) { //统一视频和图片的后缀格式
        eachSupported = eachSupported.toUpper();
        if (eachSupported.startsWith("*.")) {
            eachSupported = eachSupported.remove(0, 2);
        }
    }

    m_timer = new QTimer();
    connect(m_timer, &QTimer::timeout, this, &FileInotify::onNeedSendPictures);
    m_timer->start(1500);

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString & path) {
        Q_UNUSED(path)
        getAllPicture(false);
    });
}

FileInotify::~FileInotify()
{
    clear();
}

void FileInotify::addWather(const QString &paths, const QString &album, int UID)
{
    QFileInfo info(paths);
    if (!info.exists() || !info.isDir()) {
        return;
    }
    m_currentDir = paths + "/";
    m_currentAlbum = album;
    m_currentUID = UID;
    m_watcher.addPath(paths);

    pathLoadOnce();
}

/*void FileInotify::removeWatcher(const QString &path)
{ 需要的话，得重写一下
    QMutexLocker loker(&m_mutex);
    if (!isVaild())
        return;
    auto it = watchedDirId.find(path);
    if (it != watchedDirId.end()) {
        inotify_rm_watch(m_handleId, it.value());
        watchedDirId.erase(it);
    }
}*/

void FileInotify::clear()
{
    m_allPic.clear();
    m_Supported.clear();
    m_newFile.clear();
    m_Supported.clear();
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
    if (dir_count == 0) {
        return;
    }

    //当前文件夹下的所有文件
    QFileInfoList list = dir.entryInfoList();

    //移除不支持的文件
    auto removeIter = std::remove_if(list.begin(), list.end(), [this](const QFileInfo & info) {
        return !m_Supported.contains(info.suffix().toUpper());
    });
    list.erase(removeIter, list.end());

    //提取文件路径
    QStringList filePaths;
    for (auto &fileInfo : list) {
        filePaths.push_back(fileInfo.absoluteFilePath());
    }

    //筛选出新增图片文件
    for (auto path : filePaths) {
        if (!m_allPic.contains(path)) {
            m_allPic << path;
            m_newFile << path;
        }
    }

    //筛选出删除图片文件，初次导入不需要执行
    if (!isFirst) {
        for (int i = 0; i != m_allPic.size(); ++i) {
            if (!filePaths.contains(m_allPic[i])) {
                m_deleteFile << m_allPic[i];
                m_allPic.removeAt(i);
                i--;
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
    if (!m_newFile.isEmpty() || !m_deleteFile.isEmpty()) {
        emit dApp->signalM->sigMonitorChanged(m_newFile, m_deleteFile, m_currentAlbum, m_currentUID);
        if (m_newFile.size() > 100) {
            QStringList().swap(m_newFile); //强制清理内存
        } else {
            m_newFile.clear();
        }

        if (m_deleteFile.size() > 100) {
            QStringList().swap(m_deleteFile); //强制清理内存
        } else {
            m_deleteFile.clear();
        }
    }
}
