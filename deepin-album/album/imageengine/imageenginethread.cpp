#include "imageenginethread.h"
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
#include "imageengineapi.h"

namespace {
const QString CACHE_PATH = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QDir::separator() + "deepin" + QDir::separator() + "deepin-album"/* + QDir::separator()*/;
}

ImportImagesThread::ImportImagesThread()
{
    m_paths.clear();
    setAutoDelete(true);
}

void ImportImagesThread::setData(QList<QUrl> paths, QString albumname, ImageEngineImportObject *obj, bool bdialogselect)
{
    m_urls = paths;
    m_albumname = albumname;
    m_obj = obj;
    m_bdialogselect = bdialogselect;
    m_type = DataType_UrlList;
}

void ImportImagesThread::setData(QStringList paths, QString albumname, ImageEngineImportObject *obj, bool bdialogselect)
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

void ImportImagesThread::ImportImageLoader(DBImgInfoList dbInfos/*, QString albumname*/)
{
    DBImgInfoList dbInfoList;
    QStringList pathlist;

    for (auto info : dbInfos) {
        pathlist << info.filePath;
        dbInfoList << info;
    }
    int count = 1;
    if (m_albumname.length() > 0) {
        if (COMMON_STR_RECENT_IMPORTED != m_albumname
                && COMMON_STR_TRASH != m_albumname
                && COMMON_STR_FAVORITES != m_albumname
                && ALBUM_PATHTYPE_BY_PHONE != m_albumname
                && 0 != m_albumname.compare(tr("Gallery"))) {
            DBManager::instance()->insertIntoAlbumNoSignal(m_albumname, pathlist);
        }
    }

    DBManager::instance()->insertImgInfos(dbInfoList);
    if (pathlist.size() > 0) {
        emit dApp->signalM->updateStatusBarImportLabel(pathlist, count);
    } else {
        count = 0;
        emit dApp->signalM->ImportFailed();
    }

}

void ImportImagesThread::run()
{
    if (bneedstop) {
        m_obj->imageImported(false);
        m_obj->removeThread(this);
        return;
    }
    QStringList image_list;
    if (m_type == DataType_UrlList) {
        for (QUrl url : m_urls) {
            const QString path = url.toLocalFile();
            if (QFileInfo(path).isDir()) {
                auto finfos =  utils::image::getImagesInfo(path, true);
                for (auto finfo : finfos) {
                    if (utils::image::imageSupportRead(finfo.absoluteFilePath())) {
                        image_list << finfo.absoluteFilePath();
                    }
                }
            } else if (utils::image::imageSupportRead(path)) {
                image_list << path;
            }
        }
    } else if (m_type == DataType_StringList) {
        foreach (QString path, m_paths) {
            if (bneedstop) {
                m_obj->imageImported(false);
                m_obj->removeThread(this);
                return;
            }
            QFileInfo file(path);
            if (file.isDir()) {
                auto finfos =  utils::image::getImagesInfo(path, true);
                for (auto finfo : finfos) {
                    if (utils::image::imageSupportRead(finfo.absoluteFilePath())) {
                        image_list << finfo.absoluteFilePath();
                    }
                }
            } else if (file.exists()) { //文件存在
                image_list << path;
            }
        }
    }
    if (image_list.size() < 1) {
        emit dApp->signalM->ImportFailed();
        m_obj->imageImported(false);
        m_obj->removeThread(this);
        return;
    }
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
    int importMountFailed = 0;
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
                importMountFailed++;
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
    int noReadCount = 0;
    for (auto imagePath : image_list) {

        if (!imageSupportRead(imagePath)) {
            noReadCount++;
            continue;
        }

        QFileInfo fi(imagePath);
        if (!fi.exists()) {  //当前文件不存在
            continue;
        }
        using namespace utils::image;
        using namespace utils::base;
        auto mds = getAllMetaData(imagePath);
        QString value = mds.value("DateTimeOriginal");
        DBImgInfo dbi;
        dbi.fileName = fi.fileName();
        dbi.filePath = imagePath;
        dbi.dirHash = utils::base::hash(QString());
        if ("" != value) {
            dbi.time = QDateTime::fromString(value, "yyyy/MM/dd hh:mm");
        } else if (fi.birthTime().isValid()) {
            dbi.time = fi.birthTime();
        } else if (fi.metadataChangeTime().isValid()) {
            dbi.time = fi.metadataChangeTime();
        } else {
            dbi.time = QDateTime::currentDateTime();
        }
        dbi.changeTime = QDateTime::currentDateTime();

        dbInfos << dbi;

        emit dApp->signalM->progressOfWaitDialog(image_list.size(), dbInfos.size());
    }

    if (bneedstop) {
        m_obj->imageImported(false);
        m_obj->removeThread(this);
        return;
    }
    DBImgInfoList tempdbInfos;
    for (auto Info : dbInfos) {
        QFileInfo fi(Info.filePath);
        if (!fi.exists())
            continue;
        tempdbInfos << Info;
    }
    if (image_list.length() == tempdbInfos.length() && !tempdbInfos.isEmpty()) {
        dApp->m_imageloader->ImportImageLoader(tempdbInfos, m_albumname);
        m_obj->imageImported(true);
    } else {
        emit dApp->signalM->ImportFailed();
        m_obj->imageImported(false);
    }
    m_obj->removeThread(this);
}

ImageRecoveryImagesFromTrashThread::ImageRecoveryImagesFromTrashThread()
{
    setAutoDelete(true);
}

void ImageRecoveryImagesFromTrashThread::setData(QStringList paths)
{
    m_paths = paths;
}

void ImageRecoveryImagesFromTrashThread::run()
{
    QStringList paths = m_paths;

    DBImgInfoList infos;
    for (auto path : paths) {
        DBImgInfo info;
        info = DBManager::instance()->getTrashInfoByPath(path);
        QFileInfo fi(info.filePath);
        info.changeTime = QDateTime::currentDateTime();
        infos << info;
    }
    DBManager::instance()->insertImgInfos(infos);

    for (auto path : paths) {
        DBImgInfo info;
        info = DBManager::instance()->getTrashInfoByPath(path);
        QStringList namelist = info.albumname.split(",");
        for (auto eachname : namelist) {
            if (DBManager::instance()->isAlbumExistInDB(eachname)) {
                DBManager::instance()->insertIntoAlbum(eachname, QStringList(path));
            }
        }
    }
    DBManager::instance()->removeTrashImgInfos(paths);
    emit dApp->signalM->closeWaitDialog();
}

ImageMoveImagesToTrashThread::ImageMoveImagesToTrashThread()
{
    setAutoDelete(true);
}

void ImageMoveImagesToTrashThread::setData(QStringList paths, bool typetrash)
{
    m_paths = paths;
    btypetrash = typetrash;
}

void ImageMoveImagesToTrashThread::run()
{
    QStringList paths = m_paths;
    if (btypetrash) {
        DBManager::instance()->removeTrashImgInfos(paths);
        emit dApp->signalM->sigDeletePhotos(paths.length());
    } else {
        DBImgInfoList infos;
        for (auto path : paths) {
            DBImgInfo info;
            info = DBManager::instance()->getInfoByPath(path);
            info.changeTime = QDateTime::currentDateTime();
            QStringList allalbumnames = DBManager::instance()->getAllAlbumNames();
            for (auto eachname : allalbumnames) {
                if (DBManager::instance()->isImgExistInAlbum(eachname, path)) {
                    info.albumname += (eachname + ",");
                }
            }
            infos << info;
            emit dApp->signalM->progressOfWaitDialog(paths.size(), infos.size());

        }
        DBManager::instance()->insertTrashImgInfos(infos);
        DBManager::instance()->removeImgInfos(paths);
    }
    emit dApp->signalM->closeWaitDialog();
}

ImageImportFilesFromMountThread::ImageImportFilesFromMountThread()
{
    setAutoDelete(true);
}

void ImageImportFilesFromMountThread::setData(QString albumname, QStringList paths, ImageMountImportPathsObject *imgobject)
{
    m_paths = paths;
    m_imgobject = imgobject;
    m_albumname = albumname;
}

bool ImageImportFilesFromMountThread::ifCanStopThread(void *imgobject)
{
    static_cast<ImageMountImportPathsObject *>(imgobject)->removeThread(this);
    if (imgobject == m_imgobject) {
        return true;
    }
    return false;
}

void ImageImportFilesFromMountThread::run()
{
    if (bneedstop) {
        return;
    }
    QStringList newPathList;
    DBImgInfoList dbInfos;
    QString strHomePath = QDir::homePath();
    //获取系统现在的时间
    QString strDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString basePath = QString("%1%2%3").arg(strHomePath, "/Pictures/照片/", strDate);
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
            qDebug() << "onCopyPhotoFromPhone()";
        }
        auto mds = getAllMetaData(strNewPath);
        QString value = mds.value("DateTimeOriginal");
        DBImgInfo dbi;
        dbi.fileName = fi.fileName();
        dbi.filePath = strNewPath;
        dbi.dirHash = utils::base::hash(QString());
        if ("" != value) {
            dbi.time = QDateTime::fromString(value, "yyyy/MM/dd hh:mm:ss");
        } else if (fi.birthTime().isValid()) {
            dbi.time = fi.birthTime();
        } else if (fi.metadataChangeTime().isValid()) {
            dbi.time = fi.metadataChangeTime();
        } else {
            dbi.time = QDateTime::currentDateTime();
        }

        dbi.changeTime = QDateTime::currentDateTime();

        dbInfos << dbi;

        emit dApp->signalM->progressOfWaitDialog(m_paths.size(), dbInfos.size());
    }
    if (!dbInfos.isEmpty()) {
        DBImgInfoList dbInfoList;
        QStringList pathslist;

        for (int i = 0; i < dbInfos.length(); i++) {
            if (bneedstop) {
                return;
            }
            pathslist << dbInfos[i].filePath;
            dbInfoList << dbInfos[i];
        }

        if (m_albumname.length() > 0) {
            DBManager::instance()->insertIntoAlbumNoSignal(m_albumname, pathslist);
        }
        DBManager::instance()->insertImgInfos(dbInfoList);

        if (bneedstop) {
            return;
        }
        if (dbInfoList.length() != m_paths.length()) {
            int successful = dbInfoList.length();
            int failed = m_paths.length() - dbInfoList.length();
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

ImageGetFilesFromMountThread::ImageGetFilesFromMountThread()
{
    setAutoDelete(true);
}

void ImageGetFilesFromMountThread::setData(QString mountname, QString path, ImageMountGetPathsObject *imgobject)
{
    m_mountname = mountname;
    m_path = path;
    m_imgobject = imgobject;
}

bool ImageGetFilesFromMountThread::ifCanStopThread(void *imgobject)
{
    static_cast<ImageMountGetPathsObject *>(imgobject)->removeThread(this);
    if (imgobject == m_imgobject) {
        return true;
    }
    return false;
}

//搜索手机中存储相机照片文件的路径，采用两级文件目录深度，找"DCIM"文件目录
//经过调研，安卓手机在path/外部存储设备/DCIM下，iPhone在patn/DCIM下
bool ImageGetFilesFromMountThread::findPicturePathByPhone(QString &path)
{
    QDir dir(path);
    if (!dir.exists()) return false;
    QFileInfoList fileInfoList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    QFileInfo tempFileInfo;
    foreach (tempFileInfo, fileInfoList) {
        if (tempFileInfo.fileName().compare(ALBUM_PATHNAME_BY_PHONE) == 0) {
            path = tempFileInfo.absoluteFilePath();
            return true;
        } else {
            QDir subDir;
            subDir.setPath(tempFileInfo.absoluteFilePath());
            QFileInfoList subFileInfoList = subDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
            QFileInfo subTempFileInfo;
            foreach (subTempFileInfo, subFileInfoList) {
                if (subTempFileInfo.fileName().compare(ALBUM_PATHNAME_BY_PHONE) == 0) {
                    path = subTempFileInfo.absoluteFilePath();
                    return true;
                }
            }
            return false;
        }
    }
    return false;
}

void ImageGetFilesFromMountThread::run()
{
    if (bneedstop) {
        return;
    }
    QString strPath = m_path;
    //获取所选文件类型过滤器
    QStringList filters;
    filters << QString("*.jpeg") << QString("*.jpg")
            << QString("*.bmp") << QString("*.png")
            << QString("*.gif")
            << QString("*.JPEG") << QString("*.JPG")
            << QString("*.BMP") << QString("*.PNG")
            << QString("*.GIF")
            ;
    //定义迭代器并设置过滤器，包括子目录：QDirIterator::Subdirectories
    QDirIterator dir_iterator(m_path,
                              filters,
                              QDir::Files | QDir::NoSymLinks,
                              QDirIterator::Subdirectories);
    QStringList allfiles;
    while (dir_iterator.hasNext()) {
        if (bneedstop || ImageEngineApi::instance()->closeFg()) {

            return;
        }
        dir_iterator.next();
        QFileInfo fileInfo = dir_iterator.fileInfo();
        allfiles << fileInfo.filePath();
    }
    if (bneedstop) {
        return;
    }
    emit sigImageFilesGeted(m_imgobject, allfiles, m_path);
    m_imgobject->removeThread(this);
    emit dApp->signalM->sigLoadMountImagesEnd(m_mountname);
}

ImageLoadFromDBThread::ImageLoadFromDBThread(int loadCount)
{
    m_loadCount = loadCount;
    setAutoDelete(true);
}

void ImageLoadFromDBThread::setData(ThumbnailDelegate::DelegateType type, ImageEngineObject *imgobject, QString nametype)
{
    m_type = type;
    m_imgobject = imgobject;
    m_nametype = nametype;
}

bool ImageLoadFromDBThread::ifCanStopThread(void *imgobject)
{
    static_cast<ImageEngineObject *>(imgobject)->removeThread(this, false);
    if (imgobject == m_imgobject) {
        return true;
    }
    return false;
}

void ImageLoadFromDBThread::run()
{
    if (bneedstop) {
        return;
    }
    QStringList image_list;
    QStringList fail_image_list;
    if (ThumbnailDelegate::AllPicViewType == m_type) {
        auto infos = DBManager::instance()->getAllInfos(m_loadCount);
        for (auto info : infos) {
            //记录源文件不存在的数据
            if (!QFileInfo(info.filePath).exists()) {
                fail_image_list << info.filePath;
                emit dApp->signalM->updatePicView(0);
                continue;
            }
            image_list << info.filePath;
            if (bneedstop || ImageEngineApi::instance()->closeFg()) {
                return;
            }
            emit sigInsert(info.filePath);
        }
        if (m_nametype.isEmpty())
            ImageEngineApi::instance()->SaveImagesCache(image_list);
    }
    if (bneedstop) {
        return;
    }

    //删除数据库失效的图片
    DBManager::instance()->removeImgInfosNoSignal(fail_image_list);
    //先处理图片再存数据库
    emit sigImageLoaded(m_imgobject, image_list);

    m_imgobject->removeThread(this);
}

ImageLoadFromLocalThread::ImageLoadFromLocalThread()
{
    setAutoDelete(true);
}

void ImageLoadFromLocalThread::setData(QStringList filelist, ImageEngineObject *imgobject, bool needcheck, DataType type)
{
    m_filelist = filelist;
    m_imgobject = imgobject;
    bneedcheck = needcheck;
    if (type == DataType_NULL)
        m_type = DataType_StrList;
    else
        m_type = type;
}

bool ImageLoadFromLocalThread::ifCanStopThread(void *imgobject)
{
    static_cast<ImageEngineObject *>(imgobject)->removeThread(this, false);
    if (imgobject == m_imgobject) {
        return true;
    }
    return false;
}

void ImageLoadFromLocalThread::setData(DBImgInfoList filelist, ImageEngineObject *imgobject, bool needcheck, DataType type)
{
    m_fileinfolist = filelist;
    m_imgobject = imgobject;
    bneedcheck = needcheck;
    if (type == DataType_NULL)
        m_type = DataType_InfoList;
    else
        m_type = type;
}

QStringList ImageLoadFromLocalThread::checkImage(const QString  path)
{
    QStringList imagelist;
    QDir dir(path);
    if (!dir.exists()) {
        return imagelist;
    }
    QFileInfoList dirlist = dir.entryInfoList(QDir::Dirs);
    foreach (QFileInfo e_dir, dirlist) {
        if (bneedstop)
            return imagelist;
        if (e_dir.fileName() == "." || e_dir.fileName() == "..") {
            continue;
        }
        if (e_dir.exists()) {
            imagelist << checkImage(e_dir.filePath());
        }
    }
    static QStringList sList;
    for (const QByteArray &i : QImageReader::supportedImageFormats())
        sList << "*." + QString::fromLatin1(i);
    dir.setNameFilters(sList);
    for (int i = 0; i < static_cast<int>(dir.count()); i++) {
        if (bneedstop)
            return imagelist;
        QString ImageName  = dir[i];
        bool checkok = false;
        if (bneedcheck) {
            if (utils::image::checkFileType(path + QDir::separator() + ImageName))
                checkok = true;
        } else {
            checkok = true;
        }
        if (checkok) {
            imagelist << path + QDir::separator() + ImageName;
            sigInsert(path + QDir::separator() + ImageName);
            qDebug() << path + QDir::separator() + ImageName;//输出照片名
        }
    }

    return imagelist;
}

void ImageLoadFromLocalThread::run()
{
    if (bneedstop) {
        return;
    }
    QStringList image_list;
    switch (m_type) {
    case DataType_StrList:
        if (!m_filelist.isEmpty()) {
            for (const QString &path : m_filelist) {
                QFileInfo file(path);
                if (false) {
                } else {
                    bool checkok = false;
                    if (bneedcheck) {
                        if (utils::image::checkFileType(path))
                            checkok = true;
                    } else {
                        checkok = true;
                    }
                    if (checkok) {
                        image_list << path;
                        emit sigInsert(path);
                    }
                }

            }

        }
        break;
    case DataType_InfoList:
        if (!m_fileinfolist.isEmpty()) {
            for (auto info : m_fileinfolist) {
                if (bneedstop) {
                    return;
                }
                image_list << info.filePath;
                emit sigInsert(info.filePath);
            }
        }
        break;
    case DataType_TrashList:
        if (!m_fileinfolist.isEmpty()) {
            QStringList removepaths;
            int idaysec = 24 * 60 * 60;
            for (auto info : m_fileinfolist) {
                if (bneedstop) {
                    return;
                }
                QDateTime start = QDateTime::currentDateTime();
                QDateTime end = info.changeTime;
                int etime = static_cast<int>(start.toTime_t());
                int stime = static_cast<int>(end.toTime_t());
                int Day = (etime - stime) / (idaysec) + ((etime - stime) % (idaysec) + (idaysec - 1)) / (idaysec) - 1;
                if (30 <= Day) {
                    removepaths << info.filePath;
                } else {
                    QString remainDay = QString::number(30 - Day) + tr("days");
                    image_list << info.filePath;
                    emit sigInsert(info.filePath, remainDay);
                }
            }
            if (0 < removepaths.length()) {
                DBManager::instance()->removeTrashImgInfosNoSignal(removepaths);
            }
        }
        break;
    default:
        break;
    }

    if (bneedstop) {
        return;
    }
    if (nullptr != m_imgobject) {
        m_imgobject->removeThread(this);
        emit sigImageLoaded(m_imgobject, image_list);
    }
}

ImageEngineThread::ImageEngineThread()
{
    m_imgobject.clear();
    setAutoDelete(true);
}


ImageEngineThread::~ImageEngineThread()
{

}

void ImageEngineThread::setData(QString path, ImageEngineObject *imgobject, ImageDataSt &data, bool needcache)
{
    m_path = path;
    m_imgobject << imgobject;
    m_data = data;
    bneedcache = needcache;
}


bool ImageEngineThread::ifCanStopThread(void *imgobject)
{
    if (nullptr != imgobject && ImageEngineApi::instance()->ifObjectExist(imgobject))
        static_cast<ImageEngineObject *>(imgobject)->removeThread(this, false);
    m_imgobject.removeOne(static_cast<ImageEngineObject *>(imgobject));
    if (m_imgobject.size() < 1) {
        bneedstop = true;
        return true;
    }
    return false;
}

bool ImageEngineThread::getNeedStop()
{
    if (!bneedstop) {
        return false;
    }
    baborted = true;
    bneedstop = false;
    emit sigAborted(m_path);
    while (!bneedstop) {
        QThread::msleep(50);
    }
    if (!baborted) {
        bneedstop = false;
        return false;
    }
    return true;
}

bool ImageEngineThread::addObject(ImageEngineObject *imgobject)
{
    if (baborted) {
        baborted = false;
        QMutexLocker mutex(&m_mutex);
        m_imgobject << imgobject;
        return false;
    }
    if (bneedstop) {
        bneedstop = false;
    }
    if (!bwaitstop) {
        QMutexLocker mutex(&m_mutex);
        m_imgobject << imgobject;
    } else {
        imgobject->removeThread(this);
        emit sigImageLoaded(imgobject, m_path, m_data);
    }
    return true;
}

//载入QPixmap
void ImageEngineThread::run()
{
    if (getNeedStop())
        return;
    if (!QFileInfo(m_path).exists()) {
        emit sigAborted(m_path);
        return;
    }
    using namespace UnionImage_NameSpace;
    QImage tImg;
    bool cache_exist = false;
    QString path = m_path;
    QFileInfo file(CACHE_PATH + m_path);
    QString errMsg;
    QFileInfo srcfi(m_path);
    if (file.exists()) {
        QDateTime cachetime = file.metadataChangeTime();    //缓存修改时间
        QDateTime srctime = srcfi.metadataChangeTime();     //源数据修改时间
        if (srctime.toTime_t() > cachetime.toTime_t()) {  //源文件近期修改过，重新生成缓存文件
            cache_exist = false;
            breloadCache = true;
            path = m_path;
            if (!loadStaticImageFromFile(path, tImg, errMsg)) {
                qDebug() << errMsg;
            }
        } else {
            cache_exist = true;
            path = CACHE_PATH + m_path;
            if (!loadStaticImageFromFile(path, tImg, errMsg, "PNG")) {
                qDebug() << errMsg;
            }
        }
    } else {
        if (!loadStaticImageFromFile(path, tImg, errMsg)) {
            qDebug() << errMsg;
        }
    }
    if (getNeedStop())
        return;
    QPixmap pixmap = QPixmap::fromImage(tImg);
    if (0 != pixmap.height() && 0 != pixmap.width() && (pixmap.height() / pixmap.width()) < 10 && (pixmap.width() / pixmap.height()) < 10) {
        if (pixmap.height() != 200 && pixmap.width() != 200) {
            if (pixmap.height() >= pixmap.width()) {
                cache_exist = true;
                pixmap = pixmap.scaledToWidth(200,  Qt::FastTransformation);
            } else if (pixmap.height() <= pixmap.width()) {
                cache_exist = true;
                pixmap = pixmap.scaledToHeight(200,  Qt::FastTransformation);
            }
        }
        if (!cache_exist) {
            if ((static_cast<float>(pixmap.height()) / (static_cast<float>(pixmap.width()))) > 3) {
                pixmap = pixmap.scaledToWidth(200,  Qt::FastTransformation);
            } else {
                pixmap = pixmap.scaledToHeight(200,  Qt::FastTransformation);
            }
        }
    }
    if (pixmap.isNull()) {
        qDebug() << "null pixmap" << tImg;
        pixmap = QPixmap::fromImage(tImg);
    }
    m_data.imgpixmap = pixmap;
    QFileInfo fi(m_path);
    auto mds = getAllMetaData(m_path);
    QString value = mds.value("DateTimeOriginal");
    DBImgInfo dbi;
    dbi.fileName = fi.fileName();
    dbi.filePath = m_path;
    dbi.dirHash = utils::base::hash(QString());
    if ("" != value) {
        dbi.time = QDateTime::fromString(value, "yyyy/MM/dd hh:mm");
    } else if (fi.birthTime().isValid()) {
        dbi.time = fi.birthTime();
    } else if (fi.metadataChangeTime().isValid()) {
        dbi.time = fi.metadataChangeTime();
    } else {
        dbi.time = QDateTime::currentDateTime();
    }
    dbi.changeTime = QDateTime::currentDateTime();
    m_data.dbi = dbi;
    m_data.loaded = ImageLoadStatu_Loaded;
    if (getNeedStop()) {
        return;
    }
    bwaitstop = true;
    if (breloadCache) { //更新缓存文件
        QString spath = CACHE_PATH + m_path;
        utils::base::mkMutiDir(spath.mid(0, spath.lastIndexOf('/')));
        pixmap.save(spath, "PNG");
    }

    QMutexLocker mutex(&m_mutex);
    for (ImageEngineObject *imgobject : m_imgobject) {
        imgobject->removeThread(this);
        emit sigImageLoaded(imgobject, m_path, m_data);
    }
    //这个代码不可注释，是线程池线程自我释放的检测，调小检测时间可以提高执行速度
    while (!bneedstop && !ImageEngineApi::instance()->closeFg()) {
        QThread::msleep(100);
    }
}

ImageFromNewAppThread::ImageFromNewAppThread()
{
    setAutoDelete(true);
}

void ImageFromNewAppThread::setDate(QStringList files, ImageEngineImportObject *obj)
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

void ImageFromNewAppThread::run()
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
        if (!imageSupportRead(path)) continue;
        QFileInfo fi(path);
        using namespace utils::image;
        using namespace utils::base;
        auto mds = getAllMetaData(path);
        QString value = mds.value("DateTimeOriginal");
        DBImgInfo dbi;
        dbi.fileName = fi.fileName();
        dbi.filePath = path;
        dbi.dirHash = utils::base::hash(QString());
        if ("" != value) {
            dbi.time = QDateTime::fromString(value, "yyyy/MM/dd hh:mm:ss");
        } else if (fi.birthTime().isValid()) {
            dbi.time = fi.birthTime();
        } else if (fi.metadataChangeTime().isValid()) {
            dbi.time = fi.metadataChangeTime();
        } else {
            dbi.time = QDateTime::currentDateTime();
        }
        dbi.changeTime = QDateTime::currentDateTime();
        dbInfos << dbi;
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

ImageCacheQueuePopThread::ImageCacheQueuePopThread()
{
    setAutoDelete(true);
}

void ImageCacheQueuePopThread::saveCache(QString m_path)
{
    if (needStop || m_path.isEmpty()) {
        qDebug() << "m_path empty";
        return;
    }
    QImage tImg;
    bool cache_exist = false;
    QString path = m_path;
    QFileInfo file(CACHE_PATH + path);
    if (needStop)
        return;
    if (file.exists()) {
        return;
    }
    if (needStop)
        return;
    QString errMsg;
    if (!UnionImage_NameSpace::loadStaticImageFromFile(path, tImg, errMsg)) {
        qDebug() << errMsg;
        return;
    }
    if (needStop)
        return;
    QPixmap pixmap = QPixmap::fromImage(tImg);
    if (0 != pixmap.height() && 0 != pixmap.width() && (pixmap.height() / pixmap.width()) < 10 && (pixmap.width() / pixmap.height()) < 10) {
        if (pixmap.height() != 200 && pixmap.width() != 200) {
            if (pixmap.height() >= pixmap.width()) {
                cache_exist = true;
                pixmap = pixmap.scaledToWidth(200,  Qt::FastTransformation);
            } else if (pixmap.height() <= pixmap.width()) {
                cache_exist = true;
                pixmap = pixmap.scaledToHeight(200,  Qt::FastTransformation);
            }
        }
        if (!cache_exist) {
            if (static_cast<float>(pixmap.height()) / static_cast<float>(pixmap.width()) > 3) {
                pixmap = pixmap.scaledToWidth(200,  Qt::FastTransformation);
            } else {
                pixmap = pixmap.scaledToHeight(200,  Qt::FastTransformation);
            }
        }
    }
    QString spath = CACHE_PATH + m_path;
    if (needStop)
        return;
    utils::base::mkMutiDir(spath.mid(0, spath.lastIndexOf('/')));
    pixmap.save(spath, "PNG");
}

void ImageCacheQueuePopThread::run()
{
    while (!m_obj->isEmpty() && !needStop && !ImageEngineApi::instance()->closeFg()) {
        QString res = m_obj->pop();
        if (!res.isEmpty()) {
            saveCache(res);
        }
    }
    qDebug() << "Cachethread end,there threads:" << ImageEngineApi::instance()->CacheThreadNum() - 1;
}

ImageEngineBackThread::ImageEngineBackThread(): m_bpause(false)
{
    setAutoDelete(true);
    connect(dApp->signalM, &SignalManager::sigDevStop, this, [ = ](QString devName) {
        if (devName == m_devName || devName.isEmpty()) {
            bbackstop = true;
        }
    });

    connect(dApp->signalM, &SignalManager::sigPauseOrStart, this, &ImageEngineBackThread::onStartOrPause);
}

void ImageEngineBackThread::setData(QStringList pathlist, QString devName)
{
    m_pathlist = pathlist;
    m_devName = devName;
}

void ImageEngineBackThread::run()
{
    using namespace UnionImage_NameSpace;
    for (auto temppath : m_pathlist) {
        QImage tImg;
        bool cache_exist = false;
        QString path = temppath;
        QString errMsg;
        if (!loadStaticImageFromFile(path, tImg, errMsg)) {
            qDebug() << errMsg;
            break;
        }
//        QString format = DetectImageFormat(path);
//        if (format.isEmpty()) {
//            QImageReader reader(path);
//            reader.setAutoTransform(true);
//            if (reader.canRead()) {
//                tImg = reader.read();
//            } else if (path.contains(".tga")) {
//                bool ret = false;
//                tImg = utils::image::loadTga(path, ret);
//            }
//        } else  {
//            if (bbackstop || ImageEngineApi::instance()->closeFg())
//                return;
//            QImageReader readerF(path, format.toLatin1());
//            readerF.setAutoTransform(true);
//            if (readerF.canRead()) {
//                tImg = readerF.read();
//            } else {
//                qWarning() << "can't read image:" << readerF.errorString()
//                           << format;
//                tImg = QImage(path);
//            }
//        }

//        if (tImg.isNull()) {
//            QImageReader readerG(temppath, QFileInfo(temppath).suffix().toLatin1());
//            if (readerG.canRead())
//                tImg = readerG.read();
//        }

        if (bbackstop || ImageEngineApi::instance()->closeFg())
            return;

        QPixmap pixmap = QPixmap::fromImage(tImg);
        if (0 != pixmap.height() && 0 != pixmap.width() && (pixmap.height() / pixmap.width()) < 10 && (pixmap.width() / pixmap.height()) < 10) {
            if (pixmap.height() != 100 && pixmap.width() != 100) {
                if (pixmap.height() >= pixmap.width()) {
                    cache_exist = true;
                    pixmap = pixmap.scaledToWidth(100,  Qt::FastTransformation);
                } else if (pixmap.height() <= pixmap.width()) {
                    cache_exist = true;
                    pixmap = pixmap.scaledToHeight(100,  Qt::FastTransformation);
                }
            }
            if (!cache_exist) {
                if ((static_cast<float>(pixmap.height()) / (static_cast<float>(pixmap.width()))) > 3) {
                    pixmap = pixmap.scaledToWidth(100,  Qt::FastTransformation);
                } else {
                    pixmap = pixmap.scaledToHeight(100,  Qt::FastTransformation);
                }
            }
        }
        if (pixmap.isNull()) {
            qDebug() << "[ImageEngineBackThread]:null pixmap!" << tImg;
            pixmap = QPixmap::fromImage(tImg);
        }
        m_data.imgpixmap = pixmap;
        QFileInfo fi(temppath);
        if (bbackstop || ImageEngineApi::instance()->closeFg())
            return;
        auto mds = getAllMetaData(temppath);
        QString value = mds.value("DateTimeOriginal");
        DBImgInfo dbi;
        dbi.fileName = fi.fileName();
        dbi.filePath = temppath;
        dbi.dirHash = utils::base::hash(QString());
        if ("" != value) {
            dbi.time = QDateTime::fromString(value, "yyyy/MM/dd hh:mm");
        } else if (fi.birthTime().isValid()) {
            dbi.time = fi.birthTime();
        } else if (fi.metadataChangeTime().isValid()) {
            dbi.time = fi.metadataChangeTime();
        } else {
            dbi.time = QDateTime::currentDateTime();
        }
        dbi.changeTime = QDateTime::currentDateTime();
        m_data.dbi = dbi;
        m_data.loaded = ImageLoadStatu_Loaded;

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

RotateSaveThread::RotateSaveThread()
{
    setAutoDelete(true);
}

void RotateSaveThread::setDatas(QHash<QString, RotateSaveRequest> requests_bar)
{
    for (RotateSaveRequest i : requests_bar) {
        m_requests.append(i);
    }
}

void RotateSaveThread::run()
{
    for (RotateSaveRequest i : m_requests) {
        QString errorMsg;
        if (!UnionImage_NameSpace::rotateImageFIle(static_cast<int>(i.angel), i.path, errorMsg)) {
            qDebug() << errorMsg;
            qDebug() << "Save error";
        } else {
            qDebug() << "Save Success";
            dApp->m_imageloader->updateImageLoader(QStringList(i.path));
        }
    }
    if (m_requests.empty()) {
        qDebug() << "No Pic and Run Thread";
    } else {
        qDebug() << "Save End";
    }

}

ImageRotateThreadControler::ImageRotateThreadControler()
{
    wait = new QTimer(this);
    rotateThreadPool.setMaxThreadCount(5);
    connect(wait, &QTimer::timeout, this, &ImageRotateThreadControler::startSave);
}

ImageRotateThreadControler::~ImageRotateThreadControler()
{
    rotateThreadPool.waitForDone();
}
void ImageRotateThreadControler::addRotateAndSave(RotateSaveRequest request, int time_gap)
{
    if (NoRepeatRequest.contains(request.path)) {
        NoRepeatRequest[request.path].angel += request.angel;
        emit updateRotate(static_cast<int>(NoRepeatRequest[request.path].angel));
    } else {
        NoRepeatRequest.insert(request.path, request);
    }
    wait->start(time_gap);
}

void ImageRotateThreadControler::startSave()
{
    emit updateRotate(0);
    RotateSaveThread *thread = new RotateSaveThread;
    thread->setDatas(NoRepeatRequest);
    NoRepeatRequest.clear();
    rotateThreadPool.start(thread);
    wait->stop();
}
