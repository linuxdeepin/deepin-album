#include "importview.h"

namespace {
const QString RECENT_IMPORTED_ALBUM = "Recent imported";
const QString TRASH_ALBUM = "Trash";
const QString FAVORITES_ALBUM = "My favorite";
}  //namespace

ImportView::ImportView()
{
    initUI();
    initConnections();
}

void ImportView::initConnections()
{

}

void ImportView::initUI()
{
    QVBoxLayout* pImportFrameLayout = new QVBoxLayout();

    DLabel* pLabel = new DLabel();
    pLabel->setFixedSize(128, 128);

    QPixmap pixmap;
    pixmap = utils::base::renderSVG(":/resources/images/other/icon_import_photo.svg", QSize(128, 128));

    pLabel->setPixmap(pixmap);

    m_pImportBtn = new DPushButton();
    m_pImportBtn->setText("导入图片");
    m_pImportBtn->setFixedSize(142, 42);

    DLabel* pLabel2 = new DLabel();
    pLabel2->setFixedHeight(24);
    pLabel2->setText("您也可以拖拽或导入图片到时间线");

    pImportFrameLayout->addStretch();
    pImportFrameLayout->addWidget(pLabel, 0, Qt::AlignCenter);
    pImportFrameLayout->addSpacing(20);
    pImportFrameLayout->addWidget(m_pImportBtn, 0, Qt::AlignCenter);
    pImportFrameLayout->addSpacing(10);
    pImportFrameLayout->addWidget(pLabel2, 0, Qt::AlignCenter);
    pImportFrameLayout->addStretch();

    setLayout(pImportFrameLayout);
}
