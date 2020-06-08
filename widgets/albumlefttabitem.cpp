#include "widgets/albumlefttabitem.h"
#include "dbmanager/dbmanager.h"
#include "utils/baseutils.h"
#include "../dialogs/albumcreatedialog.h"
#include "application.h"
#include <QHBoxLayout>
#include "controller/signalmanager.h"
#include <DFontSizeManager>
#include <QPainter>
namespace {
const int OPE_MODE_ADDNEWALBUM = 0;
const int OPE_MODE_RENAMEALBUM = 1;
const int OPE_MODE_ADDRENAMEALBUM = 2;
}// namespace

using namespace utils::common;

AlbumLeftTabItem::AlbumLeftTabItem(QString str, QString strAlbumType)
    : m_albumNameStr(str), m_albumTypeStr(strAlbumType), m_opeMode(0)
    , m_pLineEdit(nullptr), m_pNewLineEdit(nullptr), m_nameLabel(nullptr)
    , pImageLabel(nullptr), m_unMountBtn(nullptr), m_pListWidget(nullptr)
    , m_pListWidgetItem(nullptr)
{
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
    pHBoxLayout->setContentsMargins(15, 0, 0, 0);
    pHBoxLayout->setSpacing(0);

    pImageLabel = new DLabel();
    pImageLabel->setFixedSize(24, 24);
    if (ALBUM_PATHTYPE_BY_PHONE == m_albumTypeStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_iphone_normal.svg", QSize(24, 24));
        pImageLabel->setPixmap(pixmap);
    } else if (ALBUM_PATHTYPE_BY_U == m_albumTypeStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_usb_normal.svg", QSize(24, 24));
        pImageLabel->setPixmap(pixmap);
    } else if (COMMON_STR_CUSTOM == m_albumTypeStr || COMMON_STR_CREATEALBUM == m_albumTypeStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_album_normal.svg", QSize(24, 24));
        pImageLabel->setPixmap(pixmap);
    } else if (COMMON_STR_RECENT_IMPORTED == m_albumNameStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_import_normal.svg", QSize(24, 24));
        pImageLabel->setPixmap(pixmap);
    } else if (COMMON_STR_TRASH == m_albumNameStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_trash_normal.svg", QSize(24, 24));
        pImageLabel->setPixmap(pixmap);
    } else if (COMMON_STR_FAVORITES == m_albumNameStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_collection_normal.svg", QSize(24, 24));
        pImageLabel->setPixmap(pixmap);
    }  else {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_album_normal.svg", QSize(24, 24));
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
        DFontSizeManager::instance()->bind(m_nameLabel, DFontSizeManager::T6, Qt::ElideRight);
        m_nameLabel->Settext(tr("Import"));
    } else if (COMMON_STR_TRASH == m_albumNameStr) {
        DFontSizeManager::instance()->bind(m_nameLabel, DFontSizeManager::T6, Qt::ElideRight);
        m_nameLabel->Settext(tr("Trash"));
    } else if (COMMON_STR_FAVORITES == m_albumNameStr) {
        DFontSizeManager::instance()->bind(m_nameLabel, DFontSizeManager::T6, Qt::ElideRight);
        m_nameLabel->Settext(tr("Favorites"));
    } else {
        DFontSizeManager::instance()->bind(m_nameLabel, DFontSizeManager::T6, Qt::ElideRight);
        m_nameLabel->Settext(m_albumNameStr);
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
        pHBoxLayout->addWidget(m_unMountBtn);
        //m_unMountBtn->move(92, 9);
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
    if (newNameStr.isEmpty()) {
        newNameStr = m_nameLabel->text();
    }
    if (OPE_MODE_RENAMEALBUM == m_opeMode || OPE_MODE_ADDRENAMEALBUM == m_opeMode) {
//        m_nameLabel->setText(newNameStr);
        if (newNameStr != m_nameLabel->text() && DBManager::instance()->isAlbumExistInDB(newNameStr)) {
            newNameStr = AlbumCreateDialog::getNewAlbumName(newNameStr, true, m_nameLabel->text());
        }
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
        if (newNameStr != m_nameLabel->text() && DBManager::instance()->isAlbumExistInDB(newNameStr)) {
            newNameStr = AlbumCreateDialog::getNewAlbumName(newNameStr);
        }
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
    if (ALBUM_PATHTYPE_BY_PHONE == m_albumTypeStr) {
        QPixmap pixmap;
        QPixmap mountpixmap;

        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_iphone_normal.svg", QSize(24, 24));
            mountpixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_exit_normal.svg", QSize(24, 24));
        }
        if (themeType == DGuiApplicationHelper::DarkType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_iphone_normal_dark.svg", QSize(24, 24));
            mountpixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_exit_normal_dark.svg", QSize(24, 24));
        }

        pImageLabel->setPixmap(pixmap);
        m_unMountBtn->setPixmap(mountpixmap);
    } else if (ALBUM_PATHTYPE_BY_U == m_albumTypeStr) {
        QPixmap pixmap;
        QPixmap mountpixmap;

        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_usb_normal.svg", QSize(24, 24));
            mountpixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_exit_normal.svg", QSize(24, 24));
        }
        if (themeType == DGuiApplicationHelper::DarkType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_usb_normal_dark.svg", QSize(24, 24));
            mountpixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_exit_normal_dark.svg", QSize(24, 24));
        }

        pImageLabel->setPixmap(pixmap);
        m_unMountBtn->setPixmap(mountpixmap);
    } else if (COMMON_STR_CUSTOM == m_albumTypeStr || COMMON_STR_CREATEALBUM == m_albumTypeStr) {
        QPixmap pixmap;
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_album_normal.svg", QSize(24, 24));
        }
        if (themeType == DGuiApplicationHelper::DarkType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_album_normal_dark.svg", QSize(24, 24));
        }
        pImageLabel->setPixmap(pixmap);
    } else if (COMMON_STR_RECENT_IMPORTED == m_albumNameStr) {
        QPixmap pixmap;
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_import_normal.svg", QSize(24, 24));
        }
        if (themeType == DGuiApplicationHelper::DarkType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_import_normal_dark.svg", QSize(24, 24));
        }
        pImageLabel->setPixmap(pixmap);

    } else if (COMMON_STR_TRASH == m_albumNameStr) {
        QPixmap pixmap;
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_trash_normal.svg", QSize(24, 24));
        }
        if (themeType == DGuiApplicationHelper::DarkType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_trash_normal_dark.svg", QSize(24, 24));
        }
        pImageLabel->setPixmap(pixmap);

    } else if (COMMON_STR_FAVORITES == m_albumNameStr) {
        QPixmap pixmap;
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_collection_normal.svg", QSize(24, 24));
        }
        if (themeType == DGuiApplicationHelper::DarkType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_collection_normal_dark.svg", QSize(24, 24));
        }
        pImageLabel->setPixmap(pixmap);

    }  else {
        QPixmap pixmap;
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_album_normal.svg", QSize(24, 24));
        }
        if (themeType == DGuiApplicationHelper::DarkType) {
            pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_album_normal_dark.svg", QSize(24, 24));
        }
        pImageLabel->setPixmap(pixmap);
    }

    m_nameLabel->setForegroundRole(DPalette::TextTitle);
}

void AlbumLeftTabItem::newAlbumStatus()
{
    if (ALBUM_PATHTYPE_BY_PHONE == m_albumTypeStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/active/icon_iphone_active.svg", QSize(24, 24));
        pImageLabel->setPixmap(pixmap);

        QPixmap mountpixmap;
        mountpixmap = utils::base::renderSVG(":/resources/images/sidebar/active/icon_exit_active.svg", QSize(24, 24));
        m_unMountBtn->setPixmap(mountpixmap);
    } else if (ALBUM_PATHTYPE_BY_U == m_albumTypeStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/active/icon_usb_active.svg", QSize(24, 24));
        pImageLabel->setPixmap(pixmap);

        QPixmap mountpixmap;
        mountpixmap = utils::base::renderSVG(":/resources/images/sidebar/active/icon_exit_active.svg", QSize(24, 24));
        m_unMountBtn->setPixmap(mountpixmap);
    } else if (COMMON_STR_CUSTOM == m_albumTypeStr || COMMON_STR_CREATEALBUM == m_albumTypeStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/active/icon_album_active.svg", QSize(24, 24));
        pImageLabel->setPixmap(pixmap);
    } else if (COMMON_STR_RECENT_IMPORTED == m_albumNameStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/active/icon_import_active.svg", QSize(24, 24));
        pImageLabel->setPixmap(pixmap);
    } else if (COMMON_STR_TRASH == m_albumNameStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/active/icon_trash_active.svg", QSize(24, 24));
        pImageLabel->setPixmap(pixmap);
    } else if (COMMON_STR_FAVORITES == m_albumNameStr) {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/active/icon_collection_active.svg", QSize(24, 24));
        pImageLabel->setPixmap(pixmap);
    } else {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/active/icon_album_active.svg", QSize(24, 24));
        pImageLabel->setPixmap(pixmap);
    }

    m_nameLabel->setForegroundRole(DPalette::HighlightedText);
}

