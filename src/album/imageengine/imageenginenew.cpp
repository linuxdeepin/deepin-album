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
#include "imageenginenew.h"
#include "DBandImgOperate.h"
#include "controller/signalmanager.h"
#include "application.h"
#include "imageengineapi.h"
#include <QMetaType>
#include <QDirIterator>
#include <QStandardPaths>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include "utils/unionimage.h"
#include "utils/baseutils.h"

ImageEngineNew *ImageEngineNew::s_ImageEngine = nullptr;

ImageEngineNew *ImageEngineNew::instance(QObject *parent)
{
    Q_UNUSED(parent);
    if (!s_ImageEngine) {
        s_ImageEngine = new ImageEngineNew();
    }
    return s_ImageEngine;
}

ImageEngineNew::~ImageEngineNew()
{
    qDebug() << "------" << __FUNCTION__ << "";
}

void ImageEngineNew::loadAllImg()
{
    QStringList pathes = DBManager::instance()->getAllPaths();
    qDebug() << "------" << __FUNCTION__ << "---size =" << pathes.size();
    emit sigDoImgsLoad(pathes);
}

ImageEngineNew::ImageEngineNew(QObject *parent)
{
    qDebug() << "------" << __FUNCTION__ << "";
    Q_UNUSED(parent);

    m_workerThread = new QThread(this);
    m_worker = new IMGOperate();
    m_worker->moveToThread(m_workerThread);
    // 发送信号给子线程导入数据
    connect(this, &ImageEngineNew::sigDoImgsLoad, m_worker, &IMGOperate::slotDoImgsLoad, Qt::QueuedConnection);

    // 完成加载
    connect(m_worker, &IMGOperate::sigThreadOneImgLoaded, this, &ImageEngineNew::slotOneImgLoaded, Qt::QueuedConnection);

    m_workerThread->start();
}

void ImageEngineNew::slotOneImgLoaded(QString path, QPixmap icon)
{
    m_AllImage[path] = icon;
//    qDebug() << "------" << __FUNCTION__ << "---size = " << m_AllImage.size();
    emit sigOneImgLoaded(path, icon);
}
