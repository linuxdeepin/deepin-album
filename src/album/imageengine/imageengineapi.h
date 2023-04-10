// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IMAGEENGINEAPI_H
#define IMAGEENGINEAPI_H

#include <QObject>
#include <QMap>
#include <QUrl>
#include "imageenginethread.h"
#include "imageengineobject.h"
#include "thumbnail/thumbnaildelegate.h"

//加载图片的频率
const int Number_Of_Displays_Per_Time = 32;

//#define   NOGLOBAL;     //是否启用全局线程
class DBandImgOperate;

class ImageEngineApi: public QObject
{
    Q_OBJECT
public:
    static ImageEngineApi *instance(QObject *parent = nullptr);
    ~ImageEngineApi();

    bool insertObject(void *obj);
    bool removeObject(void *obj);
    bool ifObjectExist(void *obj);
    bool ImportImagesFromFileList(QStringList files, const QString &albumname, int UID, ImageEngineImportObject *obj, bool bdialogselect = false, AlbumDBType dbType = Custom, bool isFirst = false);
    bool ImportImagesFromUrlList(QList<QUrl> files, const QString &albumname, int UID, ImageEngineImportObject *obj, bool bdialogselect = false, AlbumDBType dbType = Custom, bool isFirst = false);
    //清理删除时间过长图片
    void cleanUpTrash(const DBImgInfoList &list);
    //过滤不存在图片后重新加载
    bool reloadAfterFilterUnExistImage();

    //从外部启动，启用线程加载图片
    //bool loadImagesFromNewAPP(QStringList files, ImageEngineImportObject *obj);
    bool importImageFilesFromMount(QString albumname, int UID, QStringList paths, ImageMountImportPathsObject *obj);
    bool moveImagesToTrash(QStringList files, bool typetrash = false, bool bneedprogress = true);
    bool recoveryImagesFromTrash(QStringList files);

    //从自动导入路径删除图片，和移动到trash不同，此处是直接从数据库删除
    //参数：需要删除的图片路径，相册UID
    bool removeImageFromAutoImport(const QStringList &files);

    void close()
    {
        bcloseFg = true;
    }
    bool closeFg()
    {
        return bcloseFg;
    }
    void loadFirstPageThumbnails(int num);
    void thumbnailLoadThread();

    //设置线程循环跳出
    void     setThreadShouldStop();

    //启动同步回收站与最近删除线程
    void StartSynRecycleBinToTrashThread();

    //停止同步回收站与最近删除线程
    void StopSynRecycleBinToTrashThread();

private slots:
    void sltImageFilesImported(void *imgobject, QStringList &filelist);
signals:
    //发送给主线程
    //发送给缩略图控件
    void sigLoadFirstPageThumbnailsToView();
    //加载一张图片请求完成
    void sigOneImgReady(QString path, QPixmap pixmap);
    //加载设备中图片列表请求完成
    void sigMountFileListLoadReady(QString path, QStringList fileList);
    //过滤不存在图片后重新加载
    void sigReloadAfterFilterEnd(const QVector<DBImgInfo> &);

    //发给子线程
    //先加载指定数量的缩略图
    void sigLoadThumbnailsByNum(QVector<DBImgInfo> allImageDataVector, int num);
    //发给子线程，加载一张缩略图
    void sigLoadThumbnailIMG(QString path);
    //发给子线程，旋转图片
    void sigRotateImageFile(int angel, const QStringList &paths);
    void sigLoadOneThumbnail(QString imagepath, DBImgInfo data);
    void sigLoadOneThumbnailToThumbnailView(QString imagepath, DBImgInfo data);
    //加载设备中图片列表
    void sigLoadMountFileList(QString path);
    //同步设备卸载
    void sigDeciveUnMount(const QString &path);
    //发送更新信号给[最近删除]
    void sigTrashUpdate();
public:
    int m_FirstPageScreen = 0;
    QStringList m_imgLoaded;//已经加载过的图片，防止多次加载
    bool m_firstPageIsLoaded = false;
    bool isRotating();
    void stopRotate();
    void waitRotateStop();
private:
    explicit ImageEngineApi(QObject *parent = nullptr);

    std::vector<void *>m_AllObject;

    static ImageEngineApi *s_ImageEngine;
    std::atomic_bool bcloseFg;
    QThreadPool *m_pool = nullptr;

    DBandImgOperate *m_worker = nullptr;
    QMutex m_dataMutex;

    SynRecycleBinToTrashThread *threadSynRBT = nullptr; //同步线程-同步回收站与最近删除
};

#endif // IMAGEENGINEAPI_H
