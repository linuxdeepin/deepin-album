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

    //reLoadIsvideo为true，重新判断是否是视频
    bool insertImage(const QString &imagepath, const QString &remainDay, bool reLoadIsvideo = false);
    bool removeImage(QString imagepath);
    bool removeImage(QStringList imagepathList);
    bool insertObject(void *obj);
    bool removeObject(void *obj);
    bool ifObjectExist(void *obj);
    bool getImageData(QString imagepath, ImageDataSt &data);
    bool updateImageDataPixmap(QString imagepath, QPixmap &pix);
    bool ImportImagesFromFileList(QStringList files, QString albumname, ImageEngineImportObject *obj, bool bdialogselect = false);
    bool ImportImagesFromUrlList(QList<QUrl> files, QString albumname, ImageEngineImportObject *obj, bool bdialogselect = false);
    void setImgPathAndAlbumNames(const QMultiMap<QString, QString> &imgPahtAlbums);
    const QMultiMap<QString, QString> &getImgPathAndAlbumNames();
    //清理删除时间过长图片
    void cleanUpTrash(const DBImgInfoList &list);
    //过滤不存在图片后重新加载
    bool reloadAfterFilterUnExistImage();
    //判断是否视频
    bool isVideo(QString path);
    //全部数据数量
    int  getAllImageDataCount();
    //添加数据
    void addImageData(QString path, ImageDataSt data);
    //清除全部数据
    void clearAllImageData();
    //判断是否已经从数据库加载过，或者是否已经加载到缓存里了
    bool isItemLoadedFromDB(QString path);

    //从外部启动，启用线程加载图片
    bool loadImagesFromNewAPP(QStringList files, ImageEngineImportObject *obj);
    bool importImageFilesFromMount(QString albumname, QStringList paths, ImageMountImportPathsObject *obj);
    bool moveImagesToTrash(QStringList files, bool typetrash = false, bool bneedprogress = true);
    bool recoveryImagesFromTrash(QStringList files);

    QStringList get_AllImagePath();

    bool loadImageDateToMemory(QStringList pathlist, QString devName);

    void close()
    {
        bcloseFg = true;
    }
    bool closeFg()
    {
        return bcloseFg;
    }
    void loadFirstPageThumbnails(int num);
    void thumbnailLoadThread(int num);

    //设置线程循环跳出
    void     setThreadShouldStop();
    //根据路径制作缩略图，并保存到指定位置
    bool makeThumbnailByPaths(QStringList files);
private slots:
    void sltInsert(const QStringList &imagepath, const QString &remainDay);
    void sltImageLocalLoaded(void *imgobject, QStringList &filelist);
    void sltImageDBLoaded(void *imgobject, QStringList &filelist);
    void sltImageFilesGeted(void *imgobject, QStringList &filelist, QString path);
    void sltImageFilesImported(void *imgobject, QStringList &filelist);
    void sltstopCacheSave();

    void sigImageBackLoaded(QString path, const ImageDataSt &data);

    void slt80ImgInfosReady(QVector<ImageDataSt> ImageDatas);
signals:
    //发送给主线程
    //加载到数据库完成
    void sigLoadCompleted();
    //发送给缩略图控件
    void sigLoadFirstPageThumbnailsToView();
    //加载一张图片请求完成
    void sigOneImgReady(QString path, QPixmap pixmap);
    //加载设备中图片列表请求完成
    void sigMountFileListLoadReady(QString path, QStringList fileList);
    //过滤不存在图片后重新加载
    void sigReloadAfterFilterEnd();

    //发给子线程
    //先加载指定数量的缩略图
    void sigLoadThumbnailsByNum(QVector<ImageDataSt> allImageDataVector, int num);
    //发给子线程，加载一张缩略图
    void sigLoadThumbnailIMG(QString path);
    //发给子线程，旋转图片
    void sigRotateImageFile(int angel, const QString &path);
    void sigLoadOneThumbnail(QString imagepath, ImageDataSt data);
    void sigLoadOneThumbnailToThumbnailView(QString imagepath, ImageDataSt data);
    //加载设备中图片列表
    void sigLoadMountFileList(QString path);
public:
    QVector<ImageDataSt> m_AllImageDataVector;
    int m_FirstPageScreen = 0;
    QStringList m_imgLoaded;//已经加载过的图片，防止多次加载
    QMultiMap<QString, QString> m_allPathAndAlbumNames;
    bool m_firstPageIsLoaded = false;
private:
    explicit ImageEngineApi(QObject *parent = nullptr);

    QMap<void *, void *>m_AllObject;
    QMap<QString, ImageDataSt>m_AllImageData;

    static ImageEngineApi *s_ImageEngine;
    ImageCacheSaveObject *m_imageCacheSaveobj = nullptr;
    bool bcloseFg = false;
    QThreadPool *m_pool = nullptr;
#ifdef NOGLOBAL
    QThreadPool m_qtpool;
    QThreadPool cacheThreadPool;
#endif
    DBandImgOperate *m_worker = nullptr;
    QMutex m_dataMutex;
};

#endif // IMAGEENGINEAPI_H
