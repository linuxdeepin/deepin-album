// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "imageengineobject.h"
#include "imageengineapi.h"

ImageEngineThreadObject::ImageEngineThreadObject()
{
    setAutoDelete(false); //从根源上禁止auto delete
}

void ImageEngineThreadObject::needStop(void *imageobject)
{
    if (nullptr == imageobject || ifCanStopThread(imageobject)) {
        bneedstop = true;
        bbackstop = true;
    }
}

bool ImageEngineThreadObject::ifCanStopThread(void *imgobject)
{
    Q_UNUSED(imgobject);
    return true;
}

void ImageEngineThreadObject::run()
{
    runDetail(); //原本要run的内容
    emit runFinished(); //告诉m_obj我run完了，这里不再像之前那样直接判断不为nullptr然后解引用m_obj，因为m_obj销毁后不会自动置为nullptr
    this->deleteLater(); //在消息传达后销毁自己
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
        if (thread) {
            thread->needStop(this);
        }
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

    //建立信号槽连接，由于钻石继承问题，ImageEngineImportObject不能继承QObject，所以只能用lambda
    QObject::connect(thread, &ImageEngineThreadObject::runFinished, [this, thread]() {
        this->removeThread(thread);
    });
}

void ImageEngineImportObject::removeThread(ImageEngineThreadObject *thread)
{
    if (m_threads.contains(thread)) {
        m_threads.removeOne(thread);
    }
}

void ImageEngineImportObject::clearAndStopThread()
{
    for (const auto &thread : m_threads) {
        if (thread) {
            thread->needStop(this);
        }
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
    mutex.unlock();
    //建立信号槽连接，由于钻石继承问题，ImageEngineObject不能继承QObject，所以只能用lambda
    QObject::connect(thread, &ImageEngineThreadObject::runFinished, [this, thread]() {
        this->removeThread(thread);
    });
}

void ImageEngineObject::removeThread(ImageEngineThreadObject *thread)
{
    QMutexLocker mutex(&m_mutexthread);
    m_threads.removeOne(thread);
    mutex.unlock();

    //主动切断连接，否则会去执行lambda，然后解引用已经销毁的this指针
    QObject::disconnect(thread, &ImageEngineThreadObject::runFinished, nullptr, nullptr);
}

void ImageEngineObject::clearAndStopThread()
{
    QMutexLocker mutex(&m_mutexthread);
    for (auto thread : m_threads) {
        if (nullptr != thread /*&& ifObjectExist(thread)*/) {
            thread->needStop(this);
            //主动切断连接，否则会去执行lambda，然后解引用已经销毁的this指针
            QObject::disconnect(thread, &ImageEngineThreadObject::runFinished, nullptr, nullptr);
        }
    }
    m_threads.clear();
    m_checkpath.clear();
    m_pathlast.clear();
}
