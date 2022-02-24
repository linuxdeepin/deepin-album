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
#ifndef IMAGEENGINETHREAD_H
#define IMAGEENGINETHREAD_H

#include <QObject>
#include <QMutex>
#include <QUrl>
#include <QWaitCondition>

#include "imageengineobject.h"

DBImgInfo getDBInfo(const QString &srcpath, bool isVideo = false);

class ImportImagesThread : public ImageEngineThreadObject
{
    Q_OBJECT
public:
    ImportImagesThread();
    ~ImportImagesThread() override;
    void setData(QStringList &paths, const QString &albumname, int UID, ImageEngineImportObject *obj, bool bdialogselect, AlbumDBType dbType, bool isFirst);
    void setData(QList<QUrl> &paths, const QString &albumname, int UID, ImageEngineImportObject *obj, bool bdialogselect, AlbumDBType dbType, bool isFirst);

protected:
    bool ifCanStopThread(void *imgobject) override;
    void runDetail() override;
private:
    void pathCheck(QStringList *image_list, QStringList *curAlbumImportedPathList, QStringList &curAlbumImgPathList, const QString &path);
signals:
    void runFinished();

private:
    enum DataType {
        DataType_NULL,
        DataType_StringList,
        DataType_UrlList
    };

    QStringList m_paths;
    QList<QUrl> m_urls;
    int m_UID = -1;
    QString m_albumname;
    AlbumDBType m_dbType;
    ImageEngineImportObject *m_obj = nullptr;
    bool m_bdialogselect = false;
    DataType m_type = DataType_NULL;
    QStringList m_videoSupportType;
    bool m_isFirst = false;
};

class ImageRecoveryImagesFromTrashThread : public ImageEngineThreadObject
{
    Q_OBJECT
public:
    ImageRecoveryImagesFromTrashThread();
    void setData(QStringList &paths);

protected:
    void runDetail() override;

signals:
private:
    QStringList m_paths;
};

class ImageMoveImagesToTrashThread : public ImageEngineThreadObject
{
    Q_OBJECT
public:
    ImageMoveImagesToTrashThread();
    void setData(const QStringList &paths, bool typetrash);

protected:
    void runDetail() override;

signals:
private:
    QStringList m_paths;
    bool btypetrash = false;
};

class ImageImportFilesFromMountThread : public ImageEngineThreadObject
{
    Q_OBJECT
public:
    ImageImportFilesFromMountThread();
    ~ImageImportFilesFromMountThread() override;
    void setData(QString &albumname, int UID, QStringList &paths, ImageMountImportPathsObject *imgobject);

protected:
    bool ifCanStopThread(void *imgobject) override;
    void runDetail() override;

signals:
    void sigImageFilesImported(void *imgobject, QStringList &filelist);
private:
    QStringList m_paths;
    QString m_albumname;
    int m_UID = -1;
    ImageMountImportPathsObject *m_imgobject = nullptr;
};

class ImageLoadFromDBThread : public ImageEngineThreadObject
{
    Q_OBJECT
public:
    explicit ImageLoadFromDBThread();
    ~ImageLoadFromDBThread() override;
protected:
    bool ifCanStopThread(void *imgobject) override;
    void runDetail() override;
private:
    QString m_nametype;
    ThumbnailDelegate::DelegateType m_type;
    ImageEngineObject *m_imgobject = nullptr;
};
//清除过期已删除文件 >30天
class RefreshTrashThread : public ImageEngineThreadObject
{
    Q_OBJECT
public:
    RefreshTrashThread();
    ~RefreshTrashThread() override;
    void setData(DBImgInfoList filelist);

protected:
    void runDetail() override;
private:
    DBImgInfoList m_fileinfolist;
};


#endif // IMAGEENGINETHREAD_H
