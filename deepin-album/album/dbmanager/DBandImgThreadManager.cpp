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
#include "DBandImgThreadManager.h"
#include "application.h"
#include "DBandImgOperate.h"
#include "controller/signalmanager.h"
#include "utils/baseutils.h"
#include <QDebug>
#include <QDir>
#include <QMutex>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

DBandImgThreadManager  *DBandImgThreadManager::m_dbManager = nullptr;

DBandImgThreadManager  *DBandImgThreadManager::instance()
{
    if (!m_dbManager) {
        m_dbManager = new DBandImgThreadManager();
    }

    return m_dbManager;
}

DBandImgThreadManager::DBandImgThreadManager(QObject *parent)
    : QObject(parent)
      //, m_connectionName("default_connection")
{
    allImgInfos.clear();
    qRegisterMetaType<DBImgInfoList>("DBImgInfoList");

    m_worker = new DBandImgOperate(this);
    m_worker->moveToThread(&workerThread);
    connect(&workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    //发送获取全部照片信息信号
    connect(this, &DBandImgThreadManager::sigGetAllInfos, m_worker, &DBandImgOperate::getAllInfos);
    //收到获取全部照片信息成功信号
    connect(m_worker, &DBandImgOperate::sigAllImgInfosReady, this, &DBandImgThreadManager::slotGetAllImgInfos);
    workerThread.start();
//    std::atomic<bool> m_couldRun;
//    flg.store(false);
//    QAtomicInt flg;
}


DBandImgThreadManager::~DBandImgThreadManager()
{
    m_worker->setThreadShouldStop();
    workerThread.quit();
    workerThread.wait();
}

DBImgInfoList DBandImgThreadManager::getAllInfos(bool refresh)
{
    m_time.start();
    if (refresh) {
        emit sigGetAllInfos();
        return  DBImgInfoList();
    }
    if (this->allImgInfos.size() > 0) {
        return  this->allImgInfos;
    } else {
        emit sigGetAllInfos();
    }
    return  DBImgInfoList();
}

void DBandImgThreadManager::slotGetAllImgInfos(DBImgInfoList infolist)
{
    qDebug() << "zy------dbManager::slotGetAllImgInfos.size = " << infolist.size();
    this->allImgInfos = infolist;
    emit sigAllInfosReady(this->allImgInfos);
}

