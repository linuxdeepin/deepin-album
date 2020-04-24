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
#include "utils/snifferimageformat.h"
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
//    for (auto info : dbInfos) {
//        QImage tImg;

//        QString format = DetectImageFormat(info.filePath);
//        if (format.isEmpty()) {
//            QImageReader reader(info.filePath);
//            reader.setAutoTransform(true);
//            if (reader.canRead()) {
//                tImg = reader.read();
//            } else if (info.filePath.contains(".tga")) {
//                bool ret = false;
//                tImg = utils::image::loadTga(info.filePath, ret);
//            }
//        } else {
//            QImageReader readerF(info.filePath, format.toLatin1());
//            readerF.setAutoTransform(true);
//            if (readerF.canRead()) {
//                tImg = readerF.read();
//            } else {
//                qWarning() << "can't read image:" << readerF.errorString()
//                           << format;

//                tImg = QImage(info.filePath);
//            }
//        }
//        QPixmap pixmap = QPixmap::fromImage(tImg);

////        if (pixmap.isNull())
////        {
////            pixmap = QPixmap(":/resources/images/other/deepin-album.svg");
////        }

//        pixmap = pixmap.scaledToHeight(IMAGE_HEIGHT_DEFAULT,  Qt::FastTransformation);

//        if (pixmap.isNull()) {
//            pixmap = QPixmap::fromImage(tImg);
//        }
//        m_parent->m_imagemap.insert(info.filePath, pixmap);
//    }

    DBImgInfoList dbInfoList;
    QStringList pathlist;

    for (auto info : dbInfos) {
//        if ( dApp->m_imagemap.value(info.filePath).isNull()) {
//            continue;
//        }
        pathlist << info.filePath;
        dbInfoList << info;
    }

//    if (dbInfoList.size() == dbInfos.size()) {
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
                auto finfos =  utils::image::getImagesInfo(path, false);
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
                image_list << utils::image::checkImage(path);
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
    QList<QExplicitlySharedDataPointer<DGioMount> > mounts = pvfsManager->getMounts();
    for (auto mount : mounts) {
        if (bneedstop) {
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
            if (bneedstop) {
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
//        qDebug() << value;
        DBImgInfo dbi;
        dbi.fileName = fi.fileName();
        dbi.filePath = imagePath;
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

void ImportImagesThread::proDucePic(QString path)
{
    using namespace utils::image;
    using namespace utils::base;
    QImage tImg;
    bool cache_exist = false;
    QString cache_path = path;
    QFileInfo file(CACHE_PATH + path);
    if (file.exists()) {
        cache_exist = true;
        cache_path = CACHE_PATH + path;
        return;
    }

    QString format = DetectImageFormat(cache_path);
    if (format.isEmpty()) {
        QImageReader reader(cache_path);
        reader.setAutoTransform(true);
        if (reader.canRead()) {
            tImg = reader.read();
        } else if (cache_path.contains(".tga")) {
            bool ret = false;
            tImg = utils::image::loadTga(cache_path, ret);
        }
    } else {
        QImageReader readerF(cache_path, format.toLatin1());
        readerF.setAutoTransform(true);
        if (readerF.canRead()) {
            tImg = readerF.read();
        } else {
            qWarning() << "can't read image:" << readerF.errorString()
                       << format;
            tImg = QImage(path);
        }
    }

    QPixmap pixmap = QPixmap::fromImage(tImg);
    if (pixmap.isNull()) {
        qDebug() << "pixmap.isNull(),   生成缩略图失败！";
        return;
    }

    if (pixmap.height() < 100) {
        cache_exist = true;
        pixmap = pixmap.scaledToHeight(100,  Qt::FastTransformation);
    } else if (pixmap.width() < 100) {
        cache_exist = true;
        pixmap = pixmap.scaledToWidth(100,  Qt::FastTransformation);
    }

    if (!cache_exist) {
        if (((float)pixmap.height()) / ((float)pixmap.width()) > 3) {
            pixmap = pixmap.scaledToWidth(100,  Qt::FastTransformation);
        } else {
            pixmap = pixmap.scaledToHeight(100,  Qt::FastTransformation);
        }
    }
    if (pixmap.isNull()) {
        pixmap = QPixmap::fromImage(tImg);
    } else {
        if (!cache_exist) {
            QString spath = CACHE_PATH + path;
            mkMutiDir(spath.mid(0, spath.lastIndexOf('/')));
            pixmap.save(spath, "PNG");
        }
    }
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

//        dApp->m_imagetrashmap.remove(path);
    }

//    dApp->m_imageloader->addImageLoader(paths);
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
//        emit dApp->signalM->trashDelete();
    } else {
        DBImgInfoList infos;
        for (auto path : paths) {
            DBImgInfo info;
            info = DBManager::instance()->getInfoByPath(path);
//                    info.time = QDateTime::currentDateTime();
            info.changeTime = QDateTime::currentDateTime();
            QStringList allalbumnames = DBManager::instance()->getAllAlbumNames();
            for (auto eachname : allalbumnames) {
                if (DBManager::instance()->isImgExistInAlbum(eachname, path)) {
                    info.albumname += (eachname + ",");
                }
            }
            infos << info;
            //                dApp->m_imagemap.remove(path);
            emit dApp->signalM->progressOfWaitDialog(paths.size(), infos.size());

        }

//            dApp->m_imageloader->addTrashImageLoader(paths);
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
//    QStringList selectPaths = m_pRightPhoneThumbnailList->selectedPaths();
//    QString albumNameStr = m_importByPhoneComboBox->currentText();
//    QStringList picPathList;
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
        if (bneedstop) {
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

//        if (QFile::copy(strPath, strNewPath)) {
//        picPathList << strPath;
        newPathList << strNewPath;

        QFileInfo fi(strPath);
        using namespace utils::image;
        using namespace utils::base;
        auto mds = getAllMetaData(strPath);
        QString value = mds.value("DateTimeOriginal");
//        qDebug() << value;
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
        if (QFile::copy(strPath, strNewPath)) {
            qDebug() << "onCopyPhotoFromPhone()";
        }
        emit dApp->signalM->progressOfWaitDialog(m_paths.size(), dbInfos.size());
//        }
    }

//    MountLoader *pMountloader = new MountLoader(this);
//    QThread *pLoadThread = new QThread();

//    connect(pMountloader, SIGNAL(needUnMount(QString)), this, SLOT(needUnMount(QString)));
//    pMountloader->moveToThread(pLoadThread);
//    pLoadThread->start();

//    connect(pMountloader, SIGNAL(sigCopyPhotoFromPhone(QStringList, QStringList)), pMountloader, SLOT(onCopyPhotoFromPhone(QStringList, QStringList)));
//    emit pMountloader->sigCopyPhotoFromPhone(picPathList, newPathList);

//    for (int i = 0; i < picPathList.length(); i++) {
//        if (bneedstop) {
//            return;
//        }
//        if (QFile::copy(picPathList[i], newPathList[i])) {
//            qDebug() << "onCopyPhotoFromPhone()";
//        }
//    }

    if (!dbInfos.isEmpty()) {
        DBImgInfoList dbInfoList;
        QStringList pathslist;

        for (int i = 0; i < dbInfos.length(); i++) {
            if (bneedstop) {
                return;
            }
//            if (m_phonePathAndImage.value(picPathList[i]).isNull()) {
//                continue;
//            }

//            dApp->m_imagemap.insert(dbInfos[i].filePath, m_phonePathAndImage.value(picPathList[i]));

            pathslist << dbInfos[i].filePath;
            dbInfoList << dbInfos[i];
        }

        if (m_albumname.length() > 0) {
            if (COMMON_STR_RECENT_IMPORTED != m_albumname
                    && COMMON_STR_TRASH != m_albumname
                    && COMMON_STR_FAVORITES != m_albumname
                    && ALBUM_PATHTYPE_BY_PHONE != m_albumname
                    && 0 != m_albumname.compare(tr("Gallery"))) {
                DBManager::instance()->insertIntoAlbumNoSignal(m_albumname, pathslist);
            }
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
        //m_imgobject->removeThread(this);
        return;
    }
    QString strPath = m_path;
//    //判断路径是否存在
//    QDir dir(m_path);
//    if (!dir.exists()) {
//        dApp->signalM->sigLoadMountImagesEnd(m_mountname);
//        return;
//    }

//    //U盘和硬盘挂载都是/media下的，此处判断若path不包含/media/,在调用findPicturePathByPhone函数搜索DCIM文件目录
//    if (!m_path.contains("/media/")) {
//        bool bFind = findPicturePathByPhone(m_path);
//        if (!bFind) {
//            qDebug() << "onLoadMountImagesStart() !bFind";
//            dApp->signalM->sigLoadMountImagesEnd(m_mountname);
//            return;
//        }
//    }

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
        if (bneedstop) {
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

ImageLoadFromDBThread::ImageLoadFromDBThread()
{
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
//        m_imgobject->removeThread(this);
        return;
    }
    QStringList image_list;
    QStringList fail_image_list;
    if (ThumbnailDelegate::AllPicViewType == m_type) {
        auto infos = DBManager::instance()->getAllInfos();
        for (auto info : infos) {
            //记录源文件不存在的数据
            QFileInfo tempinfo(info.filePath);
            if (!tempinfo.exists()) {
                fail_image_list << info.filePath;
                continue;
            }

            image_list << info.filePath;
            if (bneedstop) {
//                m_imgobject->removeThread(this);
                return;
            }
            emit sigInsert(info.filePath);
        }
    }
    if (bneedstop) {
//        m_imgobject->removeThread(this);
        return;
    }
    ImageEngineApi::instance()->SaveImagesCache(image_list);
    qDebug() << "数据库获取所有路径，type： " << m_type;
    //删除数据库失效的图片
    DBManager::instance()->removeImgInfosNoSignal(fail_image_list);

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
//        m_imgobject->removeThread(this);
        return;
    }
    QStringList image_list;
    switch (m_type) {
    case DataType_StrList:
        if (!m_filelist.isEmpty()) {
            for (const QString &path : m_filelist) {
                //2020年03月24日17:58:55
                QFileInfo file(path);
                if (false) {
                    //qDebug() << "file.isDir()";
                    //image_list << checkImage(path);
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
//                    m_imgobject->removeThread(this);
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
//                    m_imgobject->removeThread(this);
                    return;
                }
                QDateTime start = QDateTime::currentDateTime();
                QDateTime end = info.changeTime;

                uint etime = start.toTime_t();
                uint stime = end.toTime_t();

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
                //        for (auto path : removepaths) {
                //            dApp->m_imagetrashmap.remove(path);
                //        }
                DBManager::instance()->removeTrashImgInfosNoSignal(removepaths);
            }
        }
        break;
    default:
        break;
    }

    if (bneedstop) {
//        m_imgobject->removeThread(this);
        return;
    }
    m_imgobject->removeThread(this);
    emit sigImageLoaded(m_imgobject, image_list);
}

ImageEngineThread::ImageEngineThread()
{
    m_imgobject.clear();
    setAutoDelete(true);
}

void ImageEngineThread::setData(QString path, ImageEngineObject *imgobject, ImageDataSt &data, bool needcache)
{
    m_path = path;
//    m_imgobject = imgobject;
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
        return true;
    }
    return false;
}

bool ImageEngineThread::getNeedStop()
{
//    QMutexLocker mutex(&m_mutex);
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

//QString mkMutiDir(const QString path)   //创建多级目录
//{
//    QDir dir(path);
//    if ( dir.exists(path)) {
//        return path;
//    }
//    QString parentDir = mkMutiDir(path.mid(0, path.lastIndexOf('/')));
//    QString dirname = path.mid(path.lastIndexOf('/') + 1);
//    QDir parentPath(parentDir);
//    if ( !dirname.isEmpty() )
//        parentPath.mkpath(dirname);
//    return parentDir + "/" + dirname;
//}

//载入QPixmap
void ImageEngineThread::run()
{
    if (getNeedStop())
        return;
    using namespace utils::image;
    using namespace utils::base;

    QImage tImg;
    bool cache_exist = false;
    QString path = m_path;
    QFileInfo file(CACHE_PATH + m_path);
    if (file.exists()) {
        cache_exist = true;
        path = CACHE_PATH + m_path;
    }

    QString format = DetectImageFormat(path);
    if (format.isEmpty()) {
        QImageReader reader(path);
        reader.setAutoTransform(true);
        if (reader.canRead()) {
            tImg = reader.read();
        } else if (path.contains(".tga")) {
            bool ret = false;
            tImg = utils::image::loadTga(path, ret);
        }
    } else {
        QImageReader readerF(path, format.toLatin1());
        readerF.setAutoTransform(true);
        if (readerF.canRead()) {
            tImg = readerF.read();
        } else {
            if (cache_exist) {
                QImageReader readerF1(m_path, format.toLatin1());
                readerF1.setAutoTransform(true);
                if (readerF1.canRead()) {
                    tImg = readerF1.read();
                    cache_exist = false;
                } else {
                    qWarning() << "can't read image:" << readerF.errorString()
                               << format;
                    tImg = QImage(m_path);
                }

            } else {
                qWarning() << "can't read image:" << readerF.errorString()
                           << format;
                tImg = QImage(path);
            }
        }
    }

    if (getNeedStop())
        return;
    if (tImg.isNull()) {
        QImageReader readerG(m_path, QFileInfo(m_path).suffix().toLatin1());
        if (readerG.canRead())
            tImg = readerG.read();
        //qDebug() << "ttttttt" << tImg;
    }
    if (getNeedStop())
        return;
    QPixmap pixmap = QPixmap::fromImage(tImg);
//    QPixmap pixmap = QPixmap::fromImage(tImg);
//    if (pixmap.isNull()) {
//        qDebug() << "pixmap.isNull()";
//        return;
//    }

    if (pixmap.height() < 100) {
        cache_exist = true;
        pixmap = pixmap.scaledToHeight(100,  Qt::FastTransformation);
    } else if (pixmap.width() < 100) {
        cache_exist = true;
        pixmap = pixmap.scaledToWidth(100,  Qt::FastTransformation);
    }

    if (!cache_exist) {

        if ((static_cast<float>(pixmap.height()) / (static_cast<float>(pixmap.width()))) > 3) {
            pixmap = pixmap.scaledToWidth(100,  Qt::FastTransformation);
        } else {
            pixmap = pixmap.scaledToHeight(100,  Qt::FastTransformation);
        }
    }
    if (pixmap.isNull()) {
        qDebug() << "null pixmap" << tImg;
        pixmap = QPixmap::fromImage(tImg);
    }
//    else {
//        if (!cache_exist && bneedcache) {
////            QBuffer buffer(&m_baThumb);
////            buffer.open(QIODevice::WriteOnly);
//            QString spath = CACHE_PATH + m_path;
//            mkMutiDir(spath.mid(0, spath.lastIndexOf('/')));
//            pixmap.save(spath, "PNG");
//        }
//    }
    m_data.imgpixmap = pixmap;

    QFileInfo fi(m_path);
    auto mds = getAllMetaData(m_path);
    QString value = mds.value("DateTimeOriginal");


    DBImgInfo dbi;
    dbi.fileName = fi.fileName();
    dbi.filePath = m_path;
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
    m_data.dbi = dbi;
    m_data.loaded = ImageLoadStatu_Loaded;
    if (getNeedStop()) {
        return;
    }
    bwaitstop = true;
    QMutexLocker mutex(&m_mutex);
    for (ImageEngineObject *imgobject : m_imgobject) {
//        QMutexLocker mutex(&m_mutex);
        imgobject->removeThread(this);
        emit sigImageLoaded(imgobject, m_path, m_data);
    }
    //这个代码不可注释，是线程池线程自我释放的检测，调小检测时间可以提高执行速度
    while (!bneedstop) {
        QThread::msleep(20);
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
    //clock_t start = clock();
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
//                qDebug() << value;
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
    //qDebug() << "耗时："<<(double)(clock() - start);
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

ImageSVGConvertThread::ImageSVGConvertThread()
{

}

void ImageSVGConvertThread::setData(QStringList paths, int degree)
{
    m_paths = paths;
    m_degree = degree;
}

void ImageSVGConvertThread::run()
{
    for (QString path : m_paths) {
        QImage pix(path);
        QString dpath = path.right(path.length() - path.lastIndexOf("/") - 1);
        QString strTmpPath = tr("/tmp/%1").arg(dpath);
        QSvgGenerator generator;
        generator.setFileName(strTmpPath);
        //generator.setSize(pix.size());
        generator.setViewBox(pix.rect());
        QPainter painter;
        painter.begin(&generator);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        if (m_degree < 0) {
            painter.translate(0, pix.rect().height());
        } else {
            painter.translate(pix.rect().width(), 0);
        }
        painter.rotate(m_degree);
        painter.drawImage(pix.rect(), pix.scaled(pix.width(), pix.height()));
        generator.setSize(pix.size()); //do not remove this
        painter.end();
        //remove oringnal file
        QFile::remove(path);
        //copy converted image to oringnal path
        QFile::copy(strTmpPath, path);
        //remove tmp file
        QFile::remove(strTmpPath);
    }
    emit updateImages(m_paths);
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
    using namespace utils::image;
    using namespace utils::base;
    QImage tImg;
    bool cache_exist = false;
    QString path = m_path;
    QFileInfo file(CACHE_PATH + m_path);
    if (needStop)
        return;
    if (file.exists()) {
        cache_exist = true;
        path = CACHE_PATH + m_path;
        qDebug() << "Cache exists";
    }
    if (needStop)
        return;
    QString format = DetectImageFormat(path);
    if (format.isEmpty()) {
        QImageReader reader(path);
        reader.setAutoTransform(true);
        if (reader.canRead()) {
            tImg = reader.read();
        } else if (path.contains(".tga")) {
            bool ret = false;
            tImg = utils::image::loadTga(path, ret);
        }
        if (needStop)
            return;
    } else {
        QImageReader readerF(path, format.toLatin1());
        readerF.setAutoTransform(true);
        if (needStop)
            return;
        if (readerF.canRead()) {
            tImg = readerF.read();
        } else {
            if (cache_exist) {
                QImageReader readerF1(m_path, format.toLatin1());
                readerF1.setAutoTransform(true);
                if (readerF1.canRead()) {
                    tImg = readerF1.read();
                    cache_exist = false;
                } else {
                    qWarning() << "can't read image:" << readerF.errorString()
                               << format;
                    tImg = QImage(m_path);
                }

            } else {
                qWarning() << "can't read image:" << readerF.errorString()
                           << format;
                tImg = QImage(path);
            }
        }
    }
    if (needStop)
        return;
    QPixmap pixmap = QPixmap::fromImage(tImg);
    if (pixmap.height() < 100) {
        cache_exist = true;
        pixmap = pixmap.scaledToHeight(100,  Qt::FastTransformation);
    } else if (pixmap.width() < 100) {
        cache_exist = true;
        pixmap = pixmap.scaledToWidth(100,  Qt::FastTransformation);
    }
    if (!cache_exist) {
        if (static_cast<float>(pixmap.height()) / static_cast<float>(pixmap.width()) > 3) {
            pixmap = pixmap.scaledToWidth(100,  Qt::FastTransformation);
        } else {
            pixmap = pixmap.scaledToHeight(100,  Qt::FastTransformation);
        }
    }
    QString spath = CACHE_PATH + m_path;
    if (needStop)
        return;
    mkMutiDir(spath.mid(0, spath.lastIndexOf('/')));
    pixmap.save(spath, "PNG");
}

void ImageCacheQueuePopThread::run()
{
    //QTime time_star;
    //time_star.start();
    while (!m_obj->isEmpty() && !needStop) {
        QString res = m_obj->pop();
        if (!res.isEmpty()) {
            saveCache(res);
        }
    }
    qDebug() << "Cachethread end,there threads:" << ImageEngineApi::instance()->CacheThreadNum() - 1;
    //deleteLater();
}
