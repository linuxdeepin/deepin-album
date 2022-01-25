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
    bool getImageData(QString imagepath, DBImgInfo &data);
    bool ImportImagesFromFileList(QStringList files, const QString &albumname, int UID, ImageEngineImportObject *obj, bool bdialogselect = false, AlbumDBType dbType = Custom, bool isFirst = false);
    bool ImportImagesFromUrlList(QList<QUrl> files, const QString &albumname, int UID, ImageEngineImportObject *obj, bool bdialogselect = false, AlbumDBType dbType = Custom, bool isFirst = false);
    //清理删除时间过长图片
    void cleanUpTrash(const DBImgInfoList &list);
    //过滤不存在图片后重新加载
    bool reloadAfterFilterUnExistImage();
    //全部数据数量
    int  getAllImageDataCount();
    //添加数据
    void addImageData(QString path, DBImgInfo data);
    //清除全部数据
    void clearAllImageData();
    //判断是否已经从数据库加载过，或者是否已经加载到缓存里了
    bool isItemLoadedFromDB(QString path);

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
    void loadFirstPageThumbnails(int num, bool clearCache = true);
    void thumbnailLoadThread(int num);

    //设置线程循环跳出
    void     setThreadShouldStop();

private slots:
    void sltImageFilesImported(void *imgobject, QStringList &filelist);

    void sigImageBackLoaded(const QString &path, const DBImgInfo &data);

    void slt80ImgInfosReady(QVector<DBImgInfo> ImageDatas);
signals:
    //发送给主线程
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
    void sigLoadThumbnailsByNum(QVector<DBImgInfo> allImageDataVector, int num);
    //发给子线程，加载一张缩略图
    void sigLoadThumbnailIMG(QString path);
    //发给子线程，旋转图片
    void sigRotateImageFile(int angel, const QString &path);
    void sigLoadOneThumbnail(QString imagepath, DBImgInfo data);
    void sigLoadOneThumbnailToThumbnailView(QString imagepath, DBImgInfo data);
    //加载设备中图片列表
    void sigLoadMountFileList(QString path);
    //同步设备卸载
    void sigDeciveUnMount(const QString &path);
public:
    QVector<DBImgInfo> m_AllImageDataVector;
    int m_FirstPageScreen = 0;
    QStringList m_imgLoaded;//已经加载过的图片，防止多次加载
    bool m_firstPageIsLoaded = false;
private:
    explicit ImageEngineApi(QObject *parent = nullptr);

    QMap<void *, void *>m_AllObject;
    QMap<QString, DBImgInfo>m_AllImageData;

    static ImageEngineApi *s_ImageEngine;
    std::atomic_bool bcloseFg;
    QThreadPool *m_pool = nullptr;

    DBandImgOperate *m_worker = nullptr;
    QMutex m_dataMutex;
};

#endif // IMAGEENGINEAPI_H
