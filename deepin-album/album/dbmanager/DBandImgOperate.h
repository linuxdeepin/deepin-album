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
#ifndef DBandImgOperate_H
#define DBandImgOperate_H

// ImageTable
///////////////////////////////////////////////////////
//FilePath           | FileName | Dir        | Time | ChangeTime | ImportTime //
//TEXT primari key   | TEXT     | TEXT       | TEXT | TEXT       | TEXT       //
///////////////////////////////////////////////////////

// AlbumTable
/////////////////////////////////////////////////////
//AP               | AlbumName         | FilePath  //
//TEXT primari key | TEXT              | TEXT      //
/////////////////////////////////////////////////////

#include <QObject>
#include <QDateTime>
#include <QMutex>
#include <QDebug>
#include <QSqlDatabase>
#include "dbmanager.h"
#include "imageengineobject.h"

class QSqlDatabase;

class DBandImgOperate : public QObject
{
    Q_OBJECT
public:
    explicit DBandImgOperate(QObject *parent = nullptr);
    ~DBandImgOperate();
public slots:
    void     setThreadShouldStop();
    //获取全部相片信息
    void     getAllInfos();
    //产生缩略图
    //加载一张缩略图
    ImageDataSt     loadOneThumbnail(QString imagepath/*, ImageDataSt data*/);
    void     threadSltLoad80Thumbnail(DBImgInfoList infos);
signals:
    void sig80ImgInfosReady(QMap<QString, ImageDataSt> imageDatas);
    void sigAllImgInfosReady(DBImgInfoList);
    void loadOneThumbnailReady(QString imagepath, ImageDataSt data);
    void fileIsNotExist(QString imagepath);
public:
    int m_loadBegin = 0;
    int m_loadEnd = 0;
private:
    QStringList m_ImgPaths;
    std::atomic<bool> m_couldRun;
    QVector<QPixmap> m_pixmaps;
};

#endif // DBOPERATE_H
