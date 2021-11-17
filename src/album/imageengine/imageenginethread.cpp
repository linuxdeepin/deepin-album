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

DBImgInfo getDBInfo(const QString &srcpath, bool isVideo)
{
    using namespace utils::base;
    using namespace UnionImage_NameSpace;
    QFileInfo srcfi(srcpath);
    DBImgInfo dbi;
    dbi.fileName = srcfi.fileName();
    dbi.filePath = srcpath;
    dbi.dirHash = utils::base::hashByString(QString());
    dbi.importTime = QDateTime::currentDateTime();
    if (isVideo) {
        dbi.itemType = ItemTypeVideo;
        //获取视频信息
        MovieInfo movieInfo = MovieService::instance()->getMovieInfo(QUrl::fromLocalFile(srcpath));

        dbi.changeTime = srcfi.lastModified();

        if (!movieInfo.creation.isEmpty()) {
            dbi.time = QDateTime::fromString(movieInfo.creation);
        } else if (!srcfi.birthTime().isValid()) {
            dbi.time = srcfi.birthTime();
        } else if (!srcfi.metadataChangeTime().isValid()) {
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
        } else if (!srcfi.birthTime().isValid()) {
            dbi.time = srcfi.birthTime();
        } else if (!srcfi.metadataChangeTime().isValid()) {
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
    //setAutoDelete(false); 禁用auto delete，防止崩溃
}

ImportImagesThread::~ImportImagesThread()
{
    qDebug() << "ImportImagesThread destoryed";
}

void ImportImagesThread::setData(QList<QUrl> &paths, QString &albumname, ImageEngineImportObject *obj, bool bdialogselect)
{
    m_urls = paths;
    m_albumname = albumname;
    m_obj = obj;
    m_bdialogselect = bdialogselect;
    m_type = DataType_UrlList;
}

void ImportImagesThread::setData(QStringList &paths, QString &albumname, ImageEngineImportObject *obj, bool bdialogselect)
{
    m_paths = paths;
    m_albumname = albumname;
    m_obj = obj;
    m_bdialogselect = bdialogselect;
    m_type = DataType_StringList;
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
    if (m_albumname.length() > 0) {
        // 在相册中导入时,
        curAlbumImgPathList = DBManager::instance()->getPathsByAlbum(m_albumname);
    } else {
        // 不是在相册中导入时,allpic timeline .etc
        curAlbumImgPathList = DBManager::instance()->getAllPaths();
    }
    //重复导入文件
    QStringList curAlbumImportedPathList;
    // 拖拽导入 url
    if (m_type == DataType_UrlList) {
        QStringList urlLocalPathList;
        for (QUrl url : m_urls) {
            const QString path = url.toLocalFile();
            urlLocalPathList << path;
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

    if (image_list.size() < 1) {
        if (curAlbumImportedPathList.size() < 1) {
            // 导入列表为空并且导入相同照片的列表也为空，视为导入失败,直接返回
            emit dApp->signalM->ImportFailed();
            m_obj->imageImported(false);
            m_obj->removeThread(this);
            return;
        } else if (curAlbumImportedPathList.size() > 0) {
            // 视为导入的图片全部为重复图片
            // ImportImageLoader() 中，底部状态栏将显示导入状态，之后，核对是否存在重复图片，发送信号准备提示
            emit dApp->signalM->RepeatImportingTheSamePhotos(image_list, curAlbumImportedPathList, m_albumname);
            // 导入重复照片提示
//            emit dApp->signalM->sigAddDuplicatePhotos();
            m_obj->imageImported(true);
            m_obj->removeThread(this);
            return;
        }
    } else if (image_list.size() > 0) {
        if (m_bdialogselect) {
            QFileInfo firstFileInfo(image_list.first());
            static QString cfgGroupName = QStringLiteral("General"), cfgLastOpenPath = QStringLiteral("LastOpenPath");
            dApp->setter->setValue(cfgGroupName, cfgLastOpenPath, firstFileInfo.path());
        }
        // 判断当前导入路径是否为外接设备
        int isMountFlag = 0;
        DGioVolumeManager *pvfsManager = new DGioVolumeManager;
        QList<QExplicitlySharedDataPointer<DGioMount>> mounts = pvfsManager->getMounts();
        for (auto mount : mounts) {
            if (bneedstop || ImageEngineApi::instance()->closeFg()) {
                m_obj->imageImported(false);
                m_obj->removeThread(this);
                return;
            }
            QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
            QString strPath = LocationFile->path();
            if (0 == image_list.first().compare(strPath)) {
                isMountFlag = 1;
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

        std::sort(image_list.begin(), image_list.end(), [](const QString & lhs, const QString & rhs) {
            QFileInfo l_info(lhs);
            QFileInfo r_info(rhs);
            return l_info.lastModified().toTime_t() > r_info.lastModified().toTime_t();
        });

        DBImgInfoList dbInfos;
        QStringList pathlist;
        using namespace utils::image;
        int noReadCount = 0;
        bool bIsVideo = false;
        int count = 0;
        QStringList pathlistImport;
        DBImgInfoList dbInfosImport;
        for (auto imagePath : image_list) {
            bIsVideo = utils::base::isVideo(imagePath);
            if (!imageSupportRead(imagePath) && !bIsVideo) {
                noReadCount++;
                continue;
            }
            QFileInfo srcfi(imagePath);
            if (!srcfi.exists()) {  //当前文件不存在
                continue;
            }
            DBImgInfo info =  getDBInfo(imagePath, bIsVideo);
            dbInfos << info;
            count++;
            dbInfosImport << info;
            pathlistImport << info.filePath;
            if (count == 200) {
                //导入相册数据库AlbumTable3
                DBManager::instance()->insertIntoAlbumNoSignal(m_albumname, pathlistImport);
                ImageDataService::instance()->readThumbnailByPaths(pathlistImport, true, true);
                pathlistImport.clear();
                //导入图片数据库ImageTable3
                DBManager::instance()->insertImgInfos(dbInfosImport);
                dbInfosImport.clear();
            }
            emit dApp->signalM->progressOfWaitDialog(image_list.size(), dbInfos.size());
        }
        //导入相册数据库AlbumTable3
        DBManager::instance()->insertIntoAlbumNoSignal(m_albumname, pathlistImport);
        ImageDataService::instance()->readThumbnailByPaths(pathlistImport, true, true);
        pathlistImport.clear();
        //导入图片数据库ImageTable3
        DBManager::instance()->insertImgInfos(dbInfosImport);
        dbInfosImport.clear();
        emit dApp->signalM->progressOfWaitDialog(image_list.size(), dbInfos.size());

        if (bneedstop) {
            m_obj->imageImported(false);
            m_obj->removeThread(this);
            return;
        }
        for (auto Info : dbInfos) {
            QFileInfo fi(Info.filePath);
            if (!fi.exists())
                continue;
            pathlist << Info.filePath;
        }
        emit ImageEngineApi::instance()->sigLoadCompleted();
        if (image_list.length() == pathlist.length() && !pathlist.isEmpty()) {
            if (pathlist.size() > 0) {
                emit dApp->signalM->updateStatusBarImportLabel(pathlist, 1, m_albumname);
                emit dApp->signalM->ImportSuccess();
            } else {
                emit dApp->signalM->ImportFailed();
            }
//            dApp->m_imageloader->ImportImageLoader(tempdbInfos, m_albumname);// 导入照片提示在此处理中
            m_obj->imageImported(true);
            // ImportImageLoader() 中，底部状态栏将显示导入状态，之后，核对是否存在重复图片，发送信号准备提示
            if (curAlbumImportedPathList.count() > 0) {
                emit dApp->signalM->RepeatImportingTheSamePhotos(image_list, curAlbumImportedPathList, m_albumname);
            }
        } else {
            //BUG#92844 额外提示未发现照片或文件
            if (pathlist.isEmpty()) {
                emit dApp->signalM->ImportDonotFindPicOrVideo();
            } else {
                emit dApp->signalM->ImportSomeFailed(image_list.length(), image_list.length() - pathlist.length());
            }

            emit dApp->signalM->ImportFailed();

            m_obj->imageImported(false);
        }
        m_obj->removeThread(this);
    }
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
    setAutoDelete(false);
}

void ImageRecoveryImagesFromTrashThread::setData(QStringList &paths)
{
    m_paths = paths;
}

void ImageRecoveryImagesFromTrashThread::runDetail()
{
    DBImgInfoList infos;
    for (auto path : m_paths) {
        DBImgInfo info = DBManager::instance()->getTrashInfoByPath(path);
        QFileInfo fi(info.filePath);
        if (fi.exists()) {
            info.importTime = QDateTime::currentDateTime();
            infos << info;
        }
    }
    DBManager::instance()->insertImgInfos(infos);

    //恢复到相册是无意义的getTrashInfoByPath无法查询到album
//    for (auto path : paths) {
//        DBImgInfo info = DBManager::instance()->getTrashInfoByPath(path);
//        QStringList namelist = info.albumname.split(",");
//        for (auto eachname : namelist) {
//            if (DBManager::instance()->isAlbumExistInDB(eachname)) {
//                DBManager::instance()->insertIntoAlbum(eachname, QStringList(path));
//            }
//        }
//    }

    DBManager::instance()->removeTrashImgInfos(m_paths);
    emit dApp->signalM->closeWaitDialog();
}

ImageMoveImagesToTrashThread::ImageMoveImagesToTrashThread()
{
    setAutoDelete(false);
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
        DBManager::instance()->removeTrashImgInfos(paths);
    } else {
        DBImgInfoList infos;
        int pathsCount = paths.size();
        int remmoveOffset = 30; //每30条上报前端一次
        int removedCount = 0;
        QStringList removedPaths;
        emit dApp->signalM->progressOfWaitDialog(paths.size(), 0);
        for (auto path : paths) {
            DBImgInfo info;
            info = DBManager::instance()->getInfoByPath(path);
            info.importTime = QDateTime::currentDateTime();
            QStringList allalbumnames = DBManager::instance()->getAllAlbumNames();
            for (auto eachname : allalbumnames) {
                if (DBManager::instance()->isImgExistInAlbum(eachname, path)) {
                    info.albumname += (eachname + ",");
                }
            }
            infos << info;
            removedPaths << path;
            removedCount++;
            if (removedCount % remmoveOffset == 0) {
                DBManager::instance()->insertTrashImgInfos(infos);
                DBManager::instance()->removeImgInfos(removedPaths);
                emit dApp->signalM->progressOfWaitDialog(pathsCount, removedCount);
                removedPaths.clear();
                infos.clear();
            }
        }

        if (infos.size() > 0) {
            DBManager::instance()->insertTrashImgInfos(infos);
            DBManager::instance()->removeImgInfos(removedPaths);
            emit dApp->signalM->progressOfWaitDialog(pathsCount, removedCount);
        }
    }
    emit dApp->signalM->closeWaitDialog();
}

ImageImportFilesFromMountThread::ImageImportFilesFromMountThread()
{
    setAutoDelete(false);
}

ImageImportFilesFromMountThread::~ImageImportFilesFromMountThread()
{
}

void ImageImportFilesFromMountThread::setData(QString &albumname, QStringList &paths, ImageMountImportPathsObject *imgobject)
{
    m_paths = paths;
    m_imgobject = imgobject;
    m_albumname = albumname;
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
        if (QFile::copy(strPath, strNewPath)) {
//            qDebug() << "onCopyPhotoFromPhone()";
        }
        dbInfos << getDBInfo(strNewPath, utils::base::isVideo(strNewPath));

        emit dApp->signalM->progressOfWaitDialog(m_paths.size(), dbInfos.size());
    }
    ImageDataService::instance()->readThumbnailByPaths(newPathList, true);
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
            DBManager::instance()->insertIntoAlbumNoSignal(m_albumname, pathslist);
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
    setAutoDelete(false);
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
    ImageEngineApi::instance()->m_AllImageDataVector.clear();
    ImageEngineApi::instance()->clearAllImageData();
    for (int i = 0; i < infos.size(); i++) {
        DBImgInfo info = infos.at(i);
        //记录源文件不存在的数据
        if (!QFileInfo(info.filePath).exists()) {
            fail_image_list << info.filePath;
            continue;
        }

        ImageEngineApi::instance()->m_AllImageDataVector.append(info);
        ImageEngineApi::instance()->addImageData(info.filePath, info);
    }
    qDebug() << __FUNCTION__ << "---m_AllImageDataVector.size = " << ImageEngineApi::instance()->m_AllImageDataVector.size();
    if (bneedstop) {
        return;
    }

    //删除数据库失效的图片
    DBManager::instance()->removeImgInfosNoSignal(fail_image_list);

    emit ImageEngineApi::instance()->sigReloadAfterFilterEnd();
}

RefreshTrashThread::RefreshTrashThread()
{
    setAutoDelete(false);
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

ImageFromNewAppThread::ImageFromNewAppThread()
{
    setAutoDelete(false);
}

ImageFromNewAppThread::~ImageFromNewAppThread()
{
}

void ImageFromNewAppThread::setDate(QStringList &files, ImageEngineImportObject *obj)
{
    paths = files;
    m_imgobj = obj;
}

bool ImageFromNewAppThread::ifCanStopThread(void *imgobject)
{
    static_cast<ImageEngineImportObject *>(imgobject)->removeThread(this);
    if (imgobject == m_imgobj) {
        return true;
    }
    return  false;
}

void ImageFromNewAppThread::runDetail()
{
    if (bneedstop) {
        m_imgobj->imageImported(false);
        m_imgobj->removeThread(this);
        return;
    }
    DBImgInfoList dbInfos;
    using namespace utils::image;
    for (auto path : paths) {
        if (bneedstop) {
            m_imgobj->imageImported(false);
            m_imgobj->removeThread(this);
            return;
        }
        if (!imageSupportRead(path))
            continue;
        dbInfos << getDBInfo(path);
    }
    if (! dbInfos.isEmpty()) {
        if (bneedstop) {
            m_imgobj->imageImported(false);
            m_imgobj->removeThread(this);
            return;
        }
        dApp->m_imageloader->ImportImageLoader(dbInfos);
    }
    m_imgobj->removeThread(this);
}

ImageEngineBackThread::ImageEngineBackThread(): m_bpause(false)
{
    setAutoDelete(false);
    connect(dApp->signalM, &SignalManager::sigDevStop, this, [ = ](QString devName) {
        if (devName == m_devName || devName.isEmpty()) {
            bbackstop = true;
        }
    });

    connect(dApp->signalM, &SignalManager::sigPauseOrStart, this, &ImageEngineBackThread::onStartOrPause);
}

void ImageEngineBackThread::setData(const QStringList &pathlist, const QString &devName)
{
    m_pathlist = pathlist;
    m_devName = devName;
}

void ImageEngineBackThread::runDetail()
{
    using namespace UnionImage_NameSpace;
    for (auto temppath : m_pathlist) {
        QImage tImg;
        QString path = temppath;
        QString errMsg;
        if (!loadStaticImageFromFile(path, tImg, errMsg)) {
            qDebug() << errMsg;
            break;
        }
        if (bbackstop || ImageEngineApi::instance()->closeFg())
            return;

        if (0 != tImg.height() && 0 != tImg.width() && (tImg.height() / tImg.width()) < 10 && (tImg.width() / tImg.height()) < 10) {
            bool cache_exist = false;
            if (tImg.height() != 100 && tImg.width() != 100) {
                if (tImg.height() >= tImg.width()) {
                    cache_exist = true;
                    tImg = tImg.scaledToWidth(100,  Qt::FastTransformation);
                } else if (tImg.height() <= tImg.width()) {
                    cache_exist = true;
                    tImg = tImg.scaledToHeight(100,  Qt::FastTransformation);
                }
            }
            if (!cache_exist) {
                if ((static_cast<float>(tImg.height()) / (static_cast<float>(tImg.width()))) > 3) {
                    tImg = tImg.scaledToWidth(100,  Qt::FastTransformation);
                } else {
                    tImg = tImg.scaledToHeight(100,  Qt::FastTransformation);
                }
            }
        }

        ImageDataService::instance()->addImage(path, tImg);
        if (bbackstop || ImageEngineApi::instance()->closeFg())
            return;
        m_data = getDBInfo(temppath);

        if (bbackstop || ImageEngineApi::instance()->closeFg()) {
            return;
        }

        if (m_bpause) {
            m_WatiCondition.wait(&m_mutex);     //挂起
        }
        emit sigImageBackLoaded(temppath, m_data);
    }
}

void ImageEngineBackThread::onStartOrPause(bool pause)
{
    if (pause)
        m_bpause = true;
    else {
        m_WatiCondition.wakeOne();      //恢复线程
        m_bpause = false;
    }
}
