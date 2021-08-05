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
#include "DBandImgOperate.h"
#include "dbmanager.h"
#include "application.h"
#include "controller/signalmanager.h"
#include "utils/baseutils.h"
#include "utils/unionimage.h"
#include <QDebug>
#include <QDir>
#include <QMutex>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QDirIterator>

const QString CACHE_PATH = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                           + QDir::separator() + "deepin" + QDir::separator() + "deepin-album"/* + QDir::separator()*/;

DBandImgOperate::DBandImgOperate(QObject *parent)
{
    Q_UNUSED(parent);
    m_ImgPaths.clear();
    m_couldRun.store(true);
}

DBandImgOperate::~DBandImgOperate()
{

}

void DBandImgOperate::setThreadShouldStop()
{
    m_couldRun.store(false);
}

QPixmap DBandImgOperate::loadOneThumbnail(const QString &imagepath)
{
    if (!QFileInfo(imagepath).exists()) {
        emit fileIsNotExist(imagepath);
        return QPixmap();
    }
    using namespace UnionImage_NameSpace;
    QImage tImg;
    QString srcPath = imagepath;
    QString thumbnailPath = CACHE_PATH + imagepath;
    QFileInfo file(thumbnailPath);
    QString errMsg;
    QFileInfo srcfi(srcPath);
    if (file.exists()) {
        if (!loadStaticImageFromFile(thumbnailPath, tImg, errMsg, "PNG")) {
            qDebug() << errMsg;
        }
    } else {
        if (!loadStaticImageFromFile(srcPath, tImg, errMsg)) {
            qDebug() << errMsg;
        }
    }
    if (0 != tImg.height() && 0 != tImg.width() && (tImg.height() / tImg.width()) < 10 && (tImg.width() / tImg.height()) < 10) {
        bool cache_exist = false;
        if (tImg.height() != 200 && tImg.width() != 200) {
            if (tImg.height() >= tImg.width()) {
                cache_exist = true;
                tImg = tImg.scaledToWidth(200,  Qt::FastTransformation);
            } else if (tImg.height() <= tImg.width()) {
                cache_exist = true;
                tImg = tImg.scaledToHeight(200,  Qt::FastTransformation);
            }
        }
        if (!cache_exist) {
            if ((static_cast<float>(tImg.height()) / (static_cast<float>(tImg.width()))) > 3) {
                tImg = tImg.scaledToWidth(200,  Qt::FastTransformation);
            } else {
                tImg = tImg.scaledToHeight(200,  Qt::FastTransformation);
            }
        }
    }

    if (!file.exists()) {
        utils::base::mkMutiDir(thumbnailPath.mid(0, thumbnailPath.lastIndexOf('/')));
        tImg.save(thumbnailPath, "PNG");
    }

    QPixmap pixmap = QPixmap::fromImage(tImg);
    return pixmap;
}

void DBandImgOperate::loadOneImg(QString imagepath)
{
//    qDebug() << "------" << __FUNCTION__ << "" << imagepath;
    if (!QFileInfo(imagepath).exists()) {
        emit fileIsNotExist(imagepath);
        return;
    }
    using namespace UnionImage_NameSpace;
    QImage tImg;
    QString srcPath = imagepath;
    QString thumbnailPath = CACHE_PATH + imagepath;
    QFileInfo file(thumbnailPath);
    QString errMsg;
    QFileInfo srcfi(srcPath);
    if (file.exists()) {
        if (!loadStaticImageFromFile(thumbnailPath, tImg, errMsg, "PNG")) {
            qDebug() << errMsg;
        }
    } else {
        if (!loadStaticImageFromFile(srcPath, tImg, errMsg)) {
            qDebug() << errMsg;
        }
    }

    if (0 != tImg.height() && 0 != tImg.width() && (tImg.height() / tImg.width()) < 10 && (tImg.width() / tImg.height()) < 10) {
        bool cache_exist = false;
        if (tImg.height() != 200 && tImg.width() != 200) {
            if (tImg.height() >= tImg.width()) {
                cache_exist = true;
                tImg = tImg.scaledToWidth(200,  Qt::FastTransformation);
            } else if (tImg.height() <= tImg.width()) {
                cache_exist = true;
                tImg = tImg.scaledToHeight(200,  Qt::FastTransformation);
            }
        }
        if (!cache_exist) {
            if ((static_cast<float>(tImg.height()) / (static_cast<float>(tImg.width()))) > 3) {
                tImg = tImg.scaledToWidth(200,  Qt::FastTransformation);
            } else {
                tImg = tImg.scaledToHeight(200,  Qt::FastTransformation);
            }
        }
    }

    if (!file.exists()) {
        utils::base::mkMutiDir(thumbnailPath.mid(0, thumbnailPath.lastIndexOf('/')));
        tImg.save(thumbnailPath, "PNG");
    }

    QPixmap pixmap = QPixmap::fromImage(tImg);
    emit sigOneImgReady(imagepath, pixmap);
}

void DBandImgOperate::rotateImageFIle(int angel, const QString &path)
{
    QString errMsg;
    if (!UnionImage_NameSpace::rotateImageFIle(angel, path, errMsg)) {
        qDebug() << errMsg;
        return;
    }
    loadOneImg(path);
}

void DBandImgOperate::sltLoadThumbnailByNum(QVector<ImageDataSt> infos, int num)
{
    // 从硬盘上加载图片到缓存中
    int size = infos.size();
    for (int i = 0; i < num; i++) {
        if (i < size) {
            infos[i].imgpixmap = loadOneThumbnail(infos[i].dbi.filePath);
        }
    }
    emit sig80ImgInfosReady(infos);
}

void DBandImgOperate::sltLoadMountFileList(const QString &path)
{
    QString strPath = path;
    if (!m_PhonePicFileMap.contains(strPath)) {
        m_couldRun.store(true);
        //获取所选文件类型过滤器
        QStringList filters;
        for (QString i : UnionImage_NameSpace::unionImageSupportFormat()) {
            filters << "*." + i;
        }
        //定义迭代器并设置过滤器，包括子目录：QDirIterator::Subdirectories
        QDirIterator dir_iterator(strPath,
                                  filters,
                                  QDir::Files | QDir::NoSymLinks,
                                  QDirIterator::Subdirectories);
        QStringList allfiles;
        while (dir_iterator.hasNext()) {
            //需要停止则跳出循环
            if (!m_couldRun.load()) {
                break;
            }
            dir_iterator.next();
            QFileInfo fileInfo = dir_iterator.fileInfo();
            allfiles << fileInfo.filePath();
        }
        //重置标志位，可以执行线程
        m_couldRun.store(true);
        m_PhonePicFileMap[strPath] = allfiles;
        emit sigMountFileListLoadReady(strPath, allfiles);
    } else {
        //已加载过的设备，直接发送缓存的路径
        emit sigMountFileListLoadReady(strPath, m_PhonePicFileMap[strPath]);
    }

}

void DBandImgOperate::getAllInfos()
{
    DBImgInfoList infos;
    QSqlDatabase db = DBManager::instance()->getDatabase();
    if (! db.isValid()) {
        emit sigAllImgInfosReady(infos);
        return;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare("SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime "
                           "FROM ImageTable3");
    if (!b || ! query.exec()) {
        emit sigAllImgInfosReady(infos);
        return;
    } else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
            info.changeTime = QDateTime::fromString(query.value(4).toString(), DATETIME_FORMAT_DATABASE);
            info.importTime = QDateTime::fromString(query.value(5).toString(), DATETIME_FORMAT_DATABASE);
            infos << info;
        }
    }
    emit sigAllImgInfosReady(infos);
}
