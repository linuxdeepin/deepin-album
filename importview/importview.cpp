#include "importview.h"
#include <DApplicationHelper>
#include <DFileDialog>
#include <QGraphicsDropShadowEffect>
#include <DSuggestButton>
#include <dgiovolumemanager.h>
#include <dgiofile.h>
#include <dgiofileinfo.h>
#include <dgiovolume.h>
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "utils/snifferimageformat.h"
#include "imageengine/imageengineapi.h"

ImportView::ImportView()
    : m_pImportBtn(nullptr), pLabel(nullptr)
{
    setAcceptDrops(true);

    initUI();
    initConnections();
}

void ImportView::initConnections()
{
//    connect(m_pImportBtn, &DPushButton::clicked, this, &ImportView::onImprotBtnClicked);
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, pLabel, [ = ] {
        QPixmap pixmap;
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType)
        {
            pixmap = utils::base::renderSVG(":/resources/images/other/icon_import_photo.svg", QSize(128, 128));
//            if (nullptr != m_pImportBtn) {
//                DPalette pa = DApplicationHelper::instance()->palette(m_pImportBtn);
//                pa.setColor(QPalette::Highlight, QColor(37, 183, 255));
//                m_pImportBtn->setPalette(pa);
//            }
        }
        if (themeType == DGuiApplicationHelper::DarkType)
        {
            pixmap = utils::base::renderSVG(":/resources/images/other/icon_import_photo_dark.svg", QSize(128, 128));
//            if (nullptr != m_pImportBtn) {
//                DPalette pa = DApplicationHelper::instance()->palette(m_pImportBtn);
//                pa.setColor(QPalette::Highlight, QColor(0, 152, 255));
//                m_pImportBtn->setPalette(pa);
//            }
        }
        pLabel->setPixmap(pixmap);
    });
}

void ImportView::initUI()
{
    QVBoxLayout *pImportFrameLayout = new QVBoxLayout();

    pLabel = new DLabel();
    pLabel->setFixedSize(128, 128);

    QPixmap pixmap;
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        pixmap = utils::base::renderSVG(":/resources/images/other/icon_import_photo.svg", QSize(128, 128));
    }
    if (themeType == DGuiApplicationHelper::DarkType) {
        pixmap = utils::base::renderSVG(":/resources/images/other/icon_import_photo_dark.svg", QSize(128, 128));
    }
    pLabel->setPixmap(pixmap);

    m_pImportBtn = new DSuggestButton();
    m_pImportBtn->setFocusPolicy(Qt::NoFocus);
    DFontSizeManager::instance()->bind(m_pImportBtn, DFontSizeManager::T6, QFont::ExtraLight);
    m_pImportBtn->setText(tr("Import Photos"));
    m_pImportBtn->setFixedSize(302, 36);
    m_pImportBtn->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));

//    DPalette pa = DApplicationHelper::instance()->palette(m_pImportBtn);
//    if (themeType == DGuiApplicationHelper::LightType) {
//        pa.setColor(QPalette::Highlight, QColor(37, 183, 255));
//    } else {
//        pa.setColor(QPalette::Highlight, QColor(0, 152, 255));
//    }
//    m_pImportBtn->setPalette(pa);
    DLabel *pLabel2 = new DLabel();
    DFontSizeManager::instance()->bind(pLabel2, DFontSizeManager::T9, QFont::ExtraLight);
    pLabel2->setForegroundRole(DPalette::TextTips);
    pLabel2->setFixedHeight(18);
    pLabel2->setText(tr("Or drag photos here"));

//    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
//    effect->setOffset(0,4);
//    effect->setColor(QColor(0,145,255,77));
//    effect->setBlurRadius(4);
//    m_pImportBtn->setGraphicsEffect(effect);


    pImportFrameLayout->setMargin(0);

    // pImportFrameLayout->setContentsMargins(0,0,0,0);

    pImportFrameLayout->addStretch();
    pImportFrameLayout->addWidget(pLabel, 0, Qt::AlignCenter);
    pImportFrameLayout->addSpacing(5);
    pImportFrameLayout->addWidget(m_pImportBtn, 0, Qt::AlignCenter);
    pImportFrameLayout->addSpacing(10);
    pImportFrameLayout->addWidget(pLabel2, 0, Qt::AlignCenter);
    pImportFrameLayout->addStretch();
    setLayout(pImportFrameLayout);
}

void ImportView::dragEnterEvent(QDragEnterEvent *e)
{
    const QMimeData *mimeData = e->mimeData();
    if (!utils::base::checkMimeData(mimeData)) {
        return;
    }
    e->setDropAction(Qt::CopyAction);
    e->accept();
}

void ImportView::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) {
        return;
    }
    ImageEngineApi::instance()->ImportImagesFromUrlList(urls, m_albumname, this);

    /*    using namespace utils::image;
    //    QStringList paths;
    //    for (QUrl url : urls) {
    //        const QString path = url.toLocalFile();
    //        if (QFileInfo(path).isDir()) {
    //            auto finfos =  getImagesInfo(path, false);
    //            for (auto finfo : finfos) {
    //                if (imageSupportRead(finfo.absoluteFilePath())) {
    //                    paths << finfo.absoluteFilePath();
    //                }
    //            }
    //        } else if (imageSupportRead(path)) {
    //            paths << path;
    //        }
    //    }

    //    if (paths.isEmpty()) {
    //        return;
    //    }

    //    // 判断当前导入路径是否为外接设备
    //    int isMountFlag = 0;
    //    DGioVolumeManager *pvfsManager = new DGioVolumeManager;
    //    QList<QExplicitlySharedDataPointer<DGioMount>> mounts = pvfsManager->getMounts();
    //    for (auto mount : mounts) {
    //        QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
    //        QString strPath = LocationFile->path();
    //        if (0 == paths.first().compare(strPath)) {
    //            isMountFlag = 1;
    //            break;
    //        }
    //    }

    //    // 当前导入路径
    //    if (isMountFlag) {
    //        QString strHomePath = QDir::homePath();
    //        //获取系统现在的时间
    //        QString strDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    //        QString basePath = QString("%1%2%3").arg(strHomePath, "/Pictures/照片/", strDate);
    //        QDir dir;
    //        if (!dir.exists(basePath)) {
    //            dir.mkpath(basePath);
    //        }

    //        QStringList newImagePaths;
    //        foreach (QString strPath, paths) {
    //            //取出文件名称
    //            QStringList pathList = strPath.split("/", QString::SkipEmptyParts);
    //            QStringList nameList = pathList.last().split(".", QString::SkipEmptyParts);
    //            QString strNewPath = QString("%1%2%3%4%5%6").arg(basePath, "/", nameList.first(), QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()), ".", nameList.last());

    //            newImagePaths << strNewPath;
    //            //判断新路径下是否存在目标文件，若存在，下一次张
    //            if (dir.exists(strNewPath)) {
    //                continue;
    //            }

    //            // 外接设备图片拷贝到系统
    //            if (QFile::copy(strPath, strNewPath)) {

    //            }
    //        }

    //        paths.clear();
    //        paths = newImagePaths;
    //    }

    //    DBImgInfoList dbInfos;

    //    using namespace utils::image;

    //    for (auto path : paths) {
    //        if (! imageSupportRead(path)) {
    //            continue;
    //        }

    ////        // Generate thumbnail and storage into cache dir
    ////        if (! utils::image::thumbnailExist(path)) {
    ////            // Generate thumbnail failed, do not insert into DB
    ////            if (! utils::image::generateThumbnail(path)) {
    ////                continue;
    ////            }
    ////        }

    //        QFileInfo fi(path);
    //        using namespace utils::image;
    //        using namespace utils::base;
    //        auto mds = getAllMetaData(path);
    //        QString value = mds.value("DateTimeOriginal");
    ////        qDebug() << value;
    //        DBImgInfo dbi;
    //        dbi.fileName = fi.fileName();
    //        dbi.filePath = path;
    //        dbi.dirHash = utils::base::hash(QString());
    //        if ("" != value) {
    //            dbi.time = QDateTime::fromString(value, "yyyy/MM/dd hh:mm:ss");
    //        } else if (fi.birthTime().isValid()) {
    //            dbi.time = fi.birthTime();
    //        } else if (fi.metadataChangeTime().isValid()) {
    //            dbi.time = fi.metadataChangeTime();
    //        } else {
    //            dbi.time = QDateTime::currentDateTime();
    //        }
    //        dbi.changeTime = QDateTime::currentDateTime();

    //        dbInfos << dbi;
    //    }

    //    if (! dbInfos.isEmpty()) {
    //        dApp->m_imageloader->ImportImageLoader(dbInfos, m_albumname);
    //    } else {
    //        emit dApp->signalM->ImportFailed();
       }
    */

    event->accept();
}

void ImportView::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void ImportView::dragLeaveEvent(QDragLeaveEvent *e)
{
    Q_UNUSED(e);
}

void ImportView::onImprotBtnClicked()
{
    qDebug() << "ImportView::onImprotBtnClicked()";
    static QStringList sList;

    for (const QByteArray &i : QImageReader::supportedImageFormats())
        sList << "*." + QString::fromLatin1(i);

    QString filter = tr("All Photos");

    filter.append('(');
    filter.append(sList.join(" "));
    filter.append(')');

    static QString cfgGroupName = QStringLiteral("General"), cfgLastOpenPath = QStringLiteral("LastOpenPath");
    QString pictureFolder = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QDir existChecker(pictureFolder);
    if (!existChecker.exists()) {
        pictureFolder = QDir::currentPath();
    }

    pictureFolder = dApp->setter->value(cfgGroupName, cfgLastOpenPath, pictureFolder).toString();
    DFileDialog dialog;
    dialog.setFileMode(DFileDialog::ExistingFiles);
//    dialog.setAllowMixedSelection(true);
    dialog.setDirectory(pictureFolder);
    dialog.setNameFilter(filter);
    dialog.setOption(QFileDialog::HideNameFilterDetails);
    dialog.setWindowTitle(tr("Import Photos"));
    dialog.setAllowMixedSelection(true);
    const int mode = dialog.exec();
    if (mode != QDialog::Accepted) {
        qDebug() << "mode != QDialog::Accepted";
        emit dApp->signalM->sigImportFailedToView();
        return;
    }

    const QStringList &file_list = dialog.selectedFiles();
    if (file_list.isEmpty()) {
        qDebug() << "file_list.isEmpty()";
        emit dApp->signalM->sigImportFailedToView();
        emit dApp->signalM->ImportFailed();
        return;
    }

    ////预留
    ImageEngineApi::instance()->SaveImagesCache(file_list);
    ImageEngineApi::instance()->ImportImagesFromFileList(file_list, m_albumname, this, true);

    /*    QStringList image_list;
    //    foreach (QString path, file_list) {
    //        QFileInfo file(path);
    //        if (file.isDir()) {
    //            qDebug() << "file.isDir()";
    //            image_list << utils::image::checkImage(path);
    //        } else {
    //            image_list << path;
    //        }
    //    }

    //    if (image_list.isEmpty()) {
    //        qDebug() << "image_list.isEmpty()";
    //        emit dApp->signalM->sigImportFailedToView();
    //        emit dApp->signalM->ImportFailed();
    //        return;
    //    }

    //    QFileInfo firstFileInfo(image_list.first());
    //    dApp->setter->setValue(cfgGroupName, cfgLastOpenPath, firstFileInfo.path());

    //    // 判断当前导入路径是否为外接设备
    //    int isMountFlag = 0;
    //    DGioVolumeManager *pvfsManager = new DGioVolumeManager;
    //    QList<QExplicitlySharedDataPointer<DGioMount>> mounts = pvfsManager->getMounts();
    //    for (auto mount : mounts) {
    //        QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
    //        QString strPath = LocationFile->path();
    //        if (0 == image_list.first().compare(strPath)) {
    //            isMountFlag = 1;
    //            break;
    //        }
    //    }

    //    // 当前导入路径为外接设备
    //    if (isMountFlag) {
    //        qDebug() << "isMountFlag";
    //        QString strHomePath = QDir::homePath();
    //        //获取系统现在的时间
    //        QString strDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    //        QString basePath = QString("%1%2%3").arg(strHomePath, "/Pictures/照片/", strDate);
    //        QDir dir;
    //        if (!dir.exists(basePath)) {
    //            dir.mkpath(basePath);
    //        }

    //        QStringList newImagePaths;
    //        foreach (QString strPath, image_list) {
    //            //取出文件名称
    //            QStringList pathList = strPath.split("/", QString::SkipEmptyParts);
    //            QStringList nameList = pathList.last().split(".", QString::SkipEmptyParts);
    //            QString strNewPath = QString("%1%2%3%4%5%6").arg(basePath, "/", nameList.first(), QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()), ".", nameList.last());

    //            newImagePaths << strNewPath;
    //            //判断新路径下是否存在目标文件，若存在，下一次张
    //            if (dir.exists(strNewPath)) {
    //                continue;
    //            }

    //            // 外接设备图片拷贝到系统
    //            if (QFile::copy(strPath, strNewPath)) {

    //            }
    //        }

    //        image_list.clear();
    //        image_list = newImagePaths;
    //    }

    //    m_dbInfos.clear();

    //    using namespace utils::image;

    //    for (auto imagePath : image_list) {
    //        if (!imageSupportRead(imagePath)) {
    //            qDebug() << "!imageSupportRead(imagePath)";
    //            continue;
    //        }

    ////        // Generate thumbnail and storage into cache dir
    ////        if (! utils::image::thumbnailExist(imagePath)) {
    ////            // Generate thumbnail failed, do not insert into DB
    ////            if (! utils::image::generateThumbnail(imagePath)) {
    ////                continue;
    ////            }
    ////        }

    //        QFileInfo fi(imagePath);
    //        using namespace utils::image;
    //        using namespace utils::base;
    //        auto mds = getAllMetaData(imagePath);
    //        QString value = mds.value("DateTimeOriginal");
    ////        qDebug() << value;
    //        DBImgInfo dbi;
    //        dbi.fileName = fi.fileName();
    //        dbi.filePath = imagePath;
    //        dbi.dirHash = utils::base::hash(QString());
    //        if ("" != value) {
    //            dbi.time = QDateTime::fromString(value, "yyyy/MM/dd hh:mm:ss");
    //        } else if (fi.birthTime().isValid()) {
    //            dbi.time = fi.birthTime();
    //        } else if (fi.metadataChangeTime().isValid()) {
    //            dbi.time = fi.metadataChangeTime();
    //        } else {
    //            dbi.time = QDateTime::currentDateTime();
    //        }
    //        dbi.changeTime = QDateTime::currentDateTime();

    //        m_dbInfos << dbi;
    //    }

    //    if (!m_dbInfos.isEmpty()) {
    //        qDebug() << "!m_dbInfos.isEmpty()";
    ////        dApp->m_imageloader->ImportImageLoader(dbInfos, m_albumname);
    //        ImportQThread *t = new ImportQThread(m_dbInfos, m_albumname);
    //        connect(t, &ImportQThread::finished, this, [ = ] {
    //            DBImgInfoList dbInfoList;
    //            QStringList pathlist;

    //            for (auto info : m_dbInfos)
    //            {
    //                if (dApp->m_imagemap.value(info.filePath).isNull()) {
    //                    qDebug() << "dApp->m_imagemap.value(info.filePath).isNull()";
    //                    continue;
    //                }
    //                pathlist << info.filePath;
    //                dbInfoList << info;
    //            }

    //            int count = 0;
    ////            if(dbInfoList.size() == m_dbInfos.size())
    ////            {
    //            qDebug() << "dbInfoList.size() == m_dbInfos.size()";
    //            count = 1;
    //            if (m_albumname.length() > 0)
    //            {
    //                if (COMMON_STR_RECENT_IMPORTED != m_albumname
    //                        && COMMON_STR_TRASH != m_albumname
    //                        && COMMON_STR_FAVORITES != m_albumname
    //                        && ALBUM_PATHTYPE_BY_PHONE != m_albumname
    //                        && 0 != m_albumname.compare(tr("Gallery"))) {
    //                    DBManager::instance()->insertIntoAlbumNoSignal(m_albumname, pathlist);
    //                }
    //            }

    //            DBManager::instance()->insertImgInfos(dbInfoList);
    //            if (pathlist.size() > 0)
    //            {
    //                emit dApp->signalM->updateStatusBarImportLabel(pathlist, count);
    //            } else
    //            {
    //                qDebug() << "dbInfoList.size() != m_dbInfos.size()";
    //                count = 0;
    //                emit dApp->signalM->ImportFailed();
    //                emit dApp->signalM->sigImportFailedToView();
    //            }
    ////        } else {
    ////            qDebug() << "dbInfoList.size() != m_dbInfos.size()";
    ////            count = 0;
    ////            emit dApp->signalM->ImportFailed();
    ////            emit dApp->signalM->sigImportFailedToView();
    ////        }
    //        });
    //        t->start();
    //    } else {
    //        qDebug() << "m_dbInfos.isEmpty()";
    //        emit dApp->signalM->sigImportFailedToView();
    //        emit dApp->signalM->ImportFailed();
    //    }
        */
}


bool ImportView::imageImported(bool success)
{
    emit dApp->signalM->closeWaitDialog();
    if (!success) {
        emit dApp->signalM->sigImportFailedToView();
    }

    return true;
}

void ImportView::setAlbumname(const QString &name)
{
    m_albumname = name;
}



