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
#include "imageenginethread.h"
#include "imageengineapi.h"
#include <dgiovolumemanager.h>
#include <dgiofile.h>
#include <dgiofileinfo.h>
#include <dgiovolume.h>
#include <QImage>
#include <QImageReader>
#include <QDebug>
#include <QDir>
#include <DApplication>
#include <DApplicationHelper>
#include <QStandardPaths>
#include <QDirIterator>
#include <QSvgGenerator>
#include "utils/imageutils.h"
#include "utils/unionimage.h"
#include "dbmanager/dbmanager.h"
#include "application.h"
#include "controller/signalmanager.h"
#include "albumgloabl.h"
#include "imagedataservice.h"
#include "movieservice.h"
#include <QElapsedTimer>
#include <QtConcurrent/QtConcurrent>

DBImgInfo getDBInfo(const QString &srcpath, bool isVideo)
{
    using namespace utils::base;
    using namespace UnionImage_NameSpace;
    QFileInfo srcfi(srcpath);
    DBImgInfo dbi;
    dbi.filePath = srcpath;
    dbi.importTime = QDateTime::currentDateTime();
    if (isVideo) {
        dbi.itemType = ItemTypeVideo;
        //获取视频信息
        MovieInfo movieInfo = MovieService::instance()->getMovieInfo(QUrl::fromLocalFile(srcpath));

        dbi.changeTime = srcfi.lastModified();

        if (movieInfo.creation.isValid()) {
            dbi.time = movieInfo.creation;
        } else if (srcfi.birthTime().isValid()) {
            dbi.time = srcfi.birthTime();
        } else if (srcfi.metadataChangeTime().isValid()) {
            dbi.time = srcfi.metadataChangeTime();
        } else {
            dbi.time = dbi.changeTime;
        }
    } else {
        auto mds = getAllMetaData(srcpath);
        QString value = mds.value("DateTimeOriginal");
        dbi.itemType = ItemTypePic;
        dbi.changeTime = QDateTime::fromString(mds.value("DateTimeDigitized"), "yyyy/MM/dd hh:mm");
        if (!value.isEmpty()) {
            dbi.time = QDateTime::fromString(value, "yyyy/MM/dd hh:mm");
        } else if (srcfi.birthTime().isValid()) {
            dbi.time = srcfi.birthTime();
        } else if (srcfi.metadataChangeTime().isValid()) {
            dbi.time = srcfi.metadataChangeTime();
        } else {
            dbi.time = dbi.changeTime;
        }
    }
    return dbi;
}

ImportImagesThread::ImportImagesThread()
{
    m_paths.clear();
}

ImportImagesThread::~ImportImagesThread()
{
    qDebug() << "ImportImagesThread destoryed";
}

void ImportImagesThread::setData(QList<QUrl> &paths, const QString &albumname, int UID, ImageEngineImportObject *obj, bool bdialogselect, AlbumDBType dbType, bool isFirst)
{
    m_urls = paths;
    m_UID = UID;
    m_obj = obj;
    m_bdialogselect = bdialogselect;
    m_type = DataType_UrlList;
    m_albumname = albumname;
    m_dbType = dbType;
    m_isFirst = isFirst;
}

void ImportImagesThread::setData(QStringList &paths, const QString &albumname, int UID, ImageEngineImportObject *obj, bool bdialogselect, AlbumDBType dbType, bool isFirst)
{
    m_paths = paths;
    m_UID = UID;
    m_obj = obj;
    m_bdialogselect = bdialogselect;
    m_type = DataType_StringList;
    m_albumname = albumname;
    m_dbType = dbType;
    m_isFirst = isFirst;
}

bool ImportImagesThread::ifCanStopThread(void *imgobject)
{
    static_cast<ImageEngineImportObject *>(imgobject)->removeThread(this);
    if (imgobject == m_obj) {
        return true;
    }
    return false;
}

void ImportImagesThread::runDetail()
{
    if (bneedstop) {
        m_obj->imageImported(false);
        m_obj->removeThread(this);
        return;
    }
    QStringList image_list;
    QStringList curAlbumImgPathList;
    if (m_UID >= 0) {
        // 在相册中导入时,
        curAlbumImgPathList = DBManager::instance()->getPathsByAlbum(m_UID);
    } else {
        // 不是在相册中导入时,allpic timeline .etc
        curAlbumImgPathList = DBManager::instance()->getAllPaths();
    }
    //重复导入文件
    QStringList curAlbumImportedPathList;
    // 拖拽导入 url
    if (m_type == DataType_UrlList) {
        for (QUrl url : m_urls) {
            QString temp = url.toLocalFile();
            if (QFileInfo(temp).isSymLink()) {
                temp = QFileInfo(temp).readLink();
            }
            const QString path = temp;
            if (QFileInfo(path).isDir()) {
                auto finfos =  utils::image::getImagesAndVideoInfo(path, true);
                for (auto finfo : finfos) {
                    pathCheck(&image_list, &curAlbumImportedPathList, curAlbumImgPathList, finfo.absoluteFilePath());
                }
            } else if (QFileInfo(path).exists()) { //文件存在
                pathCheck(&image_list, &curAlbumImportedPathList, curAlbumImgPathList, path);
            }
        }
    }
    // 文件管理器选中
    else if (m_type == DataType_StringList) {
        foreach (QString path, m_paths) {
            if (bneedstop) {
                m_obj->imageImported(false);
                m_obj->removeThread(this);
                return;
            }
            if (QFileInfo(path).isSymLink()) {
                path = QFileInfo(path).readLink();
            }
            QFileInfo file(path);
            if (file.isDir()) {
                auto finfos =  utils::image::getImagesAndVideoInfo(path, true);
                for (auto finfo : finfos) {
                    pathCheck(&image_list, &curAlbumImportedPathList, curAlbumImgPathList, finfo.absoluteFilePath());
                }
            } else if (file.exists()) { //文件存在
                pathCheck(&image_list, &curAlbumImportedPathList, curAlbumImgPathList, path);
            }
        }
    }

    if (image_list.isEmpty()) {
        if (curAlbumImportedPathList.size() < 1) {
            // 导入列表为空并且导入相同照片的列表也为空，视为导入失败,直接返回
            // 但是自动导入不执行此项，只是在最终界面显示无照片或视频
            if (m_dbType != AutoImport) {
                emit dApp->signalM->ImportFailed();
            }
            m_obj->imageImported(false);
            m_obj->removeThread(this);
            return;
        } else if (!curAlbumImportedPathList.isEmpty()) {
            // 视为导入的图片全部为重复图片
            // ImportImageLoader() 中，底部状态栏将显示导入状态，之后，核对是否存在重复图片，发送信号准备提示
            emit dApp->signalM->RepeatImportingTheSamePhotos(image_list, curAlbumImportedPathList, m_UID);
            // 导入重复照片提示
//            emit dApp->signalM->sigAddDuplicatePhotos();
            m_obj->imageImported(true);
            m_obj->removeThread(this);
            return;
        }
    } else if (!image_list.isEmpty()) {
        if (m_bdialogselect) {
            QFileInfo firstFileInfo(image_list.first());
            static QString cfgGroupName = QStringLiteral("General"), cfgLastOpenPath = QStringLiteral("LastOpenPath");
            dApp->setter->setValue(cfgGroupName, cfgLastOpenPath, firstFileInfo.path());
        }
        // 判断当前导入路径是否为外接设备
        bool isMountFlag = false;
        auto mounts = utils::base::getMounts_safe();
        for (auto mount : mounts) {
            if (bneedstop || ImageEngineApi::instance()->closeFg()) {
                m_obj->imageImported(false);
                m_obj->removeThread(this);
                return;
            }
            QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
            QString strPath = LocationFile->path();
            if (0 == image_list.first().compare(strPath)) {
                isMountFlag = true;
                break;
            }
        }

        // 当前导入路径
        if (isMountFlag) {
            QString strHomePath = QDir::homePath();
            //获取系统现在的时间
            QString strDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
            QString basePath = QString("%1%2%3").arg(strHomePath, "/Pictures/照片/", strDate);
            QDir dir;
            if (!dir.exists(basePath)) {
                dir.mkpath(basePath);
            }
            QStringList newImagePaths;
            foreach (QString strPath, image_list) {
                if (bneedstop || ImageEngineApi::instance()->closeFg()) {
                    m_obj->imageImported(false);
                    m_obj->removeThread(this);
                    return;
                }
                //取出文件名称
                QStringList pathList = strPath.split("/", QString::SkipEmptyParts);
                QStringList nameList = pathList.last().split(".", QString::SkipEmptyParts);
                QString strNewPath = QString("%1%2%3%4%5%6").arg(basePath, "/", nameList.first(), QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()), ".", nameList.last());

                newImagePaths << strNewPath;
                //判断新路径下是否存在目标文件，若存在，下一次张
                if (dir.exists(strNewPath)) {
                    continue;
                }

                // 外接设备图片拷贝到系统
                if (QFile::copy(strPath, strNewPath)) {

                }
            }

            image_list.clear();
            image_list = newImagePaths;
        }

        DBImgInfoList dbInfos;
        using namespace utils::image;
        int noReadCount = 0; //记录已存在于相册中的数量，若全部存在，则不进行导入操作
        QStringList allPath = DBManager::instance()->getAllPaths();
        for (auto imagePath : image_list) {
            bool bIsVideo = utils::base::isVideo(imagePath);
            if (!bIsVideo && !imageSupportRead(imagePath)) {
                continue;
            }
            QFileInfo srcfi(imagePath);
            if (!srcfi.exists()) {  //当前文件不存在
                noReadCount++;
                continue;
            }
            if (allPath.contains(imagePath)) {
                noReadCount++;
            }
            DBImgInfo info =  getDBInfo(imagePath, bIsVideo);
            dbInfos << info;
            if (m_dbType != AutoImport || m_isFirst) {
                emit dApp->signalM->progressOfWaitDialog(image_list.size(), dbInfos.size());
            }
        }

        std::sort(dbInfos.begin(), dbInfos.end(), [](const DBImgInfo & lhs, const DBImgInfo & rhs) {
            return lhs.changeTime > rhs.changeTime;
        });

        //性能测试是测导入提示框消失到第一屏缩略图加载完毕的时间
        //因此在提示框进行的时候预先加载第一屏的图片
        QFuture<void> firstImportWatcher;
        bool doLoadFirstPage = (DBManager::instance()->getImgsCount() == 0 && m_dbType != AutoImport && !SignalManager::inAutoImport);

        if (doLoadFirstPage) {
            QStringList firstLoadPaths;
            for (int i = 0; i < 70 && i < dbInfos.size(); ++i) {
                firstLoadPaths.push_back(dbInfos[i].filePath);
            }
            firstImportWatcher = utils::base::multiLoadImage(firstLoadPaths);
        }

        if (m_UID >= 0) {
            //导入相册数据库AlbumTable3
            QStringList pathlistImport;
            for (auto &dbInfo : dbInfos) {
                if (!DBManager::instance()->isImgExistInAlbum(m_UID, dbInfo.filePath))
                    pathlistImport << dbInfo.filePath;
            }

            if (!pathlistImport.isEmpty()) {
                DBManager::instance()->insertIntoAlbum(m_UID, pathlistImport, m_dbType);
            }

            //已全部存在，无需导入
            if (noReadCount == image_list.size()) {
                m_obj->imageImported(false);
                m_obj->removeThread(this);
                return;
            }
        }

        //bug112005只有在确认有需要导入的文件时才显示导入进度
        if (m_dbType != AutoImport || m_isFirst) {
            emit dApp->signalM->popupWaitDialog(QObject::tr("Importing..."));
        }

        if (doLoadFirstPage) {
            firstImportWatcher.waitForFinished();
        }

        //导入图片数据库ImageTable3
        DBManager::instance()->insertImgInfos(dbInfos);
        if (m_dbType != AutoImport || m_isFirst) {
            emit dApp->signalM->progressOfWaitDialog(image_list.size(), dbInfos.size());
        }

        if (bneedstop) {
            m_obj->imageImported(false);
            m_obj->removeThread(this);
            return;
        }

        QStringList pathlist;
        for (auto Info : dbInfos) {
            QFileInfo fi(Info.filePath);
            if (!fi.exists())
                continue;
            pathlist << Info.filePath;
        }

        if (m_dbType != AutoImport || m_isFirst) {
            if (image_list.length() == pathlist.length() && !pathlist.isEmpty()) {
                emit dApp->signalM->updateStatusBarImportLabel(pathlist, 1, m_albumname);
                emit dApp->signalM->ImportSuccess(); //导入成功
                m_obj->imageImported(true);
                if (!curAlbumImportedPathList.isEmpty()) {
                    emit dApp->signalM->RepeatImportingTheSamePhotos(image_list, curAlbumImportedPathList, m_UID); //相同图片
                }
            } else {
                if (m_dbType == AutoImport) { //发送导入中断信号
                    emit dApp->signalM->ImportInterrupted(); //导入中断
                    if (!pathlist.isEmpty()) {
                        emit dApp->signalM->ImportSomeFailed(image_list.length(), image_list.length() - pathlist.length()); //部分失败提示
                    }
                } else {
                    //BUG#92844 额外提示未发现照片或文件
                    if (pathlist.isEmpty()) {
                        emit dApp->signalM->ImportDonotFindPicOrVideo();
                    } else {
                        emit dApp->signalM->ImportSomeFailed(image_list.length(), image_list.length() - pathlist.length());
                    }
                    emit dApp->signalM->ImportFailed();
                }
            }
        }

        m_obj->imageImported(false);
    }
    m_obj->removeThread(this);
}

void ImportImagesThread::pathCheck(QStringList *image_list, QStringList *curAlbumImportedPathList, QStringList &curAlbumImgPathList, const QString &path)
{
    if (utils::image::imageSupportRead(path)) {
        if (curAlbumImgPathList.contains(path)) {
            *curAlbumImportedPathList << path;
        } else {
            *image_list << path;
        }
    } else if (utils::base::isVideo(path)) {
        if (MovieService::instance()->getMovieInfo(QUrl::fromLocalFile(path)).valid) {
            if (curAlbumImgPathList.contains(path)) {
                *curAlbumImportedPathList << path;
            } else {
                *image_list << path;
            }
        }
    }
}

ImageRecoveryImagesFromTrashThread::ImageRecoveryImagesFromTrashThread()
{
}

void ImageRecoveryImagesFromTrashThread::setData(QStringList &paths)
{
    m_paths = paths;
}

void ImageRecoveryImagesFromTrashThread::runDetail()
{
    //恢复图片至原来的位置
    auto failedFiles = DBManager::instance()->recoveryImgFromTrash(m_paths);
    emit dApp->signalM->closeWaitDialog();

    //把恢复失败的文件发出去
    if (!failedFiles.isEmpty()) {
        emit dApp->signalM->sigRestoreFailed(failedFiles);
    }
}

ImageMoveImagesToTrashThread::ImageMoveImagesToTrashThread()
{
}

void ImageMoveImagesToTrashThread::setData(const QStringList &paths, bool typetrash)
{
    m_paths = paths;
    btypetrash = typetrash;
}

void ImageMoveImagesToTrashThread::runDetail()
{
    QStringList paths = m_paths;
    if (btypetrash) {
        //从最近删除里面删除图片
        DBManager::instance()->removeTrashImgInfos(paths);
    } else {
        DBImgInfoList infos;
        int pathsCount = paths.size();
        int removedCount = 0;

        QStringList removedPaths;
        emit dApp->signalM->progressOfWaitDialog(paths.size(), 0);

        for (auto path : paths) {
            DBImgInfo info;
            info = DBManager::instance()->getInfoByPath(path);
            info.importTime = QDateTime::currentDateTime();
            //first是UID，secend是album name
            //获取生前所属相册UID，有需要再放开
            //auto allalbumnames = DBManager::instance()->getAllAlbumNames();
            /*for (auto eachname : allalbumnames) {
                if (DBManager::instance()->isImgExistInAlbum(eachname.first, path)) {
                    info.albumUID += (QString::number(eachname.first) + ",");
                }
            }*/
            infos << info;
            removedPaths << path;
            removedCount++;
        }

        DBManager::instance()->insertTrashImgInfos(infos, true);
        DBManager::instance()->removeImgInfos(removedPaths);
        emit dApp->signalM->progressOfWaitDialog(pathsCount, removedCount);
        removedPaths.clear();
        infos.clear();

    }
    emit dApp->signalM->closeWaitDialog();
}

ImageImportFilesFromMountThread::ImageImportFilesFromMountThread()
{
}

ImageImportFilesFromMountThread::~ImageImportFilesFromMountThread()
{
}

void ImageImportFilesFromMountThread::setData(QString &albumname, int UID, QStringList &paths, ImageMountImportPathsObject *imgobject)
{
    m_paths = paths;
    m_imgobject = imgobject;
    m_albumname = albumname;
    m_UID = UID;
}

bool ImageImportFilesFromMountThread::ifCanStopThread(void *imgobject)
{
    static_cast<ImageMountImportPathsObject *>(imgobject)->removeThread(this);
    return (imgobject == m_imgobject);
}

void ImageImportFilesFromMountThread::runDetail()
{
    if (bneedstop) {
        return;
    }
    QStringList newPathList;
    DBImgInfoList dbInfos;
    QString strHomePath = QDir::homePath();
    //获取系统现在的时间
    QString strDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString basePath = QString("%1%2%3").arg(strHomePath, "/Pictures/照片/", strDate); //todo... 中文“照片”需要替换
    QDir dir;
    if (!dir.exists(basePath)) {
        dir.mkpath(basePath);
    }

    foreach (QString strPath, m_paths) {
        if (bneedstop || ImageEngineApi::instance()->closeFg()) {
            return;
        }
        //取出文件名称
        QStringList pathList = strPath.split("/", QString::SkipEmptyParts);
        QStringList nameList = pathList.last().split(".", QString::SkipEmptyParts);
        QString strNewPath = QString("%1%2%3%4%5%6").arg(basePath, "/", nameList.first(),
                                                         QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()), ".", nameList.last());

        //判断新路径下是否存在目标文件，若不存在，继续循环
        if (!dir.exists(strPath)) {
            continue;
        }
        //判断新路径下是否存在目标文件，若存在，先删除掉
        if (dir.exists(strNewPath)) {
            dir.remove(strNewPath);
        }
        newPathList << strNewPath;
        QFileInfo fi(strNewPath);
        using namespace utils::image;
        using namespace utils::base;
        //复制失败的图片不算在成功导入
        if (QFile::copy(strPath, strNewPath)) {
            dbInfos << getDBInfo(strNewPath, utils::base::isVideo(strNewPath));
        } else {
            newPathList.removeOne(strNewPath);
        }
        emit dApp->signalM->progressOfWaitDialog(m_paths.size(), dbInfos.size());
    }
    if (!dbInfos.isEmpty()) {
        QStringList pathslist;
        int idblen = dbInfos.length();
        for (int i = 0; i < idblen; i++) {
            if (bneedstop) {
                return;
            }
            pathslist << dbInfos[i].filePath;
        }

        if (m_albumname.length() > 0) {
            DBManager::instance()->insertIntoAlbum(m_UID, pathslist);
        }
        DBManager::instance()->insertImgInfos(dbInfos);

        if (bneedstop) {
            return;
        }
        if (idblen != m_paths.length()) {
            int successful = dbInfos.length();
            int failed = m_paths.length() - idblen;
            emit dApp->signalM->ImportSomeFailed(successful, failed);
        } else {
            emit dApp->signalM->ImportSuccess();
        }
    } else {
        emit dApp->signalM->ImportFailed();
    }
    emit sigImageFilesImported(m_imgobject, newPathList);
    m_imgobject->removeThread(this);
}

ImageLoadFromDBThread::ImageLoadFromDBThread()
{
}

ImageLoadFromDBThread::~ImageLoadFromDBThread()
{
}

bool ImageLoadFromDBThread::ifCanStopThread(void *imgobject)
{
    static_cast<ImageEngineObject *>(imgobject)->removeThread(this);
    if (imgobject == m_imgobject) {
        return true;
    }
    return false;
}

void ImageLoadFromDBThread::runDetail()
{
    if (bneedstop) {
        return;
    }
    QStringList image_list;
    QStringList fail_image_list;
    DBImgInfoList infos = DBManager::instance()->getAllInfos(0);

    QVector<DBImgInfo> allImageDataVector;

    for (int i = 0; i < infos.size(); i++) {
        DBImgInfo info = infos.at(i);
        //记录源文件不存在的数据
        if (!QFileInfo(info.filePath).exists()) {
            fail_image_list << info.filePath;
            continue;
        }

        allImageDataVector.append(info);
    }
    qDebug() << __FUNCTION__ << "---allImageDataVector.size = " << allImageDataVector.size();
    if (bneedstop) {
        return;
    }

    //删除数据库失效的图片
    QtConcurrent::run([fail_image_list]() {
        DBManager::instance()->removeImgInfosNoSignal(fail_image_list);
    });

    emit ImageEngineApi::instance()->sigReloadAfterFilterEnd(allImageDataVector);
}

RefreshTrashThread::RefreshTrashThread()
{
}

RefreshTrashThread::~RefreshTrashThread()
{
}

void RefreshTrashThread::setData(DBImgInfoList filelist)
{
    m_fileinfolist = filelist;
}

void RefreshTrashThread::runDetail()
{
    if (bneedstop) {
        return;
    }
    QStringList image_list;

    if (!m_fileinfolist.isEmpty()) {
        QStringList removepaths;
        for (auto info : m_fileinfolist) {
            image_list << info.filePath;
        }
        if (0 < image_list.length()) {
            DBManager::instance()->removeTrashImgInfosNoSignal(image_list);
        }
    }
}
