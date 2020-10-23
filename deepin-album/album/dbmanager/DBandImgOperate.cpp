/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
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

ImageDataSt DBandImgOperate::loadOneThumbnail(QString imagepath/*, ImageDataSt data*/)
{
    if (!QFileInfo(imagepath).exists()) {
        emit fileIsNotExist(imagepath);
        return ImageDataSt();
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
    QPixmap pixmap = QPixmap::fromImage(tImg);
    if (0 != pixmap.height() && 0 != pixmap.width() && (pixmap.height() / pixmap.width()) < 10 && (pixmap.width() / pixmap.height()) < 10) {
        bool cache_exist = false;
        if (pixmap.height() != 200 && pixmap.width() != 200) {
            if (pixmap.height() >= pixmap.width()) {
                cache_exist = true;
                pixmap = pixmap.scaledToWidth(200,  Qt::FastTransformation);
            } else if (pixmap.height() <= pixmap.width()) {
                cache_exist = true;
                pixmap = pixmap.scaledToHeight(200,  Qt::FastTransformation);
            }
        }
        if (!cache_exist) {
            if ((static_cast<float>(pixmap.height()) / (static_cast<float>(pixmap.width()))) > 3) {
                pixmap = pixmap.scaledToWidth(200,  Qt::FastTransformation);
            } else {
                pixmap = pixmap.scaledToHeight(200,  Qt::FastTransformation);
            }
        }
    }
    if (pixmap.isNull()) {
        qDebug() << "null pixmap" << tImg;
        pixmap = QPixmap::fromImage(tImg);
    }
    ImageDataSt data;
    data.imgpixmap = pixmap;
    QFileInfo fi(srcPath);
    //此处不需要加载拍摄时间
    //auto mds = getAllMetaData(srcPath);
    data.loaded = ImageLoadStatu_PreLoaded;
    return data;
    //emit sigImageLoaded(imgobject, m_path, m_data);
    //emit loadOneThumbnailReady(imagepath, data);
}

void DBandImgOperate::threadSltLoad80Thumbnail(DBImgInfoList infos)
{
    DBImgInfoList infosLoad;
    for (int i = m_loadBegin; (i < infos.size() && i < m_loadEnd); i++) {
        infosLoad.append(infos.at(i));
    }
    QMap<QString, ImageDataSt> imageDatas;
    imageDatas.clear();
    for (auto info : infosLoad) {
        ImageDataSt data = loadOneThumbnail(info.filePath);
        imageDatas[info.filePath] = data;
    }
    emit sig80ImgInfosReady(imageDatas);
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
    query.prepare("SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime "
                  "FROM ImageTable3");
    if (! query.exec()) {
        qDebug() << "zy------Get data from ImageTable3 failed: " << query.lastError();
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
//            info.changeTime = stringToDateTime(query.value(4).toString());
            info.changeTime = QDateTime::fromString(query.value(4).toString(), DATETIME_FORMAT_DATABASE);
            info.importTime = QDateTime::fromString(query.value(5).toString(), DATETIME_FORMAT_DATABASE);
            infos << info;
        }
    }
    emit sigAllImgInfosReady(infos);
}
