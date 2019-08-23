#include "allpicview.h"

AllPicView::AllPicView()
{
    m_allPicNum = 0;

    m_pEmptyLayout = new QVBoxLayout();
    m_pNotEmptyLayout = new QVBoxLayout();

    initUI();
    initConnections();
}

void AllPicView::initUI()
{
    if (0 == m_allPicNum)
    {
        updateEmptyLayout();
    }
    else
    {
        updateNotEmptyLayout();
    }
}

void AllPicView::initConnections()
{
    connect(m_pImportBtn, &DPushButton::clicked, this, &AllPicView::improtBtnClicked);
}

void AllPicView::updateEmptyLayout()
{
    QLabel* pLabel = new QLabel();
    pLabel->setFixedSize(128, 128);

    QPixmap pixmap;
    pixmap = utils::base::renderSVG(":/resources/images/logo/deepin-image-viewer.svg", QSize(128, 128));

    pLabel->setPixmap(pixmap);

    m_pImportBtn = new DPushButton();
    m_pImportBtn->setText("导入图片");
    m_pImportBtn->setFixedSize(142, 42);

    QLabel* pLabel2 = new QLabel();
    pLabel2->setFixedHeight(24);
    pLabel2->setText("您也可以拖拽或导入图片到时间线");

    m_pEmptyLayout->addStretch();
    m_pEmptyLayout->addWidget(pLabel, 0, Qt::AlignCenter);
    m_pEmptyLayout->addSpacing(20);
    m_pEmptyLayout->addWidget(m_pImportBtn, 0, Qt::AlignCenter);
    m_pEmptyLayout->addSpacing(10);
    m_pEmptyLayout->addWidget(pLabel2, 0, Qt::AlignCenter);
    m_pEmptyLayout->addStretch();

    this->setLayout(m_pEmptyLayout);
}

void AllPicView::updateNotEmptyLayout()
{

}

void AllPicView::improtBtnClicked()
{
    static QStringList sList;

    for (const QByteArray &i : QImageReader::supportedImageFormats())
        sList << "*." + QString::fromLatin1(i);

    QString filter = tr("All images");

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

    const QStringList &image_list = QFileDialog::getOpenFileNames(this, tr("Open Image"),
                                                                  pictureFolder, filter, nullptr, QFileDialog::HideNameFilterDetails);

    if (image_list.isEmpty())
        return;

    QFileInfo firstFileInfo(image_list.first());
    dApp->setter->setValue(cfgGroupName, cfgLastOpenPath, firstFileInfo.path());
}
