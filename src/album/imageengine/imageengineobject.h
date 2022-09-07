// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IMAGEENGINEOBJECT_H
#define IMAGEENGINEOBJECT_H

#include "dbmanager/dbmanager.h"
#include "thumbnail/thumbnaildelegate.h"

#include <QObject>
#include <QPixmap>
#include <QThreadPool>
#include <QRunnable>

class ImageEngineThreadObject;

//这里将QRunnable继承转移到这里，方便将run函数的实现也转移过来
class ImageEngineThreadObject : public QObject, public QRunnable
{
    Q_OBJECT
public:
    ImageEngineThreadObject();
    virtual void needStop(void *imageobject);

signals:
    void runFinished();

protected:
    virtual bool ifCanStopThread(void *imgobject);

    //这里需要在run前run后执行一些操作，即需要一个装饰器
    //但C++不支持像Python那样的装饰器操作，就只能先这样搞了
    //由于thread pool固定执行run，所以后续继承的函数把操作全部扔进runDetail，多出来的操作扔进run
    virtual void run() override final;//使用final禁止后续继承修改run函数实现
    virtual void runDetail() = 0;

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
    void removeThread(ImageEngineThreadObject *thread);
    void addCheckPath(QString &path);
    void checkSelf();

//    void checkAndReturnPath(QString &path);//保证顺序排列
protected:
    void clearAndStopThread();
    QList<ImageEngineThreadObject *> m_threads;
    QStringList m_checkpath;
    QStringList m_pathlast;
    QMutex m_mutexthread;
};

#endif // IMAGEENGINEOBJECT_H
