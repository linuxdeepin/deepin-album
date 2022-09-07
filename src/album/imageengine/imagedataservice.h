// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IMAGEDATASERVICE_H
#define IMAGEDATASERVICE_H

#include <QObject>
#include <QMap>
#include <QUrl>
#include <QMutex>
#include <QThread>
#include <QQueue>
#include <deque>

class readThumbnailThread;
class ReadThumbnailManager;
class ImageDataService: public QObject
{
    Q_OBJECT
public:
    static ImageDataService *instance(QObject *parent = nullptr);
    explicit ImageDataService(QObject *parent = nullptr);

    void addImage(const QString &path, const QImage &image);
    QImage getThumnailImageByPathRealTime(const QString &path, bool isTrashFile);
    bool imageIsLoaded(const QString &path, bool isTrashFile);

    void addMovieDurationStr(const QString &path, const QString &durationStr);
    QString getMovieDurationStrByPath(const QString &path);

    void stopFlushThumbnail();
    void waitFlushThumbnailFinish();

    bool readerIsRunning();

private slots:
signals:
    void sigeUpdateListview();
    void startImageLoad();
public:
private:
    bool pathInMap(const QString &path);
    void onNeedReflushThumbnail(const QStringList &paths);

    //QImage:图片，bool:是否是从缓存加载
    std::pair<QImage, bool> getImageFromMap(const QString &path);

    static ImageDataService *s_ImageDataService;

    //图片数据锁
    QMutex m_imgDataMutex;
    //QString:原图路径 QImage:缩略图
    std::deque<std::pair<QString, QImage>> m_AllImageMap;
    QMap<QString, QString> m_movieDurationStrMap;
    QQueue<QString> m_imageKeys;
    int m_visualIndex = 0;//用户查找视图中的model index

    ReadThumbnailManager *readThumbnailManager;
    QThread *readThread;
};

//缩略图读取类
class ReadThumbnailManager : public QObject
{
    Q_OBJECT
public:
    explicit ReadThumbnailManager(QObject *parent = nullptr);
    void addLoadPath(const QString &path);

    bool isRunning()
    {
        return runningFlag;
    }

    void stopRead()
    {
        stopFlag = true;
    }

public slots:
    void readThumbnail();

private:
    std::deque<QString> needLoadPath;
    QMutex mutex;
    std::atomic_bool runningFlag;
    std::atomic_bool stopFlag;
};

#endif // IMAGEDATASERVICE_H
