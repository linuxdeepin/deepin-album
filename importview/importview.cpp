#include "importview.h"
#include <DApplicationHelper>
#include <DFileDialog>
#include <QGraphicsDropShadowEffect>
#include <DSuggestButton>
#include <dgiovolumemanager.h>
#include <dgiofile.h>
#include <dgiofileinfo.h>
#include <dgiovolume.h>

ImportView::ImportView()
{
    setAcceptDrops(true);

    initUI();
    initConnections();
}

void ImportView::initConnections()
{
//    connect(m_pImportBtn, &DPushButton::clicked, this, &ImportView::onImprotBtnClicked);
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, pLabel,[=]{
        QPixmap pixmap;
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType)
        {
            pixmap = utils::base::renderSVG(":/resources/images/other/icon_import_photo.svg", QSize(128, 128));
        }
        if (themeType == DGuiApplicationHelper::DarkType)
        {
            pixmap = utils::base::renderSVG(":/resources/images/other/icon_import_photo_dark.svg", QSize(128, 128));
        }
        pLabel->setPixmap(pixmap);
    });
}

void ImportView::initUI()
{
    QVBoxLayout* pImportFrameLayout = new QVBoxLayout();

    pLabel = new DLabel();
    pLabel->setFixedSize(128, 128);

    QPixmap pixmap;
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType)
    {
        pixmap = utils::base::renderSVG(":/resources/images/other/icon_import_photo.svg", QSize(128, 128));
    }
    if (themeType == DGuiApplicationHelper::DarkType)
    {
        pixmap = utils::base::renderSVG(":/resources/images/other/icon_import_photo_dark.svg", QSize(128, 128));
    }
    pLabel->setPixmap(pixmap);

    m_pImportBtn = new DSuggestButton();
//    m_pImportBtn->setFocusPolicy(Qt::NoFocus);
    m_pImportBtn->setText(tr("Import Photos"));
    m_pImportBtn->setFixedSize(302, 36);
    m_pImportBtn->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));

    DPalette pa = DApplicationHelper::instance()->palette(m_pImportBtn);
//    pa.setColor(QPalette::Light,QColor(37,183,255));
//    pa.setColor(QPalette::Dark,QColor(0,152,255));
    pa.setColor(QPalette::Highlight,QColor(0,0,0,0));
//    pa.setBrush(DPalette::ButtonText, pa.color(DPalette::Base));
    m_pImportBtn->setPalette(pa);

    DLabel* pLabel2 = new DLabel();
    DFontSizeManager::instance()->bind(pLabel2, DFontSizeManager::T8, QFont::Normal);
    pLabel2->setForegroundRole(DPalette::TextTips);
    pLabel2->setFixedHeight(18);
    pLabel2->setText(tr("Or drag photos here"));

//    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
//    effect->setOffset(0,4);
//    effect->setColor(QColor(0,145,255,77));
//    effect->setBlurRadius(4);
//    m_pImportBtn->setGraphicsEffect(effect);

    pImportFrameLayout->addStretch();
    pImportFrameLayout->addWidget(pLabel, 0, Qt::AlignCenter);
    pImportFrameLayout->addSpacing(10);
    pImportFrameLayout->addWidget(m_pImportBtn, 0, Qt::AlignCenter);
    pImportFrameLayout->addSpacing(20);
    pImportFrameLayout->addWidget(pLabel2, 0, Qt::AlignCenter);
    pImportFrameLayout->addStretch();

    setLayout(pImportFrameLayout);
}

void ImportView::dragEnterEvent(QDragEnterEvent *e)
{
    e->setDropAction(Qt::CopyAction);
    e->accept();
}

void ImportView::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) {
        return;
    }

    using namespace utils::image;
    QStringList paths;
    for (QUrl url : urls) {
        const QString path = url.toLocalFile();
        if (QFileInfo(path).isDir()) {
            auto finfos =  getImagesInfo(path, false);
            for (auto finfo : finfos) {
                if (imageSupportRead(finfo.absoluteFilePath())) {
                    paths << finfo.absoluteFilePath();
                }
            }
        } else if (imageSupportRead(path)) {
            paths << path;
        }
    }

    if (paths.isEmpty())
    {
        return;
    }

    // 判断当前导入路径是否为外接设备
    int isMountFlag = 0;
    DGioVolumeManager *pvfsManager = new DGioVolumeManager;
    QList<QExplicitlySharedDataPointer<DGioMount>> mounts = pvfsManager->getMounts();
    for(auto mount : mounts)
    {
        QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
        QString strPath = LocationFile->path();
        if (0 == paths.first().compare(strPath))
        {
            isMountFlag = 1;
            break;
        }
    }

    // 当前导入路径
    if(isMountFlag)
    {
        QString strHomePath = QDir::homePath();
        //获取系统现在的时间
        QString strDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
        QString basePath = QString("%1%2%3").arg(strHomePath, "/Pictures/照片/", strDate);
        QDir dir;
        if (!dir.exists(basePath))
        {
            dir.mkpath(basePath);
        }

        QStringList newImagePaths;
        foreach (QString strPath, paths)
        {
            //取出文件名称
            QStringList pathList = strPath.split("/", QString::SkipEmptyParts);
            QStringList nameList = pathList.last().split(".", QString::SkipEmptyParts);
            QString strNewPath = QString("%1%2%3%4%5%6").arg(basePath, "/", nameList.first(), QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()), ".", nameList.last());

            newImagePaths << strNewPath;
            //判断新路径下是否存在目标文件，若存在，下一次张
            if (dir.exists(strNewPath))
            {
                continue;
            }

            // 外接设备图片拷贝到系统
            if (QFile::copy(strPath, strNewPath))
            {

            }
        }

        paths.clear();
        paths = newImagePaths;
    }

    DBImgInfoList dbInfos;

    using namespace utils::image;

    for (auto path : paths)
    {
        if (! imageSupportRead(path)) {
            continue;
        }

//        // Generate thumbnail and storage into cache dir
//        if (! utils::image::thumbnailExist(path)) {
//            // Generate thumbnail failed, do not insert into DB
//            if (! utils::image::generateThumbnail(path)) {
//                continue;
//            }
//        }

        QFileInfo fi(path);
        DBImgInfo dbi;
        dbi.fileName = fi.fileName();
        dbi.filePath = path;
        dbi.dirHash = utils::base::hash(QString());
        if(fi.birthTime().isValid())
        {
            dbi.time = fi.birthTime();
        }
        else if (fi.metadataChangeTime().isValid())
        {
            dbi.time = fi.metadataChangeTime();
        }
        else
        {
            dbi.time = QDateTime::currentDateTime();
        }
        dbi.changeTime = QDateTime::currentDateTime();

        dbInfos << dbi;
    }

    if (! dbInfos.isEmpty())
    {
        dApp->m_imageloader->ImportImageLoader(dbInfos, m_albumname);
    }
    else
    {
        emit dApp->signalM->ImportFailed();
    }

    event->accept();
}

void ImportView::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void ImportView::dragLeaveEvent(QDragLeaveEvent *e)
{

}

void ImportView::onImprotBtnClicked()
{
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
    dialog.setWindowTitle(tr("Open photos"));
    dialog.setAllowMixedSelection(true);
    const int mode = dialog.exec();
    if (mode != QDialog::Accepted)
    {
        emit importFailedToView();
        return;
    }

    const QStringList &file_list = dialog.selectedFiles();
    if (file_list.isEmpty())
        return;

    QStringList image_list;
    foreach(QString path, file_list)
    {
        QFileInfo file(path);
        if(file.isDir())
        {
            image_list<<utils::image::checkImage(path);
        }
        else {
            image_list<<path;
        }
    }

    if (image_list.isEmpty())
        return;

    QFileInfo firstFileInfo(image_list.first());
    dApp->setter->setValue(cfgGroupName, cfgLastOpenPath, firstFileInfo.path());

    // 判断当前导入路径是否为外接设备
    int isMountFlag = 0;
    DGioVolumeManager *pvfsManager = new DGioVolumeManager;
    QList<QExplicitlySharedDataPointer<DGioMount>> mounts = pvfsManager->getMounts();
    for(auto mount : mounts)
    {
        QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
        QString strPath = LocationFile->path();
        if (0 == image_list.first().compare(strPath))
        {
            isMountFlag = 1;
            break;
        }
    }

    // 当前导入路径
    if(isMountFlag)
    {
        QString strHomePath = QDir::homePath();
        //获取系统现在的时间
        QString strDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
        QString basePath = QString("%1%2%3").arg(strHomePath, "/Pictures/照片/", strDate);
        QDir dir;
        if (!dir.exists(basePath))
        {
            dir.mkpath(basePath);
        }

        QStringList newImagePaths;
        foreach (QString strPath, image_list)
        {
            //取出文件名称
            QStringList pathList = strPath.split("/", QString::SkipEmptyParts);
            QStringList nameList = pathList.last().split(".", QString::SkipEmptyParts);
            QString strNewPath = QString("%1%2%3%4%5%6").arg(basePath, "/", nameList.first(), QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()), ".", nameList.last());

            newImagePaths << strNewPath;
            //判断新路径下是否存在目标文件，若存在，下一次张
            if (dir.exists(strNewPath))
            {
                continue;
            }

            // 外接设备图片拷贝到系统
            if (QFile::copy(strPath, strNewPath))
            {

            }
        }

        image_list.clear();
        image_list = newImagePaths;
    }


    DBImgInfoList dbInfos;

    using namespace utils::image;

    for (auto imagePath : image_list)
    {
        if (! imageSupportRead(imagePath)) {
            continue;
        }

//        // Generate thumbnail and storage into cache dir
//        if (! utils::image::thumbnailExist(imagePath)) {
//            // Generate thumbnail failed, do not insert into DB
//            if (! utils::image::generateThumbnail(imagePath)) {
//                continue;
//            }
//        }

        QFileInfo fi(imagePath);
        DBImgInfo dbi;
        dbi.fileName = fi.fileName();
        dbi.filePath = imagePath;
        dbi.dirHash = utils::base::hash(QString());
        if(fi.birthTime().isValid())
        {
            dbi.time = fi.birthTime();
        }
        else if (fi.metadataChangeTime().isValid())
        {
            dbi.time = fi.metadataChangeTime();
        }
        else
        {
            dbi.time = QDateTime::currentDateTime();
        }
        dbi.changeTime = QDateTime::currentDateTime();

        dbInfos << dbi;
    }

    if (! dbInfos.isEmpty())
    {
        dApp->m_imageloader->ImportImageLoader(dbInfos, m_albumname);
    }
    else
    {
        emit importFailedToView();
        emit dApp->signalM->ImportFailed();
    }
}

void ImportView::setAlbumname(const QString &name)
{
    m_albumname = name;
}
