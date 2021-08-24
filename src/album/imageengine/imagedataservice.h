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

#include "playlist_model.h"

class ImageDataService: public QObject
{
    Q_OBJECT
public:
    static ImageDataService *instance(QObject *parent = nullptr);
    explicit ImageDataService(QObject *parent = nullptr);
    ~ImageDataService();

    bool add(const QStringList &paths);
    bool add(const QString &path);
    QString pop();
    bool isEmpty();

    //读取缩略图到缓存map
    bool readThumbnailByPaths(QStringList files);
//    bool readThumbnailByPath(QString file);

    void addImage(const QString &path, const QImage &image);
    QImage getThumnailImageByPath(const QString &path);
    bool imageIsLoaded(const QString &path);

    void addMovieDurationStr(const QString &path, const QString &durationStr);
    QString getMovieDurationStrByPath(const QString &path);
private slots:
signals:
public:
private:
    static ImageDataService *s_ImageDataService;
    QMutex m_queuqMutex;
    QList<QString> m_requestQueue;

    //图片数据锁
    QMutex m_imgDataMutex;
    QMap<QString, QImage> m_AllImageMap;
    QMap<QString, QString> m_movieDurationStrMap;
    QList<QString> m_imageKey;
};


//缩略图读取线程
class readThumbnailThread : public QThread
{
    Q_OBJECT
public:
    readThumbnailThread();
    ~readThumbnailThread() override;
    void readThumbnail(QString m_path);
protected:
    void run() override;
    dmr::PlaylistModel *m_playlistModel = nullptr;
};
#endif // IMAGEDATASERVICE_H
