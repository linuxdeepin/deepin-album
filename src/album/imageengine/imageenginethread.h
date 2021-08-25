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
#ifndef IMAGEENGINETHREAD_H
#define IMAGEENGINETHREAD_H

#include <QObject>
#include <QMutex>
#include <QUrl>
#include "imageengineobject.h"
#include "playlist_model.h"

DBImgInfo getDBInfo(const QString &srcpath, bool isVideo = false);

class ImportImagesThread : public ImageEngineThreadObject
{
    Q_OBJECT
public:
    ImportImagesThread();
    ~ImportImagesThread() override;
    void setData(QStringList &paths, QString &albumname, ImageEngineImportObject *obj, bool bdialogselect);
    void setData(QList<QUrl> &paths, QString &albumname, ImageEngineImportObject *obj, bool bdialogselect);

protected:
    bool ifCanStopThread(void *imgobject) override;
    void runDetail() override;
private:
    void pathCheck(QStringList *image_list, QStringList *curAlbumImportedPathList, QStringList &curAlbumImgPathList, const QString &path);
signals:
    void runFinished();

private:
    enum DataType {
        DataType_NULL,
        DataType_StringList,
        DataType_UrlList
    };

    QStringList m_paths;
    QList<QUrl> m_urls;
    QString m_albumname;
    ImageEngineImportObject *m_obj = nullptr;
    bool m_bdialogselect = false;
    DataType m_type = DataType_NULL;
    QStringList m_videoSupportType;
};

class ImageRecoveryImagesFromTrashThread : public ImageEngineThreadObject
{
    Q_OBJECT
public:
    ImageRecoveryImagesFromTrashThread();
    void setData(QStringList &paths);

protected:
    void runDetail() override;

signals:
private:
    QStringList m_paths;
};

class ImageMoveImagesToTrashThread : public ImageEngineThreadObject
{
    Q_OBJECT
public:
    ImageMoveImagesToTrashThread();
    void setData(const QStringList &paths, bool typetrash);

protected:
    void runDetail() override;

signals:
private:
    QStringList m_paths;
    bool btypetrash = false;
};

class ImageImportFilesFromMountThread : public ImageEngineThreadObject
{
    Q_OBJECT
public:
    ImageImportFilesFromMountThread();
    ~ImageImportFilesFromMountThread() override;
    void setData(QString &albumname, QStringList &paths, ImageMountImportPathsObject *imgobject);

protected:
    bool ifCanStopThread(void *imgobject) override;
    void runDetail() Q_DECL_OVERRIDE;

signals:
    void sigImageFilesImported(void *imgobject, QStringList &filelist);
private:
    QStringList m_paths;
    QString m_albumname;
    ImageMountImportPathsObject *m_imgobject = nullptr;
};

class ImageGetFilesFromMountThread : public ImageEngineThreadObject
{
    Q_OBJECT
public:
    ImageGetFilesFromMountThread();
    ~ImageGetFilesFromMountThread() override;
    void setData(QString &mountname, QString &path, ImageMountGetPathsObject *imgobject);

protected:
    bool ifCanStopThread(void *imgobject) override;
    void runDetail() override;

signals:
    void sigImageFilesGeted(void *imgobject, QStringList &filelist, QString path);
private:
    bool findPicturePathByPhone(QString &path);
    QString m_path;
    QString m_mountname;
    ImageMountGetPathsObject *m_imgobject = nullptr;
};

//class ImageLoadFromDBThread : public ImageEngineThreadObject
//{
//    Q_OBJECT
//public:
//    explicit ImageLoadFromDBThread(int loadCount = 0);
//    ~ImageLoadFromDBThread() override;
//    void setData(ThumbnailDelegate::DelegateType, ImageEngineObject *imgobject, const QString &nametype = "");

//protected:
//    bool ifCanStopThread(void *imgobject) override;
//    void runDetail() override;

//signals:
//    void sigImageLoaded(void *imgobject, QStringList &filelist);
//    void sigInsert(QString imagepath, QString remainDay = "");
//private:
//    QString m_nametype;
//    ThumbnailDelegate::DelegateType m_type;
//    ImageEngineObject *m_imgobject = nullptr;
//    int m_loadCount = 0;
//};

class ImageLoadFromLocalThread : public ImageEngineThreadObject
{
    Q_OBJECT
public:
    enum DataType {
        DataType_NULL,
        DataType_StrList,
        DataType_InfoList,
        DataType_TrashList
    };
    ImageLoadFromLocalThread();
    ~ImageLoadFromLocalThread() override;
    void setData(QStringList &filelist, ImageEngineObject *imgobject, bool needcheck, DataType type = DataType_NULL);
    void setData(DBImgInfoList filelist, ImageEngineObject *imgobject, bool needcheck, DataType type = DataType_NULL);

protected:
    bool ifCanStopThread(void *imgobject) override;
    void runDetail() override;

signals:
    void sigImageLoaded(void *imgobject, QStringList &filelist);
    void sigInsert(QString imagepath, QString remainDay = "");
private:
    QStringList m_filelist;
    DBImgInfoList m_fileinfolist;
    ImageEngineObject *m_imgobject = nullptr;
    DataType m_type = DataType_NULL;
    bool bneedcheck = true;
};

class ImageEngineThread : public ImageEngineThreadObject
{
    Q_OBJECT
public:
    ImageEngineThread();
    ~ImageEngineThread() override;
    void setData(QString &path, ImageEngineObject *imgobject, ImageDataSt &data, bool needcache = true);
    bool addObject(ImageEngineObject *imgobject);

protected:
    bool ifCanStopThread(void *imgobject) override;
    void runDetail() override;

signals:
    void sigImageLoaded(void *imgobject, QString path, ImageDataSt &data);
    void sigAborted(const QString &path);
private:
    bool getNeedStop();
    QString m_path = "";
    QList<ImageEngineObject *>m_imgobject;
    ImageDataSt m_data;
    QMutex m_mutex;
    bool bwaitstop = false;
    bool bneedcache = true;
    bool baborted = false;

    bool breloadCache = false;      //重新生成缓存
};

//通过参数启动载入图像的线程
class ImageFromNewAppThread : public ImageEngineThreadObject
{
    Q_OBJECT
public:
    ImageFromNewAppThread();
    ~ImageFromNewAppThread() override;
    //配置参数
    void setDate(QStringList &files, ImageEngineImportObject *obj);
protected:
    bool ifCanStopThread(void *imgobject) override;
    void runDetail() override;

private:
    ImageEngineImportObject *m_imgobj = nullptr;
    QStringList paths;

};
//缩略图制作线程
class makeThumbnailThread : public QThread
{
    Q_OBJECT
public:
    makeThumbnailThread();
    ~makeThumbnailThread() override;
    void setObject(ImageCacheSaveObject *obj)
    {
        m_obj = obj;
    }
    void saveCache(QString m_path);
    void stopThread()
    {
        needStop = true;
    }
protected:
    void run() override;
private:
    ImageCacheSaveObject *m_obj = nullptr;
    bool needStop = false;
    dmr::PlaylistModel *m_playlistModel = nullptr;
};

#include <QWaitCondition>
class ImageEngineBackThread : public ImageEngineThreadObject
{
    Q_OBJECT
public:
    ImageEngineBackThread();

    void setData(const QStringList &pathlist = QStringList(), const QString &devName = QString());

protected:
    void runDetail() override;

signals:
    void sigImageBackLoaded(QString path, const ImageDataSt &data);
private slots:
    void onStartOrPause(bool pause);
private:
    QStringList m_pathlist;
    ImageDataSt m_data;
    QString m_devName;

    QWaitCondition  m_WatiCondition;
    QMutex  m_mutex;
    bool m_bpause;
};


#endif // IMAGEENGINETHREAD_H
