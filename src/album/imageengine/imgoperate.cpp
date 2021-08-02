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

#include "imgoperate.h"
#include <QDebug>
#include <QDir>
#include <QMutex>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QThread>
#include <QDirIterator>
#include <QIcon>

#define SLEEPTIME 0
const QString CACHE_PATH = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                           + QDir::separator() + "deepin" + QDir::separator() + "deepin-album"/* + QDir::separator()*/;

IMGOperate::IMGOperate(QObject *parent)
{
    Q_UNUSED(parent);
}

IMGOperate::~IMGOperate()
{

}

void IMGOperate::stop()
{
    m_mutex.lock();
    m_needStop = true;
    m_mutex.unlock();
}

void IMGOperate::setNeedSleep()
{
    m_mutex.lock();
    m_mutex.unlock();
}

void IMGOperate::slotDoImgsLoad(QStringList paths)
{
    int i = 0;
    for (QString path : paths) {
        i++;
//        qDebug() << "------" << __FUNCTION__ << "---" << i << "---path = " << path;
        if (m_needStop) {
            break;
        }
        QThread::msleep(SLEEPTIME);
        QString imgPath = CACHE_PATH + path;
        QFileInfo coverInfo(imgPath);
        QPixmap pixmap;
        if (coverInfo.exists()) {
            pixmap = cutPixmap(QPixmap(imgPath));
            emit sigThreadOneImgLoaded(path, pixmap);
        }
    }
}
//将图片裁剪成正方形
QPixmap IMGOperate::cutPixmap(QPixmap pixmap)
{
    if (pixmap.isNull()) {
        return QPixmap();
    }
    int width = pixmap.width();
    int height = pixmap.height();
    if (abs((width - height) * 10 / width) >= 1) {
        QRect rect = pixmap.rect();
        int x = rect.x() + width / 2;
        int y = rect.y() + height / 2;
        if (width > height) {
            x = x - height / 2;
            y = 0;
            pixmap = pixmap.copy(x, y, height, height);
        } else {
            y = y - width / 2;
            x = 0;
            pixmap = pixmap.copy(x, y, width, width);
        }
    }
    return pixmap;
}

