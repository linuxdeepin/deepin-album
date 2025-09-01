// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fileinotify.h"
#include "application.h"
#include "imageengineapi.h"
#include "signalmanager.h"
#include "utils/unionimage.h"
#include "baseutils.h"
#include "imageutils.h"
#include "signalmanager.h"

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

FileInotify::FileInotify(QObject *parent)
    : QObject(parent)
    , m_Supported(UnionImage_NameSpace::unionImageSupportFormat() + utils::base::m_videoFiletypes) //图片+视频
{
    for (auto &eachData : m_Supported) {
        eachData = eachData.toUpper();
    }

    m_timer = new QTimer();
    connect(m_timer, &QTimer::timeout, this, &FileInotify::onNeedSendPictures);

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString & path) {
        qDebug() << "Directory change detected in:" << path;

        // 检查是否有待创建的目录已经被创建
        checkPendingDirectories(path);

        // 启动定时器处理变化
        if (m_timer) {
            m_timer->start(500);
        }
    });
}

FileInotify::~FileInotify()
{
    clear();
}

void FileInotify::checkNewPath()
{
    // 检查现有监控目录的子目录
    for (const auto &currentDir : m_currentDirs) {
        QDir dir(currentDir);
        QFileInfoList list;
        utils::image::getAllDirInDir(dir, list);

        QStringList dirs;
        std::transform(list.begin(), list.end(), std::back_inserter(dirs), [](const QFileInfo & info) {
            return info.absoluteFilePath();
        });

        if (!dirs.isEmpty()) {
            qDebug() << "Adding new subdirectories to watch:" << dirs;
            m_watcher.addPaths(dirs);
        }
    }

    // 检查待创建目录是否已经存在
    QStringList newlyCreatedDirs;
    for (const QString &pendingDir : m_pendingDirs) {
        QFileInfo pendingInfo(pendingDir);
        if (pendingInfo.exists() && pendingInfo.isDir()) {
            newlyCreatedDirs.append(pendingDir);
            qInfo() << "Pending directory now exists:" << pendingDir;
        }
    }

    // 为新创建的目录设置直接监听
    for (const QString &newDir : newlyCreatedDirs) {
        checkPendingDirectories(QFileInfo(newDir).absolutePath());
    }
}

void FileInotify::addWatcher(const QStringList &paths, const QString &album, int UID)
{
    m_currentAlbum = album;
    m_currentUID = UID;

    // 分类路径：存在的和不存在的
    QStringList existingPaths;
    QStringList nonExistingPaths;

    for (const QString &path : paths) {
        QFileInfo info(path);
        if (info.exists() && info.isDir()) {
            existingPaths.append(path);
        } else {
            nonExistingPaths.append(path);
            qDebug() << "Path does not exist, will try parent monitoring:" << path;
        }
    }

    // 设置当前监控的直接路径
    m_currentDirs = existingPaths;

    // 为存在的路径添加直接监听
    if (!existingPaths.isEmpty()) {
        m_watcher.addPaths(existingPaths);
        qDebug() << "Added direct monitoring for existing paths:" << existingPaths;
    }

    // 为不存在的路径设置父级监听
    for (const QString &path : nonExistingPaths) {
        QFileInfo pathInfo(path);
        QString parentPath = pathInfo.absolutePath();
        QFileInfo parentInfo(parentPath);

        if (parentInfo.exists() && parentInfo.isDir()) {
            addParentWatcher(parentPath, path);
            m_pendingDirs.append(path);
            qDebug() << "Added parent monitoring for non-existing path:" << path << "parent:" << parentPath;
        } else {
            qWarning() << "Cannot monitor path, parent directory does not exist:" << path << "parent:" << parentPath;
        }
    }

    if (m_timer) {
        m_timer->start(1500);
    }
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
    m_Supported.clear();
    m_newFile.clear();
    m_deleteFile.clear();
    m_currentDirs.clear();
    m_pendingDirs.clear();
    m_parentDirs.clear();
    m_parentToChildren.clear();

    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
        delete m_timer;
        m_timer = nullptr;
    }
}

void FileInotify::getAllPicture(bool isFirst)
{
    QFileInfoList list;
    for (int i = 0; i != m_currentDirs.size(); ++i) {
        QDir dir(m_currentDirs[i]);
        if (!dir.exists()) {
            m_currentDirs.removeAt(i);
            --i;
            continue;
        }

        //获取监控目录的所有文件
        utils::image::getAllFileInDir(dir, list);
    }

    if (m_currentDirs.isEmpty()) { //文件夹被删除，清理数据库
        DBManager::instance()->removeCustomAutoImportPath(m_currentUID);
        m_deleteFile = DBManager::instance()->getPathsByAlbum(m_currentUID);
        m_newFile.clear();
        emit pathDestroyed(m_currentUID);
        return;
    }

    //移除不支持的文件
    auto removeIter = std::remove_if(list.begin(), list.end(), [this](const QFileInfo & info) {
        return !m_Supported.contains(info.suffix().toUpper());
    });
    list.erase(removeIter, list.end());

    //提取文件路径
    QStringList filePaths;
    std::transform(list.begin(), list.end(), std::back_inserter(filePaths), [](const QFileInfo & info) {
        return info.isSymLink() ? info.readLink() : info.absoluteFilePath();
    });

    //获取当前已导入的全部文件
    auto allPaths = DBManager::instance()->getPathsByAlbum(m_currentUID);

    //筛选出新增图片文件
    for (auto path : filePaths) {
        if (!allPaths.contains(path)) {
            qDebug() << "New file detected:" << path;
            m_newFile << path;
        }
    }

    //筛选出删除图片文件，初次导入不需要执行
    if (!isFirst) {
        for (auto path : allPaths) {
            if (!filePaths.contains(path)) {
                qDebug() << "File removed:" << path;
                m_deleteFile << path;
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

void FileInotify::onNeedSendPictures()
{
    checkNewPath();
    getAllPicture(false);

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

    if (m_timer) {
        m_timer->stop();
    }
}

void FileInotify::addParentWatcher(const QString &parentPath, const QString &targetChild)
{
    qDebug() << "Adding parent watcher for:" << parentPath << "target child:" << targetChild;

    // 检查是否已经监听了这个父级目录
    if (!m_parentDirs.contains(parentPath)) {
        m_parentDirs.append(parentPath);
        m_watcher.addPath(parentPath);
        qDebug() << "Started monitoring parent directory:" << parentPath;
    }

    // 建立父级目录到子目录的映射
    if (!m_parentToChildren.contains(parentPath)) {
        m_parentToChildren[parentPath] = QStringList();
    }

    if (!m_parentToChildren[parentPath].contains(targetChild)) {
        m_parentToChildren[parentPath].append(targetChild);
        qDebug() << "Added target child mapping:" << parentPath << "->" << targetChild;
    }
}

void FileInotify::checkPendingDirectories(const QString &changedPath)
{
    qDebug() << "Checking pending directories for changed path:" << changedPath;

    // 检查是否有待创建的目录位于变化的路径下
    QStringList foundDirs;
    for (const QString &pendingDir : m_pendingDirs) {
        QFileInfo pendingInfo(pendingDir);

        // 如果待创建目录的父级路径就是变化的路径，并且现在已经存在
        if (pendingInfo.absolutePath() == changedPath && pendingInfo.exists() && pendingInfo.isDir()) {
            foundDirs.append(pendingDir);
            qInfo() << "Found newly created target directory:" << pendingDir;
        }
    }

    // 为新创建的目录添加直接监听
    for (const QString &foundDir : foundDirs) {
        // 从待创建列表中移除
        m_pendingDirs.removeAll(foundDir);

        // 添加到当前监控目录列表
        m_currentDirs.append(foundDir);

        // 添加直接监听
        m_watcher.addPath(foundDir);
        qInfo() << "Added direct monitoring for newly created directory:" << foundDir;

        // 检查是否可以移除父级监听
        QString parentPath = QFileInfo(foundDir).absolutePath();
        if (m_parentToChildren.contains(parentPath)) {
            m_parentToChildren[parentPath].removeAll(foundDir);

            // 如果这个父级目录不再需要监听其他子目录，移除父级监听
            if (m_parentToChildren[parentPath].isEmpty()) {
                m_parentToChildren.remove(parentPath);
                m_parentDirs.removeAll(parentPath);
                m_watcher.removePath(parentPath);
                qDebug() << "Removed parent monitoring for:" << parentPath;
            }
        }
    }
}
