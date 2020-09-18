#ifndef IMAGEENGINETHREAD_H
#define IMAGEENGINETHREAD_H

#include <QObject>
#include <QMutex>
#include <QUrl>
#include "imageengineobject.h"

DBImgInfo getDBInfo(const QString &srcpath);

class ImportImagesThread : public ImageEngineThreadObject, public QRunnable
{
    Q_OBJECT
public:
    ImportImagesThread();
    ~ImportImagesThread() override;
    void setData(QStringList paths, QString albumname, ImageEngineImportObject *obj, bool bdialogselect);
    void setData(QList<QUrl> paths, QString albumname, ImageEngineImportObject *obj, bool bdialogselect);

protected:
    bool ifCanStopThread(void *imgobject) override;
    void run() Q_DECL_OVERRIDE;

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
    ~ImageImportFilesFromMountThread() override;
    void setData(QString albumname, QStringList paths, ImageMountImportPathsObject *imgobject);

protected:
    bool ifCanStopThread(void *imgobject) override;
    void run() Q_DECL_OVERRIDE;

signals:
    void sigImageFilesImported(void *imgobject, QStringList &filelist);
private:
    QStringList m_paths;
    QString m_albumname;
    ImageMountImportPathsObject *m_imgobject = nullptr;
};

class ImageGetFilesFromMountThread : public ImageEngineThreadObject, public QRunnable
{
    Q_OBJECT
public:
    ImageGetFilesFromMountThread();
    ~ImageGetFilesFromMountThread() override;
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
    ImageLoadFromDBThread(int loadCount = 0);
    ~ImageLoadFromDBThread() override;
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
    int m_loadCount = 0;
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
    ~ImageLoadFromLocalThread() override;
    void setData(QStringList filelist, ImageEngineObject *imgobject, bool needcheck, DataType type = DataType_NULL);
    void setData(DBImgInfoList filelist, ImageEngineObject *imgobject, bool needcheck, DataType type = DataType_NULL);

protected:
    bool ifCanStopThread(void *imgobject) override;
    void run() override;

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

class ImageEngineThread : public ImageEngineThreadObject, public QRunnable
{
    Q_OBJECT
public:
    ImageEngineThread();
    ~ImageEngineThread() override;
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
    QList<ImageEngineObject *>m_imgobject;
    ImageDataSt m_data;
    QMutex m_mutex;
    bool bwaitstop = false;
    bool bneedcache = true;
    bool baborted = false;

    bool breloadCache = false;      //重新生成缓存
};

//通过参数启动载入图像的线程
class ImageFromNewAppThread : public ImageEngineThreadObject, public QRunnable
{
    Q_OBJECT
public:
    ImageFromNewAppThread();
    ~ImageFromNewAppThread() override;
    //配置参数
    void setDate(QStringList files, ImageEngineImportObject *obj);
protected:
    bool ifCanStopThread(void *imgobject) override;
    void run() override;

private:
    ImageEngineImportObject *m_imgobj = nullptr;
    QStringList paths;

};

class ImageCacheQueuePopThread : public QRunnable
{
public:
    ImageCacheQueuePopThread();
    ~ImageCacheQueuePopThread() override;
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

struct RotateSaveRequest {
    double angel;
    QString path;
};

class RotateSaveThread : public QRunnable
{
public:
    RotateSaveThread();
    void setDatas(QHash<QString, RotateSaveRequest>   requests_bar);
protected:
    void run();
private:
    QVector<RotateSaveRequest> m_requests;
};

class ImageRotateThreadControler : public QObject
{
    Q_OBJECT
public:
    ImageRotateThreadControler();
    ~ImageRotateThreadControler();
signals:
    void updateRotate(int angel);
public slots:
    /**
     * @brief addRotateAndSave
     * @param request
     * @param time_gap
     * @author DJH
     * 添加旋转请求队列，并设置队列清空等待时间，等待时间结束后，会执行队列所有命令
     */
    void addRotateAndSave(RotateSaveRequest request, int time_gap);
    void startSave();

private:
    QTimer *wait;
    QThreadPool rotateThreadPool;
    //QList<RotateSaveThread *> rotateThreads;
    QHash<QString, RotateSaveRequest> NoRepeatRequest;
};

#endif // IMAGEENGINETHREAD_H
