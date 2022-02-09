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

    bool add(const QStringList &paths, bool reLoadThumbnail = false);
    bool add(const QString &path);
    QString pop();
    bool isRequestQueueEmpty();
    //获取全部图片数量
    int getCount();

    void addImage(const QString &path, const QImage &image);
    QImage getThumnailImageByPath(const QString &path);
    QImage getThumnailImageByPathRealTime(const QString &path, bool isTrashFile);
    bool imageIsLoaded(const QString &path, bool isTrashFile);

    void addMovieDurationStr(const QString &path, const QString &durationStr);
    QString getMovieDurationStrByPath(const QString &path);

    //
    void setVisualIndex(int row);
    int getVisualIndex();
private slots:
signals:
    void sigeUpdateListview();
    void startImageLoad();
public:
private:
    bool pathInMap(const QString &path);

    //QImage:图片，bool:是否是从缓存加载
    std::pair<QImage, bool> getImageFromMap(const QString &path);

    static ImageDataService *s_ImageDataService;
    QMutex m_queuqMutex;
    QList<QString> m_requestQueue;

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

public slots:
    void readThumbnail();

private:
    std::deque<QString> needLoadPath;
    QMutex mutex;
    std::atomic_bool runningFlag;
};

#endif // IMAGEDATASERVICE_H
