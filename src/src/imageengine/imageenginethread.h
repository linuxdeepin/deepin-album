/*
 * Copyright (C) 2022 ~ 2023 Uniontech Software Technology Co., Ltd.
 *
 * Author:     lichunlong <lichunlong@uniontech.com>
 *
 * Maintainer: lichunlong <lichunlong@uniontech.com>
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
#ifndef IMAGEENGINETHREAD_H
#define IMAGEENGINETHREAD_H

#include <QObject>
#include <QPixmap>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>
#include <QUrl>
#include <QWaitCondition>

class ImageEngineThreadObject;

//这里将QRunnable继承转移到这里，方便将run函数的实现也转移过来
class ImageEngineThreadObject : public QObject, public QRunnable
{
    Q_OBJECT
public:
    ImageEngineThreadObject();
    virtual void needStop(void *imageobject);

signals:
    void runFinished();

protected:
    virtual bool ifCanStopThread(void *imgobject);

    //这里需要在run前run后执行一些操作，即需要一个装饰器
    //但C++不支持像Python那样的装饰器操作，就只能先这样搞了
    //由于thread pool固定执行run，所以后续继承的函数把操作全部扔进runDetail，多出来的操作扔进run
    virtual void run() override final;//使用final禁止后续继承修改run函数实现
    virtual void runDetail() = 0;

    bool bneedstop = false;
    bool bbackstop = false;
};

//导入多个文件/文件夹线程
class ImportImagesThread : public ImageEngineThreadObject
{
    Q_OBJECT
public:
    ImportImagesThread();
    ~ImportImagesThread() override;
    void setData(const QStringList &paths, const int UID);
    void setData(const QList<QUrl> &paths, const int UID, const bool checkRepeat);

protected:
    bool ifCanStopThread(void *imgobject) override;
    void runDetail() override;

signals:
    void runFinished();
    //导入完成信号
    void sigImportFinished();
    //导入失败信号
    void sigImportFailed(int error);
    //导入文件重复信号
    void sigRepeatUrls(QStringList urls);
    //导入进度信号
    void sigImportProgress(int value, int max = 100);

private:
    enum DataType {
        DataType_NULL,
        DataType_String,
        DataType_Url
    };

    QStringList m_paths;//所有的本地路径
    QStringList m_filePaths;//所有的本地文件路径
    int m_UID = -1;
    DataType m_type = DataType_NULL;
    bool m_checkRepeat = true;
};

#endif // IMAGEENGINETHREAD_H
