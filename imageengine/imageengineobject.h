#ifndef IMAGEENGINEOBJECT_H
#define IMAGEENGINEOBJECT_H

#include "dbmanager/dbmanager.h"
#include "thumbnail/thumbnaildelegate.h"

#include <QObject>
#include <QPixmap>
#include <QThreadPool>
#include <QRunnable>

class ImageEngineThreadObject;

enum ImageLoadStatu {
    ImageLoadStatu_False,
    ImageLoadStatu_BeLoading,
    ImageLoadStatu_Loaded,
};

struct ImageDataSt {
    QPixmap imgpixmap;
    DBImgInfo dbi;
    ImageLoadStatu loaded;
    ImageEngineThreadObject *thread;
    QString remainDays = "30天";
    ImageDataSt()
    {
        loaded = ImageLoadStatu_False;
        thread = nullptr;
    }
};

//enum DBType {
//    DBNULL,
//    DBAllPaths,
//};

class ImageEngineThreadObject : public QObject
{
    Q_OBJECT
public:
    ImageEngineThreadObject();
    virtual void needStop(void *imageobject);

protected:
    virtual bool ifCanStopThread(void *imgobject);
    bool bneedstop = false;
    bool bbackstop = false;
};

class ImageMountImportPathsObject
{
public:
    ImageMountImportPathsObject();
    virtual ~ImageMountImportPathsObject();
    virtual bool imageMountImported(QStringList &filelist) = 0;
    void addThread(ImageEngineThreadObject *thread);
    void removeThread(ImageEngineThreadObject *thread);
protected:
    void clearAndStopThread();
    QList<ImageEngineThreadObject *> m_threads;
};

class ImageMountGetPathsObject
{
public:
    ImageMountGetPathsObject();
    virtual ~ImageMountGetPathsObject();
    virtual bool imageGeted(QStringList &filelist, QString path) = 0;
    void addThread(ImageEngineThreadObject *thread);
    void removeThread(ImageEngineThreadObject *thread);
protected:
    void clearAndStopThread();
    QList<ImageEngineThreadObject *> m_threads;
};

class ImageEngineImportObject
{
public:
    ImageEngineImportObject();
    virtual ~ImageEngineImportObject();
    virtual bool imageImported(bool success) = 0;
    void addThread(ImageEngineThreadObject *thread);
    void removeThread(ImageEngineThreadObject *thread);
protected:
    void clearAndStopThread();
    QList<ImageEngineThreadObject *> m_threads;
};

class ImageEngineObject
{
public:
    ImageEngineObject();
    virtual ~ImageEngineObject();
    virtual bool imageLoaded(QString filepath) = 0;
    virtual bool imageLocalLoaded(QStringList &filelist) = 0;
    virtual bool imageFromDBLoaded(QStringList &filelist) = 0;
    void addThread(ImageEngineThreadObject *thread);
    void removeThread(ImageEngineThreadObject *thread, bool needmutex = true);
    void addCheckPath(QString path);
    void checkSelf();

    void checkAndReturnPath(QString path);//保证顺序排列
protected:
    void clearAndStopThread();
    QList<ImageEngineThreadObject *> m_threads;
    QStringList m_checkpath;
    QStringList m_pathlast;
    QMutex m_mutexthread;
};

//这是一个用于生成缓存的队列
class ImageCacheSaveObject : public QObject
{
    Q_OBJECT
public:
    ImageCacheSaveObject();

//void setData(QStringList paths);
public:
    bool add(const QStringList pahts);
    bool add(const QString path);
    QString pop();
    bool isEmpty();
private:
    QList<QString> requestQueue;
    QMutex m_queuqMutex;
};


#endif // IMAGEENGINEOBJECT_H
