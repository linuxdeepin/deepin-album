#include "imageengineapi.h"
#include "controller/signalmanager.h"
#include "application.h"
#include "imageengineapi.h"
#include <QMetaType>
#include <QDirIterator>
#include <QStandardPaths>


namespace {
const QString CACHE_PATH = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                           + QDir::separator() + "deepin" + QDir::separator() + "deepin-album"/* + QDir::separator()*/;
}

ImageEngineApi *ImageEngineApi::s_ImageEngine = nullptr;

ImageEngineApi *ImageEngineApi::instance(QObject *parent)
{
    Q_UNUSED(parent);
//    if (nullptr == parent && nullptr == s_ImageEngine) {
//        return nullptr;
//    }
//    if (nullptr != parent && nullptr == s_ImageEngine) {
//        s_ImageEngine = new ImageEngineApi(parent);
//    }
    if (!s_ImageEngine) {
        s_ImageEngine = new ImageEngineApi();
    }
    return  s_ImageEngine;
}

ImageEngineApi::ImageEngineApi(QObject *parent)
//    : QObject(nullptr) 和MainApplication绑定是没必要的，api继承object的作用是使用信号槽
{
    Q_UNUSED(parent);
    //文件加载线程池上限
    m_qtpool.setMaxThreadCount(20);
    qRegisterMetaType<QStringList>("QStringList &");
    qRegisterMetaType<ImageDataSt>("ImageDataSt &");
    //m_qtpool.setExpiryTimeout(100);
    cacheThreadPool.setMaxThreadCount(10);
}

bool ImageEngineApi::insertObject(void *obj)
{
    m_AllObject.insert(obj, obj);
    return true;
}
bool ImageEngineApi::removeObject(void *obj)
{
    QMap<void *, void *>::iterator it;
    it = m_AllObject.find(obj);
    if (it != m_AllObject.end()) {
        m_AllObject.erase(it);
        return true;
    }
    return false;
}

bool ImageEngineApi::ifObjectExist(void *obj)
{
    QMap<void *, void *>::iterator it;
    it = m_AllObject.find(obj);
    if (it != m_AllObject.end()) {
        return true;
    }
    return false;
}

bool ImageEngineApi::removeImage(QString imagepath)
{
    QMap<QString, ImageDataSt>::iterator it;
    it = m_AllImageData.find(imagepath);
    if (it != m_AllImageData.end()) {
        m_AllImageData.erase(it);
        return true;
    }
    return false;
}

bool ImageEngineApi::removeImage(QStringList imagepathList)
{
    for (const auto &imagepath : imagepathList) {
        m_AllImageData.remove(imagepath);
    }
    return true;
}

bool ImageEngineApi::insertImage(QString imagepath, QString remainDay)
{
    QMap<QString, ImageDataSt>::iterator it;
    it = m_AllImageData.find(imagepath);
    ImageDataSt data;
    if (it != m_AllImageData.end()) {
        if ("" == remainDay) {
            return false;
        }
        data = it.value();
//        return false;
    }
    if ("" != remainDay)
        data.remainDays = remainDay;
    m_AllImageData.insert(imagepath, data);
    return true;
}

void ImageEngineApi::sltInsert(QString imagepath, QString remainDay)
{
    insertImage(imagepath, remainDay);
}

bool ImageEngineApi::updateImageDataPixmap(QString imagepath, QPixmap &pix)
{
    ImageDataSt data;
    if (getImageData(imagepath, data)) {
        data.imgpixmap = pix;
        m_AllImageData[imagepath] = data;

        QFileInfo file(CACHE_PATH + imagepath);
        if (file.exists()) {
            QFile::remove(CACHE_PATH + imagepath);
        }
        return true;
    }
    return false;
}

bool ImageEngineApi::getImageData(QString imagepath, ImageDataSt &data)
{
    QMap<QString, ImageDataSt>::iterator it;
    it = m_AllImageData.find(imagepath);
    if (it == m_AllImageData.end()) {
        return false;
    }
    data = it.value();
    return true;
}

//载入图片实际位置
bool ImageEngineApi::reQuestImageData(QString imagepath, ImageEngineObject *obj, bool needcache)
{

    if (nullptr == obj) {
        return false;
    }
    QMap<QString, ImageDataSt>::iterator it;
    it = m_AllImageData.find(imagepath);
    if (it == m_AllImageData.end()) {
        return false;
    }
    ImageDataSt data = it.value();
    dynamic_cast<ImageEngineObject *>(obj)->addCheckPath(imagepath);
    if (ImageLoadStatu_Loaded == data.loaded) {
        dynamic_cast<ImageEngineObject *>(obj)->checkAndReturnPath(imagepath);
    } else if (ImageLoadStatu_BeLoading == data.loaded && nullptr != data.thread) {
        obj->addThread(dynamic_cast<ImageEngineThreadObject *>(data.thread));
        dynamic_cast<ImageEngineThread *>(data.thread)->addObject(obj);
    } else {
        ImageEngineThread *imagethread = new ImageEngineThread;
        connect(imagethread, &ImageEngineThread::sigImageLoaded, this, &ImageEngineApi::sltImageLoaded);
        connect(imagethread, &ImageEngineThread::sigAborted, this, &ImageEngineApi::sltAborted);
        data.thread = imagethread;
        data.loaded = ImageLoadStatu_BeLoading;
        m_AllImageData[imagepath] = data;
        imagethread->setData(imagepath, obj, data, needcache);
        obj->addThread(imagethread);
        m_qtpool.start(imagethread);
    }
    return true;
}

bool ImageEngineApi::imageNeedReload(QString imagepath)
{
    QMap<QString, ImageDataSt>::iterator it;
    it = m_AllImageData.find(imagepath);
    if (it == m_AllImageData.end()) {
        return false;
    }
    ImageDataSt data = it.value();
    data.loaded = ImageLoadStatu_False;
    m_AllImageData[imagepath] = data;
    return true;
}

void ImageEngineApi::sltAborted(QString path)
{
    removeImage(path);
    ImageEngineThread *thread = dynamic_cast<ImageEngineThread *>(sender());
    if (nullptr != thread)
        thread->needStop(nullptr);
}

void ImageEngineApi::sltImageLoaded(void *imgobject, QString path, ImageDataSt &data)
{
    m_AllImageData[path] = data;
    ImageEngineThread *thread = dynamic_cast<ImageEngineThread *>(sender());
    if (nullptr != thread)
        thread->needStop(imgobject);
    if (nullptr != imgobject && ifObjectExist(imgobject)) {
        static_cast<ImageEngineObject *>(imgobject)->checkAndReturnPath(path);
    }
}

void ImageEngineApi::sltImageLocalLoaded(void *imgobject, QStringList &filelist)
{
    if (nullptr != imgobject && ifObjectExist(imgobject)) {
        static_cast<ImageEngineObject *>(imgobject)->imageLocalLoaded(filelist);
    }
}

void ImageEngineApi::sltImageDBLoaded(void *imgobject, QStringList &filelist)
{
    if (nullptr != imgobject && ifObjectExist(imgobject)) {
        static_cast<ImageEngineObject *>(imgobject)->imageFromDBLoaded(filelist);
    }
}

void ImageEngineApi::sltImageFilesGeted(void *imgobject, QStringList &filelist, QString path)
{
    if (nullptr != imgobject && ifObjectExist(imgobject)) {
        static_cast<ImageMountGetPathsObject *>(imgobject)->imageGeted(filelist, path);
    }
}

void ImageEngineApi::sltImageFilesImported(void *imgobject, QStringList &filelist)
{
    if (nullptr != imgobject && ifObjectExist(imgobject)) {
        static_cast<ImageMountImportPathsObject *>(imgobject)->imageMountImported(filelist);
    }
}

void ImageEngineApi::sltstopCacheSave()
{
    for (auto i : cacheThreads) {
        i->stopThread();
    }
    cacheThreadPool.waitForDone();
}

void ImageEngineApi::sigImageBackLoaded(QString path, ImageDataSt data)
{
    m_AllImageData[path] = data;
}

bool ImageEngineApi::loadImagesFromTrash(DBImgInfoList files, ImageEngineObject *obj)
{
    ImageLoadFromLocalThread *imagethread = new ImageLoadFromLocalThread;
    connect(imagethread, &ImageLoadFromLocalThread::sigImageLoaded, this, &ImageEngineApi::sltImageLocalLoaded);
    connect(imagethread, &ImageLoadFromLocalThread::sigInsert, this, &ImageEngineApi::sltInsert);
    imagethread->setData(files, obj, true, ImageLoadFromLocalThread::DataType_TrashList);
    obj->addThread(imagethread);
    m_qtpool.start(imagethread);
    return true;
}

bool ImageEngineApi::loadImagesFromLocal(DBImgInfoList files, ImageEngineObject *obj, bool needcheck)
{
    ImageLoadFromLocalThread *imagethread = new ImageLoadFromLocalThread;
    connect(imagethread, &ImageLoadFromLocalThread::sigImageLoaded, this, &ImageEngineApi::sltImageLocalLoaded);
    connect(imagethread, &ImageLoadFromLocalThread::sigInsert, this, &ImageEngineApi::sltInsert);
    imagethread->setData(files, obj, needcheck);
    obj->addThread(imagethread);
    m_qtpool.start(imagethread);
    return true;
}
bool ImageEngineApi::ImportImagesFromUrlList(QList<QUrl> files, QString albumname, ImageEngineImportObject *obj, bool bdialogselect)
{
    emit dApp->signalM->popupWaitDialog(tr("Importing..."));
    ImportImagesThread *imagethread = new ImportImagesThread;
//    connect(imagethread, &ImageLoadFromLocalThread::sigImageLoaded, this, &ImageEngineApi::sltImageLocalLoaded);
//    connect(imagethread, &ImageLoadFromLocalThread::sigInsert, this, &ImageEngineApi::sltInsert);
    imagethread->setData(files, albumname, obj, bdialogselect);
    obj->addThread(imagethread);
    m_qtpool.start(imagethread);
    return true;
}

bool ImageEngineApi::ImportImagesFromFileList(QStringList files, QString albumname, ImageEngineImportObject *obj, bool bdialogselect)
{
    emit dApp->signalM->popupWaitDialog(tr("Importing..."));
    ImportImagesThread *imagethread = new ImportImagesThread;
//    connect(imagethread, &ImageLoadFromLocalThread::sigImageLoaded, this, &ImageEngineApi::sltImageLocalLoaded);
//    connect(imagethread, &ImageLoadFromLocalThread::sigInsert, this, &ImageEngineApi::sltInsert);
    imagethread->setData(files, albumname, obj, bdialogselect);
    obj->addThread(imagethread);
    m_qtpool.start(imagethread);
    return true;
}

bool ImageEngineApi::loadImagesFromLocal(QStringList files, ImageEngineObject *obj, bool needcheck)
{
    ImageLoadFromLocalThread *imagethread = new ImageLoadFromLocalThread;
    connect(imagethread, &ImageLoadFromLocalThread::sigImageLoaded, this, &ImageEngineApi::sltImageLocalLoaded);
    connect(imagethread, &ImageLoadFromLocalThread::sigInsert, this, &ImageEngineApi::sltInsert);
    imagethread->setData(files, obj, needcheck);
    obj->addThread(imagethread);
    m_qtpool.start(imagethread);
    return true;
}
bool ImageEngineApi::loadImagesFromPath(ImageEngineObject *obj, QString path)
{
    sltImageDBLoaded(obj, QStringList() << path );
    insertImage(path, "30");
}

bool ImageEngineApi::loadImageDateToMemory(QStringList pathlist, QString devName)
{
    bool iRet = false;
    //判断是否已经在线程中加载LMH0426
    QStringList tmpPathlist = pathlist;
    if (m_AllImageData.count() > 0) {
        for (auto imagepath : pathlist) {
            if (m_AllImageData.contains(imagepath)) {
                tmpPathlist.removeOne(imagepath);
            }
        }
    }
    if (tmpPathlist.count() > 0) {
        ImageEngineBackThread *imagethread = new ImageEngineBackThread;
        imagethread->setData(tmpPathlist, devName);
        connect(imagethread, &ImageEngineBackThread::sigImageBackLoaded, this, &ImageEngineApi::sigImageBackLoaded, Qt::QueuedConnection);
        m_qtpool.start(imagethread);
        iRet = true;
    } else {
        iRet = false;
    }
    return iRet;
}
bool ImageEngineApi::loadImagesFromDB(ThumbnailDelegate::DelegateType type, ImageEngineObject *obj, QString name)
{
    ImageLoadFromDBThread *imagethread = new ImageLoadFromDBThread;
    connect(imagethread, &ImageLoadFromDBThread::sigImageLoaded, this, &ImageEngineApi::sltImageDBLoaded);
    connect(imagethread, &ImageLoadFromDBThread::sigInsert, this, &ImageEngineApi::sltInsert);
    imagethread->setData(type, obj, name);
    obj->addThread(imagethread);
    m_qtpool.start(imagethread);
    return true;
}

bool ImageEngineApi::SaveImagesCache(QStringList files)
{
    if (!m_imageCacheSaveobj) {
        m_imageCacheSaveobj = new ImageCacheSaveObject;
        connect(dApp->signalM, &SignalManager::cacheThreadStop, this, &ImageEngineApi::sltstopCacheSave);
    }
    m_imageCacheSaveobj->add(files);
    int coreCounts = static_cast<int>(std::thread::hardware_concurrency());
    if (coreCounts * 50 > files.size()) {
        if (files.empty()) {
            coreCounts = 0;
        } else {
            coreCounts = (files.size() / 50) + 1 - cacheThreadPool.activeThreadCount();
        }
    }
    for (int i = 0; i < coreCounts; i++) {
        ImageCacheQueuePopThread *thread = new ImageCacheQueuePopThread;
        thread->setObject(m_imageCacheSaveobj);
        cacheThreadPool.start(thread);
        cacheThreads.append(thread);
        qDebug() << "current Threads:" << cacheThreadPool.activeThreadCount();
    }
    return true;
}

int ImageEngineApi::CacheThreadNum()
{
    return cacheThreadPool.activeThreadCount();
}

//从外部启动，启用线程加载图片
bool ImageEngineApi::loadImagesFromNewAPP(QStringList files, ImageEngineImportObject *obj)
{
    ImageFromNewAppThread *imagethread = new ImageFromNewAppThread;
    //imagethread->setData(type, obj, name);
    imagethread->setDate(files, obj);
    obj->addThread(imagethread);
    m_qtpool.start(imagethread);
    return true;
}

bool ImageEngineApi::getImageFilesFromMount(QString mountname, QString path, ImageMountGetPathsObject *obj)
{
    ImageGetFilesFromMountThread *imagethread = new ImageGetFilesFromMountThread;
    connect(imagethread, &ImageGetFilesFromMountThread::sigImageFilesGeted, this, &ImageEngineApi::sltImageFilesGeted);
    imagethread->setData(mountname, path, obj);
    obj->addThread(imagethread);
    //emit dApp->signalM->waitDevicescan();
    m_qtpool.start(imagethread);

    return true;
}

bool ImageEngineApi::importImageFilesFromMount(QString albumname, QStringList paths, ImageMountImportPathsObject *obj)
{
    emit dApp->signalM->popupWaitDialog(tr("Importing..."));
    ImageImportFilesFromMountThread *imagethread = new ImageImportFilesFromMountThread;
    connect(imagethread, &ImageImportFilesFromMountThread::sigImageFilesImported, this, &ImageEngineApi::sltImageFilesImported);
    imagethread->setData(albumname, paths, obj);
    obj->addThread(imagethread);
    m_qtpool.start(imagethread);
    return true;
}

bool ImageEngineApi::moveImagesToTrash(QStringList files, bool typetrash, bool bneedprogress)
{
    emit dApp->signalM->popupWaitDialog(tr("Deleting..."), bneedprogress); //autor : jia.dong
    if (typetrash)  //如果为回收站删除，则删除内存数据
        removeImage(files);
    ImageMoveImagesToTrashThread *imagethread = new ImageMoveImagesToTrashThread;
    imagethread->setData(files, typetrash);
    m_qtpool.start(imagethread);
    return true;
}

bool ImageEngineApi::recoveryImagesFromTrash(QStringList files)
{
    emit dApp->signalM->popupWaitDialog(tr("Restoring..."), false);
    ImageRecoveryImagesFromTrashThread *imagethread = new ImageRecoveryImagesFromTrashThread;
    imagethread->setData(files);
    m_qtpool.start(imagethread);
    return true;
}

int ImageEngineApi::Getm_AllImageDataNum()
{
    return m_AllImageData.size();
}

bool ImageEngineApi::clearAllImageDate()
{
    if (!m_AllImageData.empty()) {
        m_AllImageData.clear();
        return true;
    }
    return false;
}
