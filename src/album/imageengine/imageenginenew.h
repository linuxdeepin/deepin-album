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
#ifndef IMAGEENGINENEW_H
#define IMAGEENGINENEW_H

#include <QObject>
#include <QMap>
#include <QUrl>
#include "imageenginethread.h"
#include "imageengineobject.h"
#include "thumbnail/thumbnaildelegate.h"
#include "imgoperate.h"

//#define   NOGLOBAL;     //是否启用全局线程
class DBandImgOperate;

class ImageEngineNew: public QObject
{
    Q_OBJECT

public:
    static ImageEngineNew *instance(QObject *parent = nullptr);
    ~ImageEngineNew();

    void loadAllImg();
private:
    explicit ImageEngineNew(QObject *parent = nullptr);

public slots:
    void slotOneImgLoaded(QString path, QPixmap icon);
signals:
    //发送给子线程加载图片
    void sigDoImgsLoad(QStringList pathes);

    //子线程加载一张图片完成
    void sigOneImgLoaded(QString path, QPixmap icon);
public:
    QMap<QString, QPixmap> m_AllImage;
private:
    static ImageEngineNew *s_ImageEngine;
    QThread *m_workerThread = nullptr;

    IMGOperate *m_worker;
};

#endif // IMAGEENGINENEW_H
