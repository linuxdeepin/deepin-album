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
//    void     getAllInfos();
    //产生缩略图
    //加载一张缩略图
//    QPixmap     loadOneThumbnail(const QString &imagepath);
    //制作一张缩略图，并通过信号传至主线程
//    void     loadOneImg(QString imagepath);
    void     loadOneImgForce(QString imagepath, bool refresh);//缩略图已存在仍然强制刷新缩略图
    //旋转图片并重新制作缩略图
    void     rotateImageFile(int angel, const QString &path);
    //加载设备中图片列表请求完成
    void sltLoadMountFileList(const QString &path);
    //同步设备卸载
    void sltDeciveUnMount(const QString &path);
signals:
    void sigOneImgReady(QString imagepath, QPixmap pimap);

    void sigAllImgInfosReady(DBImgInfoList);
    void loadOneThumbnailReady(QString imagepath, DBImgInfo data);
    void fileIsNotExist(QString imagepath);
    //加载设备中图片列表请求完成
    void sigMountFileListLoadReady(QString path, QStringList fileList);
public:
    int m_loadBegin = 0;
    int m_loadEnd = 0;
private:
    QStringList m_ImgPaths;
    std::atomic<bool> m_couldRun;
    QVector<QPixmap> m_pixmaps;
    QMap<QString, QStringList>m_PhonePicFileMap; //外部设备及其全部图片路径
};

#endif // DBOPERATE_H
