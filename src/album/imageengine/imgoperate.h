/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     ZouYa <zouya@uniontech.com>
 *
 * Maintainer: WangYu <wangyu@uniontech.com>
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

#ifndef IMGOPERATE_H
#define IMGOPERATE_H

#include <QObject>
#include <QDateTime>
#include <QMutex>
#include <QDebug>
#include <QSqlDatabase>

#include "dbmanager.h"
class QSqlDatabase;
class MediaMeta;
class MediaLibrary;
class IMGOperate : public QObject
{
    Q_OBJECT
public:
    explicit IMGOperate(QObject *parent = nullptr);
    ~IMGOperate();

    struct PlaylistDataThread {
        QString uuid;
        bool    readonly    = false;
    };

    void stop();
    void setNeedSleep();
public slots:
    //从缓存中加载图片
    void slotDoImgsLoad(QStringList paths);

    //导入图片进相册
private:
    QPixmap             cutPixmap(QPixmap pixmap);
signals:
    void sigThreadOneImgLoaded(QString path, QPixmap icon);
public:
private:
    bool              m_needStop = false;
    QMutex            m_mutex;
};

#endif // IMGOPERATE_H
