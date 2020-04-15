#include "imageengineobject.h"
#include "imageengineapi.h"

ImageEngineThreadObject::ImageEngineThreadObject()
{

}

void ImageEngineThreadObject::needStop(void *imageobject)
{
    if (nullptr == imageobject || ifCanStopThread(imageobject))
        bneedstop = true;
}

bool ImageEngineThreadObject::ifCanStopThread(void *imgobject)
{
    Q_UNUSED(imgobject);
    return true;
}

ImageMountImportPathsObject::ImageMountImportPathsObject()
{
    ImageEngineApi::instance()->insertObject(this);
}
ImageMountImportPathsObject::~ImageMountImportPathsObject()
{
    ImageEngineApi::instance()->removeObject(this);
    clearAndStopThread();
}


void ImageMountImportPathsObject::addThread(ImageEngineThreadObject *thread)
{
    m_threads.append(thread);
}
void ImageMountImportPathsObject::removeThread(ImageEngineThreadObject *thread)
{
    m_threads.removeOne(thread);
}
void ImageMountImportPathsObject::clearAndStopThread()
{
    for (auto thread : m_threads) {
        thread->needStop(this);
    }
    m_threads.clear();
}

ImageMountGetPathsObject::ImageMountGetPathsObject()
{
    ImageEngineApi::instance()->insertObject(this);
}
ImageMountGetPathsObject::~ImageMountGetPathsObject()
{
    ImageEngineApi::instance()->removeObject(this);
    clearAndStopThread();
}


void ImageMountGetPathsObject::addThread(ImageEngineThreadObject *thread)
{
    m_threads.append(thread);
}
void ImageMountGetPathsObject::removeThread(ImageEngineThreadObject *thread)
{
    m_threads.removeOne(thread);
}
void ImageMountGetPathsObject::clearAndStopThread()
{
    for (auto thread : m_threads) {
        thread->needStop(this);
    }
    m_threads.clear();
}

ImageEngineImportObject::ImageEngineImportObject()
{
    ImageEngineApi::instance()->insertObject(this);

}
ImageEngineImportObject::~ImageEngineImportObject()
{
    ImageEngineApi::instance()->removeObject(this);
    clearAndStopThread();
}


void ImageEngineImportObject::addThread(ImageEngineThreadObject *thread)
{
    m_threads.append(thread);
}
void ImageEngineImportObject::removeThread(ImageEngineThreadObject *thread)
{
    m_threads.removeOne(thread);
}
void ImageEngineImportObject::clearAndStopThread()
{
    for (const auto &thread : m_threads) {
        thread->needStop(this);
    }
    m_threads.clear();
}

ImageEngineObject::ImageEngineObject()
{
    ImageEngineApi::instance()->insertObject(this);
    m_threads.clear();
}

ImageEngineObject::~ImageEngineObject()
{
    ImageEngineApi::instance()->removeObject(this);
    clearAndStopThread();
}

void ImageEngineObject::addThread(ImageEngineThreadObject *thread)
{
    QMutexLocker mutex(&m_mutexthread);
    m_threads.append(thread);
}
void ImageEngineObject::removeThread(ImageEngineThreadObject *thread, bool needmutex)
{
    if (needmutex)
        QMutexLocker mutex(&m_mutexthread);
    m_threads.removeOne(thread);
}
void ImageEngineObject::addCheckPath(QString path)
{
    m_checkpath << path;
}

void ImageEngineObject::checkSelf()
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

void ImageEngineObject::checkAndReturnPath(QString path)//保证顺序排列
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
void ImageEngineObject::clearAndStopThread()
{
    QMutexLocker mutex(&m_mutexthread);
    for (auto thread : m_threads) {
        thread->needStop(this);
    }
    m_threads.clear();
    m_checkpath.clear();
    m_pathlast.clear();
}

ImageCacheSaveObject::ImageCacheSaveObject()
{

}

bool ImageCacheSaveObject::add(const QString path)
{
    QMutexLocker locker(&m_queuqMutex);
    requestQueue.append(path);
    return true;
}

bool ImageCacheSaveObject::add(const QStringList paths)
{
    QMutexLocker locker(&m_queuqMutex);
    requestQueue.append(paths);
    return true;
}

QString ImageCacheSaveObject::pop()
{
    QMutexLocker locker(&m_queuqMutex);
    QString res = requestQueue.first();
    requestQueue.pop_front();
    return res;
}

bool ImageCacheSaveObject::isEmpty()
{
    return requestQueue.isEmpty();
}
