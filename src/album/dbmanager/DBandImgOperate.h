// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

    void waitRotateStop();
    void stopRotate();
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
    void     rotateImageFile(int angel, const QStringList &paths);
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
    std::atomic_bool m_couldRun;
    std::atomic_bool m_rotateNeedStop;
    std::atomic_bool m_rotateIsRunning;
    QVector<QPixmap> m_pixmaps;
    QMap<QString, QStringList>m_PhonePicFileMap; //外部设备及其全部图片路径
};

#endif // DBOPERATE_H
