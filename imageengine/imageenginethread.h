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
    bool ifCanStopThread(void *imgobject) override
    {
        ((ImageEngineImportObject *)imgobject)->removeThread(this);
        if (imgobject == m_obj) {
            return true;
        }
        return false;
    }
    virtual void run();

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

class ImageImportFilesFromMountThread : public ImageEngineThreadObject, public QRunnable
{
    Q_OBJECT
public:
    ImageImportFilesFromMountThread();
    void setData(QString albumname, QStringList paths, ImageMountImportPathsObject *imgobject);

protected:
    bool ifCanStopThread(void *imgobject) override
    {
        ((ImageMountImportPathsObject *)imgobject)->removeThread(this);
        if (imgobject == m_imgobject) {
            return true;
        }
        return false;
    }
    virtual void run();

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
    bool ifCanStopThread(void *imgobject) override
    {
        ((ImageMountGetPathsObject *)imgobject)->removeThread(this);
        if (imgobject == m_imgobject) {
            return true;
        }
        return false;
    }
    virtual void run();

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
    bool ifCanStopThread(void *imgobject) override
    {
        ((ImageEngineObject *)imgobject)->removeThread(this, false);
        if (imgobject == m_imgobject) {
            return true;
        }
        return false;
    }
    virtual void run();

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
    bool ifCanStopThread(void *imgobject) override
    {
        ((ImageEngineObject *)imgobject)->removeThread(this, false);
        if (imgobject == m_imgobject) {
            return true;
        }
        return false;
    }
    virtual void run();

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
    bool ifCanStopThread(void *imgobject) override
    {
        ((ImageEngineObject *)imgobject)->removeThread(this, false);
        m_imgobject.removeOne((ImageEngineObject *)imgobject);
        if (m_imgobject.size() < 1) {
            return true;
        }
        return false;
    }
    virtual void run();

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

#endif // IMAGEENGINETHREAD_H
