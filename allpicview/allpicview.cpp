#include "allpicview.h"
#include <QMimeData>
#include "utils/snifferimageformat.h"
#include "imageengine/imageengineapi.h"
#include <dgiovolumemanager.h>
#include <dgiofile.h>
#include <dgiofileinfo.h>
#include <dgiovolume.h>

namespace {
struct MetaData {
    QString key;
    const char *name;
};

static MetaData MetaDataBasics[] = {
    {"FileName",            QT_TRANSLATE_NOOP("MetadataName", "Photo name")},
    {"FileFormat",          QT_TRANSLATE_NOOP("MetadataName", "Type")},
    {"FileSize",            QT_TRANSLATE_NOOP("MetadataName", "File size")},
    {"Dimension",           QT_TRANSLATE_NOOP("MetadataName", "Dimensions")},
    {"DateTimeOriginal",    QT_TRANSLATE_NOOP("MetadataName", "Date captured")},
    {"DateTimeDigitized",   QT_TRANSLATE_NOOP("MetadataName", "Date modified")},
    {"Tag",                 QT_TRANSLATE_NOOP("MetadataName", "Tag")},
    {"", ""}
};
};

namespace {
const int VIEW_IMPORT = 0;
const int VIEW_ALLPICS = 1;
const int VIEW_SEARCH = 2;
const int VIEW_MAINWINDOW_ALLPIC = 0;
}  //namespace

AllPicView::AllPicView()
{
    setAcceptDrops(true);

    fatherwidget = new DWidget(this);
    fatherwidget->setFixedSize(this->size());
    m_pStackedWidget = new DStackedWidget(this);
    m_pImportView = new ImportView();
    m_pThumbnailListView = new ThumbnailListView(ThumbnailDelegate::AllPicViewType);
//    m_pThumbnailListView->setStyleSheet("background:red");
    DWidget *pThumbnailListView = new DWidget();
    QLayout *m_mainLayout = new QVBoxLayout();
    m_mainLayout->setContentsMargins(8, 0, 0, 0);
    m_mainLayout->addWidget(m_pThumbnailListView);
    pThumbnailListView->setLayout(m_mainLayout);
    m_pSearchView = new SearchView();
    m_pStackedWidget->addWidget(m_pImportView);
    m_pStackedWidget->addWidget(pThumbnailListView);
    m_pStackedWidget->addWidget(m_pSearchView);
    m_pStatusBar = new StatusBar(this);
    m_pStatusBar->raise();
    m_pStatusBar->setFixedWidth(this->width());
    m_pStatusBar->move(0, this->height() - m_pStatusBar->height());
//    m_pStatusBar->setParent(this);
    QVBoxLayout *pVBoxLayout = new QVBoxLayout();
    pVBoxLayout->setContentsMargins(2, 0, 0, 0);
    pVBoxLayout->addWidget(m_pStackedWidget);
//    pVBoxLayout->addWidget(m_pStatusBar);
    fatherwidget->setLayout(pVBoxLayout);
//    updateStackedWidget();
    initConnections();

    m_spinner = new DSpinner(this);
    m_spinner->setFixedSize(40, 40);
    m_spinner->hide();

//    if (0 < DBManager::instance()->getImgsCount())
//    {
//        m_spinner->show();
//        m_spinner->start();
//    }
    updatePicsIntoThumbnailView();

    m_pwidget = new QWidget(this);
    m_pwidget->setAttribute(Qt::WA_TransparentForMouseEvents);
}

void AllPicView::initConnections()
{
    qRegisterMetaType<DBImgInfoList>("DBImgInfoList &");
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &AllPicView::updatePicsIntoThumbnailView);
    connect(dApp->signalM, &SignalManager::imagesRemoved, this, &AllPicView::updatePicsIntoThumbnailView);
    connect(dApp, &Application::sigFinishLoad, this, [ = ] {
        m_pThumbnailListView->update();
    });

    connect(m_pThumbnailListView, &ThumbnailListView::openImage, this, [ = ](int index) {
        SignalManager::ViewInfo info;
        info.album = "";
        info.lastPanel = nullptr;
        auto imagelist = DBManager::instance()->getAllInfos();
        if (imagelist.size() > 1) {
            for (auto image : imagelist) {
                info.paths << image.filePath;
            }
        } else {
            info.paths.clear();
        }
        info.path = imagelist[index].filePath;
        info.viewType = utils::common::VIEW_ALLPIC_SRN;
        info.viewMainWindowID = VIEW_MAINWINDOW_ALLPIC;
        emit dApp->signalM->viewImage(info);
        emit dApp->signalM->showImageView(VIEW_MAINWINDOW_ALLPIC);
    });
    connect(m_pThumbnailListView, &ThumbnailListView::menuOpenImage, this, [ = ](QString path, QStringList paths, bool isFullScreen, bool isSlideShow) {
        SignalManager::ViewInfo info;
        info.album = "";
        info.lastPanel = nullptr;
//        auto imagelist1 = DBManager::instance()->getAllInfos();
        auto imagelist = DBManager::instance()->getAllPaths();
        if (paths.size() > 1) {
            info.paths = paths;
        } else {
            if (imagelist.size() > 1) {
//                for (auto image : imagelist) {
//                    info.paths << image.filePath;
//                }
                info.paths << imagelist;
            } else {
                info.paths.clear();
            }
        }
        info.path = path;
        info.fullScreen = isFullScreen;
        info.slideShow = isSlideShow;
        info.viewType = utils::common::VIEW_ALLPIC_SRN;
        info.viewMainWindowID = VIEW_MAINWINDOW_ALLPIC;
        if (info.slideShow) {
            if (imagelist.count() == 1) {
                info.paths = paths;
            }

            QStringList pathlist;
            pathlist.clear();
            for (auto path : info.paths) {
                if (QFileInfo(path).exists()) {
                    pathlist << path;
                }
            }

            info.paths = pathlist;
            emit dApp->signalM->startSlideShow(info);
            emit dApp->signalM->showSlidePanel(VIEW_MAINWINDOW_ALLPIC);
        } else {
            emit dApp->signalM->viewImage(info);
            emit dApp->signalM->showImageView(VIEW_MAINWINDOW_ALLPIC);
        }
    });
    connect(dApp->signalM, &SignalManager::sigUpdateImageLoader, this, &AllPicView::updatePicsIntoThumbnailView);
    connect(m_pStatusBar->m_pSlider, &DSlider::valueChanged, dApp->signalM, &SignalManager::sigMainwindowSliderValueChg);
    connect(m_pThumbnailListView, &ThumbnailListView::sigMouseRelease, this, &AllPicView::updatePicNum);
    connect(m_pThumbnailListView, &ThumbnailListView::customContextMenuRequested, this, &AllPicView::updatePicNum);
    connect(m_pSearchView->m_pThumbnailListView, &ThumbnailListView::sigMouseRelease, this, &AllPicView::updatePicNum);
    connect(m_pSearchView->m_pThumbnailListView, &ThumbnailListView::customContextMenuRequested, this, &AllPicView::updatePicNum);

    connect(m_pImportView->m_pImportBtn, &DPushButton::clicked, this, [ = ] {
//        m_spinner->show();
//        m_spinner->start();
        m_pStackedWidget->setCurrentIndex(VIEW_ALLPICS);
        emit dApp->signalM->startImprot();
        m_pImportView->onImprotBtnClicked();
    });
    connect(dApp->signalM, &SignalManager::sigImportFailedToView, this, [ = ] {
        if (isVisible())
        {
            m_spinner->hide();
            m_spinner->stop();
            updateStackedWidget();
        }
    });

    connect(m_pThumbnailListView, &ThumbnailListView::sigSelectAll, this, &AllPicView::updatePicNum);
    connect(dApp->signalM, &SignalManager::sigShortcutKeyDelete, this, &AllPicView::onKeyDelete);
}

void AllPicView::updateStackedWidget()
{
    if (0 < DBManager::instance()->getImgsCount()) {
        m_pStackedWidget->setCurrentIndex(VIEW_ALLPICS);
    } else {
        m_pStackedWidget->setCurrentIndex(VIEW_IMPORT);
    }
}

void AllPicView::updatePicsIntoThumbnailView()
{
    m_spinner->hide();
    m_spinner->stop();
    m_pThumbnailListView->stopLoadAndClear();
    m_pThumbnailListView->loadFilesFromDB();

//    using namespace utils::image;
//    QList<ThumbnailListView::ItemInfo> thumbnaiItemList;

//    auto infos = DBManager::instance()->getAllInfos();
//    for (auto info : infos) {
//        ThumbnailListView::ItemInfo vi;
//        vi.name = info.fileName;
//        vi.path = info.filePath;
////        vi.image = dApp->m_imagemap.value(info.filePath);
//        if (dApp->m_imagemap.value(info.filePath).isNull()) {
//            QSize imageSize = getImageQSize(vi.path);

//            vi.width = imageSize.width();
//            vi.height = imageSize.height();
//        } else {
//            vi.width = dApp->m_imagemap.value(info.filePath).width();
//            vi.height = dApp->m_imagemap.value(info.filePath).height();
//        }

//        thumbnaiItemList << vi;
//    }

//    m_pThumbnailListView->insertThumbnails(thumbnaiItemList);

    if (VIEW_SEARCH == m_pStackedWidget->currentIndex()) {
        //donothing
    } else {
        updateStackedWidget();
    }

    restorePicNum();
}

void AllPicView::dragEnterEvent(QDragEnterEvent *e)
{
    const QMimeData *mimeData = e->mimeData();
    if (!utils::base::checkMimeData(mimeData)) {
        return;
    }
    e->setDropAction(Qt::CopyAction);
    e->accept();
}

void AllPicView::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) {
        return;
    }
    ImageEngineApi::instance()->ImportImagesFromUrlList(urls, nullptr, this);

//    using namespace utils::image;
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
//        qDebug() << value;
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
//        dApp->m_imageloader->ImportImageLoader(dbInfos);
//    } else {
//        emit dApp->signalM->ImportFailed();
//    }

    event->accept();
}

void AllPicView::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void AllPicView::dragLeaveEvent(QDragLeaveEvent *e)
{

}

void AllPicView::resizeEvent(QResizeEvent *e)
{
    m_spinner->move(width() / 2 - 20, (height() - 50) / 2 - 20);
//    m_pwidget->setFixedWidth(this->width() / 2 + 150);
//    m_pwidget->setFixedHeight(443);
//    m_pwidget->move(this->width() / 4, this->height() - 443 - 23);
    m_pwidget->setFixedHeight(this->height() - 23);
    m_pwidget->setFixedWidth(this->width());
    m_pwidget->move(0, 0);
    m_pStatusBar->setFixedWidth(this->width());
    m_pStatusBar->move(0, this->height() - m_pStatusBar->height());
    fatherwidget->setFixedSize(this->size());
}

void AllPicView::updatePicNum()
{
    QString str = tr("%1 photo(s) selected");
    int selPicNum = 0;

    if (VIEW_ALLPICS == m_pStackedWidget->currentIndex()) {
        QStringList paths = m_pThumbnailListView->selectedPaths();
        selPicNum = paths.length();

    } else if (VIEW_SEARCH == m_pStackedWidget->currentIndex()) {
        QStringList paths = m_pSearchView->m_pThumbnailListView->selectedPaths();
        selPicNum = paths.length();
    }

    if (0 < selPicNum) {
        m_pStatusBar->m_pAllPicNumLabel->setText(str.arg(QString::number(selPicNum)));
    } else {
        restorePicNum();
    }
}

void AllPicView::restorePicNum()
{
    QString str = tr("%1 photo(s)");
    int selPicNum = 0;

    if (VIEW_ALLPICS == m_pStackedWidget->currentIndex()) {
        selPicNum = DBManager::instance()->getImgsCount();
    } else if (VIEW_SEARCH == m_pStackedWidget->currentIndex()) {
        selPicNum = m_pSearchView->m_searchPicNum;
    }

    m_pStatusBar->m_pAllPicNumLabel->setText(str.arg(QString::number(selPicNum)));
}

void AllPicView::onKeyDelete()
{
    if (!isVisible()) return;
    if (VIEW_SEARCH == m_pStackedWidget->currentIndex()) return;

    QStringList paths;
    paths.clear();

    paths = m_pThumbnailListView->selectedPaths();
    if (0 >= paths.length()) {
        return;
    }

    ImageEngineApi::instance()->moveImagesToTrash(paths);
//    DBImgInfoList infos;
//    for (auto path : paths) {
//        DBImgInfo info;
//        info = DBManager::instance()->getInfoByPath(path);
//        info.changeTime = QDateTime::currentDateTime();

//        QStringList allalbumnames = DBManager::instance()->getAllAlbumNames();
//        for (auto eachname : allalbumnames) {
//            if (DBManager::instance()->isImgExistInAlbum(eachname, path)) {
//                info.albumname += (eachname + ",");
//            }
//        }
//        infos << info;
//    }

////    dApp->m_imageloader->addTrashImageLoader(paths);
//    DBManager::instance()->insertTrashImgInfos(infos);
//    DBManager::instance()->removeImgInfos(paths);
}
