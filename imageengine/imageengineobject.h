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
    virtual void needStop(void *imageobject)
    {
        if (nullptr == imageobject || ifCanStopThread(imageobject))
            bneedstop = true;
    }

protected:
    virtual bool ifCanStopThread(void *imgobject)
    {
        return true;
    }
    bool bneedstop = false;
};

class ImageMountGetPathsObject
{
public:
    ImageMountGetPathsObject();
    ~ImageMountGetPathsObject()
    {
        clearAndStopThread();
    }
    virtual bool imageGeted(QStringList &filelist, QString path) = 0;
    void addThread(ImageEngineThreadObject *thread)
    {
        m_threads.append(thread);
    }
    void removeThread(ImageEngineThreadObject *thread)
    {
        m_threads.removeOne(thread);
    }
protected:
    void clearAndStopThread()
    {
        for (auto thread : m_threads) {
            thread->needStop(this);
        }
        m_threads.clear();
    }
    QList<ImageEngineThreadObject *> m_threads;
};

class ImageEngineImportObject
{
public:
    ImageEngineImportObject();
    ~ImageEngineImportObject()
    {
        clearAndStopThread();
    }
    virtual bool imageImported(bool success) = 0;
    void addThread(ImageEngineThreadObject *thread)
    {
        m_threads.append(thread);
    }
    void removeThread(ImageEngineThreadObject *thread)
    {
        m_threads.removeOne(thread);
    }
protected:
    void clearAndStopThread()
    {
        for (auto thread : m_threads) {
            thread->needStop(this);
        }
        m_threads.clear();
    }
    QList<ImageEngineThreadObject *> m_threads;
};

class ImageEngineObject
{
public:
    ImageEngineObject();
    ~ImageEngineObject()
    {
        clearAndStopThread();
    }
    virtual bool imageLoaded(QString filepath) = 0;
    virtual bool imageLocalLoaded(QStringList &filelist) = 0;
    virtual bool imageFromDBLoaded(QStringList &filelist) = 0;
    void addThread(ImageEngineThreadObject *thread)
    {
//        QMutexLocker mutex(&m_mutexthread);
        m_threads.append(thread);
    }
    void removeThread(ImageEngineThreadObject *thread)
    {
//        QMutexLocker mutex(&m_mutexthread);
        m_threads.removeOne(thread);
    }
    void addCheckPath(QString path)
    {
        m_checkpath << path;
    }

    void checkSelf()
    {
        if (m_checkpath.size() < 1) {
            return;
        }
        for (auto file : m_pathlast) {
            if (file == m_checkpath.first()) {
                m_checkpath.removeFirst();
                imageLoaded(file);
                m_pathlast.removeOne(file);
                checkSelf();
                break;
            }
        }
    }

    void checkAndReturnPath(QString path)//保证顺序排列
    {
        if (m_checkpath.size() < 1) {
            return;
        }
        if (path == m_checkpath.first()) {
            m_checkpath.removeFirst();
            imageLoaded(path);
            checkSelf();
        } else {
            m_pathlast << path;
        }
    }
protected:
    void clearAndStopThread()
    {
//        QMutexLocker mutex(&m_mutexthread);
        for (auto thread : m_threads) {
            thread->needStop(this);
        }
        m_threads.clear();
        m_checkpath.clear();
        m_pathlast.clear();
    }
    QList<ImageEngineThreadObject *> m_threads;
    QStringList m_checkpath;
    QStringList m_pathlast;
//    QMutex m_mutexthread;
};

#endif // IMAGEENGINEOBJECT_H
