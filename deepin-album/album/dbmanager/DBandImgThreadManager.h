/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
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
#ifndef DBandImgThreadManager_H
#define DBandImgThreadManager_H

// ImageTable
///////////////////////////////////////////////////////
//FilePath           | FileName | Dir        | Time | ChangeTime | ImportTime//
//TEXT primari key   | TEXT     | TEXT       | TEXT | TEXT       | TEXT      //
///////////////////////////////////////////////////////

// AlbumTable
/////////////////////////////////////////////////////
//AP               | AlbumName         | FilePath  //
//TEXT primari key | TEXT              | TEXT      //
/////////////////////////////////////////////////////
#include "dbmanager.h"
#include <QObject>
#include <QDateTime>
#include <QMutex>
#include <QDebug>
#include <QThread>
#include <QSqlDatabase>

class QSqlDatabase;
class DBandImgOperate;

class DBandImgThreadManager : public QObject
{
    Q_OBJECT
public:
    static DBandImgThreadManager  *instance();
    explicit DBandImgThreadManager(QObject *parent = nullptr);
    ~DBandImgThreadManager();

    //获取全部相片信息
    DBImgInfoList     getAllInfos(bool refresh = false);
public slots:
    void slotGetAllImgInfos(DBImgInfoList);
signals:
    //发给线程
    void sigGetAllInfos();
    //发给主线程
    void sigAllInfosReady(DBImgInfoList);
private:
    static DBandImgThreadManager *m_dbManager;
    QThread workerThread;
    QTime m_time;

    DBandImgOperate *m_worker = nullptr;
    DBImgInfoList allImgInfos;
};

#endif // DBMANAGER2_H
