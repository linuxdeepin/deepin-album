#include "widgets/albumlefttabitem.h"
#include "dbmanager/dbmanager.h"
#include "utils/baseutils.h"
#include "application.h"
#include <QHBoxLayout>
#include "controller/signalmanager.h"
#include <DFontSizeManager>
#include <QPainter>
namespace {
const int LAYOUT_SPACING = 10;
const int OPE_MODE_ADDNEWALBUM = 0;
const int OPE_MODE_RENAMEALBUM = 1;
const int OPE_MODE_ADDRENAMEALBUM = 2;
}// namespace

using namespace utils::common;

AlbumLeftTabItem::AlbumLeftTabItem(QString str, QString strAlbumType)
{
    m_albumNameStr = str;
    m_opeMode = 0;
    m_albumTypeStr = strAlbumType;
    m_mountPath = "";

    initUI();
    initConnections();
}

AlbumLeftTabItem::~AlbumLeftTabItem()
{

}

void AlbumLeftTabItem::initConnections()
{
    connect(m_pLineEdit, &DLineEdit::editingFinished, this, &AlbumLeftTabItem::onCheckNameValid);
    connect(m_unMountBtn, &MountExternalBtn::sigMountExternalBtnClicked, this, &AlbumLeftTabItem::unMountBtnClicked);
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, m_nameLabel, [ = ] {
        oriAlbumStatus();
    });
}

void AlbumLeftTabItem::initUI()
{
    setFocusPolicy(Qt::NoFocus);
    setFixedSize(160, 40);
    QHBoxLayout *pHBoxLayout = new QHBoxLayout();
    pHBoxLayout->setContentsMargins(10, 0, 0, 0);
    pHBoxLayout->setSpacing(0);

    pImageLabel = new DLabel();
    pImageLabel->setFixedSize(18, 18);


    if (COMMON_STR_RECENT_IMPORTED == m_albumNameStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_import_normal.svg", QSize(20, 20));
        pImageLabel->setPixmap(pixmap);
    } else if (COMMON_STR_TRASH == m_albumNameStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_trash_normal.svg", QSize(20, 20));
        pImageLabel->setPixmap(pixmap);
    } else if (COMMON_STR_FAVORITES == m_albumNameStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_collection_normal.svg", QSize(20, 20));
        pImageLabel->setPixmap(pixmap);
    } else if (ALBUM_PATHTYPE_BY_PHONE == m_albumTypeStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_iphone_normal.svg", QSize(20, 20));
        pImageLabel->setPixmap(pixmap);
    } else if (ALBUM_PATHTYPE_BY_U == m_albumTypeStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_usb_normal.svg", QSize(20, 20));
        pImageLabel->setPixmap(pixmap);
    } else {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_album_normal.svg", QSize(20, 20));
        pImageLabel->setPixmap(pixmap);
    }

    DWidget *pWidget = new DWidget();

//    QWidget *widget = m_pListWidget->itemDelegate()->createEditor(m_pListWidget, m_pListWidget->viewOptions(), m_pListWidget->getModelIndex(m_pListWidgetItem));
//    m_pLineEdit = dynamic_cast<QLineEdit*>(widget);
//    m_pLineEdit = dynamic_cast<DLineEdit*>(widget);

    m_pLineEdit = new DLineEdit();

    if (!m_pLineEdit) {
        qDebug() << "Create LineEdit Error!!!";
        return;
    }

    m_nameLabel = new DDlabel(pWidget);
    m_nameLabel->setGeometry(QRect(16, 0, 118, 40));

    QFontMetrics elideFont(m_nameLabel->font());
    if (COMMON_STR_RECENT_IMPORTED == m_albumNameStr) {
//        m_nameLabel->Settext(elideFont.elidedText(tr("Import"), Qt::ElideRight, 85));
        DFontSizeManager::instance()->bind(m_nameLabel, DFontSizeManager::T6, Qt::ElideRight);
        m_nameLabel->Settext(tr("Import"));
    } else if (COMMON_STR_TRASH == m_albumNameStr) {
//        m_nameLabel->Settext(elideFont.elidedText(tr("Trash"), Qt::ElideRight, 85));
        DFontSizeManager::instance()->bind(m_nameLabel, DFontSizeManager::T6, Qt::ElideRight);
        m_nameLabel->Settext(tr("Trash"));
    } else if (COMMON_STR_FAVORITES == m_albumNameStr) {
//        m_nameLabel->Settext(elideFont.elidedText(tr("Favorites"), Qt::ElideRight, 85));
        DFontSizeManager::instance()->bind(m_nameLabel, DFontSizeManager::T6, Qt::ElideRight);
        m_nameLabel->Settext(tr("Favorites"));
    } else {
        DFontSizeManager::instance()->bind(m_nameLabel, DFontSizeManager::T6, Qt::ElideRight);
//        m_nameLabel->Settext(elideFont.elidedText(m_albumNameStr, Qt::ElideRight, 85));
        m_nameLabel->Settext(m_albumNameStr);
        //        m_nameLabel->setText(m_albumNameStr);
    }
    m_nameLabel->setAlignment(Qt::AlignVCenter);

    QFont ft = DFontSizeManager::instance()->get(DFontSizeManager::T6);
    ft.setFamily("SourceHanSansSC");
    ft.setWeight(QFont::Medium);
    m_nameLabel->setFont(ft);

//    DPalette pa = DApplicationHelper::instance()->palette(m_nameLabel);
//    pa.setBrush(DPalette::Text, pa.color(DPalette::ToolTipText));
    m_nameLabel->setForegroundRole(DPalette::TextTitle);
//    m_nameLabel->setPalette(pa);

    m_pLineEdit->setParent(pWidget);
    m_pLineEdit->setGeometry(QRect(0, 0, 120, 40));
    if (COMMON_STR_RECENT_IMPORTED == m_albumNameStr) {

        m_pLineEdit->setText(tr("Import"));
    } else if (COMMON_STR_TRASH == m_albumNameStr) {

        m_pLineEdit->setText(tr("Trash"));
    } else if (COMMON_STR_FAVORITES == m_albumNameStr) {

        m_pLineEdit->setText(tr("Favorites"));
    } else {
        m_pLineEdit->setText(m_albumNameStr);
    }

//    m_pLineEdit->setTextMargins(5,0,0,0);
    m_pLineEdit->lineEdit()->setTextMargins(5, 0, 0, 0);
//    m_pLineEdit->setAlignment(Qt::AlignVCenter| Qt::AlignLeft);
    m_pLineEdit->lineEdit()->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

//    m_pLineEdit->setStyleSheet(QString::fromUtf8("selection-background-color: rgb(0,129,255);"));
//    m_pLineEdit->setStyleSheet("border-radius:8px;"
//                               "background: rgba(255,255,255,0.00);"
//                               "border: 2px solid #0081FF;"
//                               "box-shadow: 0 2px 4px 0 rgba(0,0,0,0.05);"
//                               );

    m_pLineEdit->setVisible(false);
//    m_pLineEdit->setMaxLength(64);
    m_pLineEdit->lineEdit()->setMaxLength(64);

    m_pLineEdit->setClearButtonEnabled(false);

    pHBoxLayout->addWidget(pImageLabel, Qt::AlignVCenter);
    pHBoxLayout->addWidget(pWidget, Qt::AlignVCenter);

    m_unMountBtn = new MountExternalBtn(m_nameLabel);
    //外部设备插入，需要添加卸载按钮
    if (ALBUM_PATHTYPE_BY_PHONE == m_albumTypeStr || ALBUM_PATHTYPE_BY_U == m_albumTypeStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_exit_normal.svg", QSize(24, 24));
        m_unMountBtn->setPixmap(pixmap);
        m_unMountBtn->move(92, 9);
    }

    this->setLayout(pHBoxLayout);
}

void AlbumLeftTabItem::unMountBtnClicked()
{
    emit unMountExternalDevices(m_mountPath);
}

void AlbumLeftTabItem::editAlbumEdit()
{
    m_nameLabel->setVisible(false);
    m_pLineEdit->setVisible(true);
//    m_pLineEdit->selectAll();
    m_pLineEdit->lineEdit()->selectAll();
//    m_pLineEdit->setFocus();
    m_pLineEdit->lineEdit()->setFocus();
}

//设置外部设备挂载的path，在相册中卸载外部设备时用（如果有两个外部设置挂载点name一样，就不能使用name做卸载判断，使用path没问题）
void AlbumLeftTabItem::setExternalDevicesMountPath(QString strPath)
{
    m_mountPath = strPath;
}

QString AlbumLeftTabItem::getalbumname()
{
    return m_nameLabel->text();
}

void AlbumLeftTabItem::onCheckNameValid()
{
//    QString newNameStr = m_pLineEdit->text().trimmed();
    QString newNameStr = m_pLineEdit->lineEdit()->text().trimmed();

    if (DBManager::instance()->getAllAlbumNames().contains(newNameStr)
            || COMMON_STR_RECENT_IMPORTED == newNameStr
            || COMMON_STR_TRASH == newNameStr
            || COMMON_STR_FAVORITES == newNameStr
            || newNameStr.isEmpty()) {
        newNameStr = m_albumNameStr;
    }

    if (OPE_MODE_RENAMEALBUM == m_opeMode || OPE_MODE_ADDRENAMEALBUM == m_opeMode) {
//        m_nameLabel->setText(newNameStr);
        QFontMetrics elideFont(m_nameLabel->font());
        m_nameLabel->Settext(elideFont.elidedText(newNameStr, Qt::ElideRight, 85));

        QFont ft;
        ft.setPixelSize(14);

        m_pLineEdit->setText(newNameStr);
        m_nameLabel->setVisible(true);
        m_pLineEdit->setVisible(false);

        DBManager::instance()->renameAlbum(m_albumNameStr, newNameStr);

        m_albumNameStr = newNameStr;
        emit dApp->signalM->sigUpdataAlbumRightTitle(m_albumNameStr);
    } else if (OPE_MODE_ADDNEWALBUM == m_opeMode) {
//        m_nameLabel->setText(newNameStr);
        QFontMetrics elideFont(m_nameLabel->font());
        m_nameLabel->Settext(elideFont.elidedText(newNameStr, Qt::ElideRight, 85));
        QFont ft;
        ft.setPixelSize(14);

        m_pLineEdit->setText(newNameStr);

        m_nameLabel->setVisible(true);
        m_pLineEdit->setVisible(false);

        DBManager::instance()->insertIntoAlbum(newNameStr, QStringList(" "));

        m_albumNameStr = newNameStr;
        emit dApp->signalM->sigUpdataAlbumRightTitle(m_albumNameStr);
    }
}

void AlbumLeftTabItem::oriAlbumStatus()
{
    if (COMMON_STR_RECENT_IMPORTED == m_albumNameStr) {
        QPixmap pixmap;
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_import_normal.svg", QSize(20, 20));
        }
        if (themeType == DGuiApplicationHelper::DarkType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_import_normal_dark.svg", QSize(20, 20));
        }
        pImageLabel->setPixmap(pixmap);

    } else if (COMMON_STR_TRASH == m_albumNameStr) {
        QPixmap pixmap;
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_trash_normal.svg", QSize(20, 20));
        }
        if (themeType == DGuiApplicationHelper::DarkType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_trash_normal_dark.svg", QSize(20, 20));
        }
        pImageLabel->setPixmap(pixmap);

    } else if (COMMON_STR_FAVORITES == m_albumNameStr) {
        QPixmap pixmap;
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_collection_normal.svg", QSize(20, 20));
        }
        if (themeType == DGuiApplicationHelper::DarkType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_collection_normal_dark.svg", QSize(20, 20));
        }
        pImageLabel->setPixmap(pixmap);

    } else if (ALBUM_PATHTYPE_BY_PHONE == m_albumTypeStr) {
        QPixmap pixmap;
        QPixmap mountpixmap;

        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_iphone_normal.svg", QSize(20, 20));
            mountpixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_exit_normal.svg", QSize(20, 20));
        }
        if (themeType == DGuiApplicationHelper::DarkType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_iphone_normal_dark.svg", QSize(20, 20));
            mountpixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_exit_normal_dark.svg", QSize(20, 20));
        }

        pImageLabel->setPixmap(pixmap);
        m_unMountBtn->setPixmap(mountpixmap);
    } else if (ALBUM_PATHTYPE_BY_U == m_albumTypeStr) {
        QPixmap pixmap;
        QPixmap mountpixmap;

        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_usb_normal.svg", QSize(20, 20));
            mountpixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_exit_normal.svg", QSize(20, 20));
        }
        if (themeType == DGuiApplicationHelper::DarkType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_usb_normal_dark.svg", QSize(20, 20));
            mountpixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_exit_normal_dark.svg", QSize(20, 20));
        }

        pImageLabel->setPixmap(pixmap);
        m_unMountBtn->setPixmap(mountpixmap);
    } else {
        QPixmap pixmap;
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_album_normal.svg", QSize(20, 20));
        }
        if (themeType == DGuiApplicationHelper::DarkType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_album_normal_dark.svg", QSize(20, 20));
        }
        pImageLabel->setPixmap(pixmap);

    }

    m_nameLabel->setForegroundRole(DPalette::TextTitle);
}

void AlbumLeftTabItem::newAlbumStatus()
{
    if (COMMON_STR_RECENT_IMPORTED == m_albumNameStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/active/icon_import_active.svg", QSize(20, 20));
        pImageLabel->setPixmap(pixmap);
    } else if (COMMON_STR_TRASH == m_albumNameStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/active/icon_trash_active.svg", QSize(20, 20));
        pImageLabel->setPixmap(pixmap);
    } else if (COMMON_STR_FAVORITES == m_albumNameStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/active/icon_collection_active.svg", QSize(20, 20));
        pImageLabel->setPixmap(pixmap);
    } else if (ALBUM_PATHTYPE_BY_PHONE == m_albumTypeStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/active/icon_iphone_active.svg", QSize(20, 20));
        pImageLabel->setPixmap(pixmap);

        QPixmap mountpixmap;
        mountpixmap = utils::base::renderSVG(":/resources/images/sidebar/active/icon_exit_active.svg", QSize(20, 20));
        m_unMountBtn->setPixmap(mountpixmap);
    } else if (ALBUM_PATHTYPE_BY_U == m_albumTypeStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/active/icon_usb_active.svg", QSize(20, 20));
        pImageLabel->setPixmap(pixmap);

        QPixmap mountpixmap;
        mountpixmap = utils::base::renderSVG(":/resources/images/sidebar/active/icon_exit_active.svg", QSize(20, 20));
        m_unMountBtn->setPixmap(mountpixmap);
    } else {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/active/icon_album_active.svg", QSize(20, 20));
        pImageLabel->setPixmap(pixmap);
    }

//    DPalette pa = DApplicationHelper::instance()->palette(m_nameLabel);
//    pa.setBrush(DPalette::Text, pa.color(DPalette::HighlightedText));
    m_nameLabel->setForegroundRole(DPalette::HighlightedText);
//    m_nameLabel->setPalette(pa);

}

