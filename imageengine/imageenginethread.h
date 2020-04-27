#ifndef IMAGEENGINETHREAD_H
#define IMAGEENGINETHREAD_H

#include <QObject>
#include <QMutex>
#include <QUrl>
#include "imageengineobject.h"


class ImportImagesThread : public ImageEngineThreadObject, public QRunnable
{
    Q_OBJECT
public:
    ImportImagesThread();
    void setData(QStringList paths, QString albumname, ImageEngineImportObject *obj, bool bdialogselect);
    void setData(QList<QUrl> paths, QString albumname, ImageEngineImportObject *obj, bool bdialogselect);

protected:
    bool ifCanStopThread(void *imgobject) override;
    void run() Q_DECL_OVERRIDE;

private:
    //提前生成缩略图，避免导入过程删除源文件，后续显示空白   暂时没用
    void proDucePic(QString path);

signals:

private:
    enum DataType {
        DataType_NULL,
        DataType_StringList,
        DataType_UrlList
    };
    void ImportImageLoader(DBImgInfoList dbInfos/*, QString albumname = nullptr*/);
    QStringList m_paths;
    QList<QUrl> m_urls;
    QString m_albumname;
    ImageEngineImportObject *m_obj = nullptr;
    bool m_bdialogselect = false;
    DataType m_type = DataType_NULL;
};

class ImageRecoveryImagesFromTrashThread : public ImageEngineThreadObject, public QRunnable
{
    Q_OBJECT
public:
    ImageRecoveryImagesFromTrashThread();
    void setData(QStringList paths);

protected:
    void run() override;

signals:
private:
    QStringList m_paths;
};

class ImageMoveImagesToTrashThread : public ImageEngineThreadObject, public QRunnable
{
    Q_OBJECT
public:
    ImageMoveImagesToTrashThread();
    void setData(QStringList paths, bool typetrash);

protected:
    void run() override;

signals:
private:
    QStringList m_paths;
    bool btypetrash = false;
};

class ImageImportFilesFromMountThread : public ImageEngineThreadObject, public QRunnable
{
    Q_OBJECT
public:
    ImageImportFilesFromMountThread();
    void setData(QString albumname, QStringList paths, ImageMountImportPathsObject *imgobject);

protected:
    bool ifCanStopThread(void *imgobject) override;
    void run() Q_DECL_OVERRIDE;

signals:
    void sigImageFilesImported(void *imgobject, QStringList &filelist);
private:
//    bool findPicturePathByPhone(QString &path);
    QStringList m_paths;
    QString m_albumname;
    ImageMountImportPathsObject *m_imgobject = nullptr;
};

class ImageGetFilesFromMountThread : public ImageEngineThreadObject, public QRunnable
{
    Q_OBJECT
public:
    ImageGetFilesFromMountThread();
    void setData(QString mountname, QString path, ImageMountGetPathsObject *imgobject);

protected:
    bool ifCanStopThread(void *imgobject) override;
    void run() override;

signals:
    void sigImageFilesGeted(void *imgobject, QStringList &filelist, QString path);
private:
    bool findPicturePathByPhone(QString &path);
    QString m_path;
    QString m_mountname;
    ImageMountGetPathsObject *m_imgobject = nullptr;
};

class ImageLoadFromDBThread : public ImageEngineThreadObject, public QRunnable
{
    Q_OBJECT
public:
    ImageLoadFromDBThread();
    void setData(ThumbnailDelegate::DelegateType, ImageEngineObject *imgobject, QString nametype = "");

protected:
    bool ifCanStopThread(void *imgobject) override;
    void run() override;

signals:
    void sigImageLoaded(void *imgobject, QStringList &filelist);
    void sigInsert(QString imagepath, QString remainDay = "");
private:
    QString m_nametype;
    ThumbnailDelegate::DelegateType m_type;
    ImageEngineObject *m_imgobject = nullptr;
};

class ImageLoadFromLocalThread : public ImageEngineThreadObject, public QRunnable
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
    void setData(QStringList filelist, ImageEngineObject *imgobject, bool needcheck, DataType type = DataType_NULL);
    void setData(DBImgInfoList filelist, ImageEngineObject *imgobject, bool needcheck, DataType type = DataType_NULL);

protected:
    bool ifCanStopThread(void *imgobject) override;
    void run() override;

signals:
    void sigImageLoaded(void *imgobject, QStringList &filelist);
    void sigInsert(QString imagepath, QString remainDay = "");
private:
    QStringList checkImage(const QString  path);
    QStringList m_filelist;
    DBImgInfoList m_fileinfolist;
    ImageEngineObject *m_imgobject = nullptr;
    DataType m_type = DataType_NULL;
    bool bneedcheck = true;
};

class ImageEngineThread : public ImageEngineThreadObject, public QRunnable
{
    Q_OBJECT
public:
    ImageEngineThread();
    void setData(QString path, ImageEngineObject *imgobject, ImageDataSt &data, bool needcache = true);
    bool addObject(ImageEngineObject *imgobject);

protected:
    bool ifCanStopThread(void *imgobject) override;
    void run() override;

signals:
    void sigImageLoaded(void *imgobject, QString path, ImageDataSt &data);
    void sigAborted(QString path);
private:
    bool getNeedStop();
    QString m_path = "";
//    ImageEngineObject *m_imgobject = nullptr;
    QList<ImageEngineObject *>m_imgobject;
    ImageDataSt m_data;
    QMutex m_mutex;
    bool bwaitstop = false;
    bool bneedcache = true;
    bool baborted = false;
};

//通过参数启动载入图像的线程
class ImageFromNewAppThread : public ImageEngineThreadObject, public QRunnable
{
    Q_OBJECT
public:
    ImageFromNewAppThread();
    //配置参数
    void setDate(QStringList files, ImageEngineImportObject *obj);
protected:
    bool ifCanStopThread(void *imgobject) override;
    void run() override;

private:
    ImageEngineImportObject *m_imgobj = nullptr;
    QStringList paths;

};

/**
 * @brief The ImageSVGConvertThread class  process svg format image thread,it converts svg image
 *  to another svg image by  QSvgGenerator and QPainter
 */
class ImageSVGConvertThread : public QThread
{
    Q_OBJECT
public:
    ImageSVGConvertThread();
    void setData(QStringList paths, int degree);

protected:
    void run() override;
signals:
    void updateImages(QStringList paths);
    void finished();
private:
    QStringList m_paths;
    int m_degree;
};

class ImageCacheQueuePopThread : public QRunnable
{
public:
    ImageCacheQueuePopThread();
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
    ImageCacheSaveObject *m_obj;
    bool needStop = false;
};

#include <QWaitCondition>
class ImageEngineBackThread : public ImageEngineThreadObject, public QRunnable
{
    Q_OBJECT
public:
    ImageEngineBackThread();

    void setData(QStringList pathlist = QStringList(), QString devName = QString());

protected:
    void run() override;

signals:
    void sigImageBackLoaded(QString path, ImageDataSt &data);
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
