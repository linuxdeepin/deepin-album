#include "imageengineapi.h"
#include "controller/signalmanager.h"
#include "application.h"
#include <QMetaType>
#include <QDirIterator>
#include <QStandardPaths>


namespace {
const QString CACHE_PATH = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QDir::separator() + "deepin" + QDir::separator() + "deepin-album"/* + QDir::separator()*/;
}

ImageEngineApi *ImageEngineApi::s_ImageEngine = nullptr;

ImageEngineApi *ImageEngineApi::instance(QObject *parent)
{
    if (nullptr == parent && nullptr == s_ImageEngine) {
        return nullptr;
    }
    if (nullptr != parent && nullptr == s_ImageEngine) {
        s_ImageEngine = new ImageEngineApi(parent);
    }
    return  s_ImageEngine;
}

ImageEngineApi::ImageEngineApi(QObject *parent)
    : QObject(parent)
{
    m_qtpool.setMaxThreadCount(20);
    qRegisterMetaType<QStringList>("QStringList &");
    qRegisterMetaType<ImageDataSt>("ImageDataSt &");
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
    if ( it != m_AllObject.end()) {
        m_AllObject.erase(it);
        return true;
    }
    return false;
}

bool ImageEngineApi::ifObjectExist(void *obj)
{
    QMap<void *, void *>::iterator it;
    it = m_AllObject.find(obj);
    if ( it != m_AllObject.end()) {
        return true;
    }
    return false;
}

bool ImageEngineApi::removeImage(QString imagepath)
{
    QMap<QString, ImageDataSt>::iterator it;
    it = m_AllImageData.find(imagepath);
    if ( it != m_AllImageData.end()) {
        m_AllImageData.erase(it);
        return true;
    }
    return false;
}

bool ImageEngineApi::insertImage(QString imagepath, QString remainDay)
{
    QMap<QString, ImageDataSt>::iterator it;
    it = m_AllImageData.find(imagepath);
    ImageDataSt data;
    if ( it != m_AllImageData.end()) {
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
    if ( it == m_AllImageData.end()) {
        return false;
    }
    data = it.value();
    return true;
}

bool ImageEngineApi::reQuestImageData(QString imagepath, ImageEngineObject *obj, bool needcache)
{

    if (nullptr == obj) {
        return false;
    }
    QMap<QString, ImageDataSt>::iterator it;
    it = m_AllImageData.find(imagepath);
    if ( it == m_AllImageData.end()) {
        return false;
    }
    ImageDataSt data = it.value();
    ((ImageEngineObject *)obj)->addCheckPath(imagepath);
    if (ImageLoadStatu_Loaded == data.loaded) {
        ((ImageEngineObject *)obj)->checkAndReturnPath(imagepath);
    } else if (ImageLoadStatu_BeLoading == data.loaded && nullptr != data.thread) {
        obj->addThread((ImageEngineThreadObject *)data.thread);
        ((ImageEngineThread *)data.thread)->addObject(obj);
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
    if ( it == m_AllImageData.end()) {
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
    ImageEngineThread *thread = (ImageEngineThread *)sender();
    if (nullptr != thread)
        thread->needStop(nullptr);
}

void ImageEngineApi::sltImageLoaded(void *imgobject, QString path, ImageDataSt &data)
{
    m_AllImageData[path] = data;
    ImageEngineThread *thread = (ImageEngineThread *)sender();
    if (nullptr != thread)
        thread->needStop(imgobject);
    if (nullptr != imgobject && ifObjectExist(imgobject)) {
        ((ImageEngineObject *)imgobject)->checkAndReturnPath(path);
    }
}

void ImageEngineApi::sltImageLocalLoaded(void *imgobject, QStringList &filelist)
{
    if (nullptr != imgobject && ifObjectExist(imgobject)) {
        ((ImageEngineObject *)imgobject)->imageLocalLoaded(filelist);
    }
}

void ImageEngineApi::sltImageDBLoaded(void *imgobject, QStringList &filelist)
{
    if (nullptr != imgobject && ifObjectExist(imgobject)) {
        ((ImageEngineObject *)imgobject)->imageFromDBLoaded(filelist);
    }
}

void ImageEngineApi::sltImageFilesGeted(void *imgobject, QStringList &filelist, QString path)
{
    if (nullptr != imgobject && ifObjectExist(imgobject)) {
        ((ImageMountGetPathsObject *)imgobject)->imageGeted(filelist, path);
    }
}

void ImageEngineApi::sltImageFilesImported(void *imgobject, QStringList &filelist)
{
    if (nullptr != imgobject && ifObjectExist(imgobject)) {
        ((ImageMountImportPathsObject *)imgobject)->imageMountImported(filelist);
    }
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
    emit dApp->signalM->popupWaitDialog(tr("Importing... "));
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
    emit dApp->signalM->popupWaitDialog("Importing... ");
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

bool ImageEngineApi::getImageFilesFromMount(QString mountname, QString path, ImageMountGetPathsObject *obj)
{
    ImageGetFilesFromMountThread *imagethread = new ImageGetFilesFromMountThread;
    connect(imagethread, &ImageGetFilesFromMountThread::sigImageFilesGeted, this, &ImageEngineApi::sltImageFilesGeted);
    imagethread->setData(mountname, path, obj);
    obj->addThread(imagethread);
    m_qtpool.start(imagethread);
    return true;
}

bool ImageEngineApi::importImageFilesFromMount(QString albumname, QStringList paths, ImageMountImportPathsObject *obj)
{
    ImageImportFilesFromMountThread *imagethread = new ImageImportFilesFromMountThread;
    connect(imagethread, &ImageImportFilesFromMountThread::sigImageFilesImported, this, &ImageEngineApi::sltImageFilesImported);
    imagethread->setData(albumname, paths, obj);
    obj->addThread(imagethread);
    m_qtpool.start(imagethread);
    return true;
}
