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
#ifndef FILEINOTIFY_H
#define FILEINOTIFY_H

#include <QThread>
#include <QObject>
#include <QMutex>
#include <QMap>
#include <QTimer>

class FileInotify : public QThread
{
    Q_OBJECT
public:
    explicit FileInotify(QObject *parent = nullptr);
    ~FileInotify() override;

    bool isVaild();
    //添加和删除监控
    void addWather(const QString &path);
//    void removeWatcher(const QString &path);

    void clear();
    //获取监控目录所有照片
    void getAllPicture(bool isFirst);
    //文件数量改变
//    void fileNumChange(); //预留，暂未使用
    //启动时加载一次
    void pathLoadOnce();

public slots:
    //发送插入
    void onNeedSendPictures();

protected:
    void run() override;

private:
    int  m_handleId = -1;
    int m_wd = -1;
    bool m_running = false;
    QMutex m_mutex;
    QMap<QString, int> watchedDirId;
    QStringList m_allPic;       //目前所有照片
    QStringList m_newFile;      //当前新添加的
    QString m_currentDir;       //给定的当前监控路径
    QStringList  m_Supported;   //支持的格式
    QTimer *m_timer;
};

#endif // FILEINOTIFY_H
