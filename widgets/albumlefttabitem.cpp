#include "widgets/albumlefttabitem.h"
#include "dbmanager/dbmanager.h"
#include "utils/baseutils.h"

#include <QHBoxLayout>
namespace
{
const int LAYOUT_SPACING = 10;
const int OPE_MODE_ADDNEWALBUM = 0;
const int OPE_MODE_RENAMEALBUM = 1;
const QString RECENT_IMPORTED_ALBUM = "Recent imported";
const QString TRASH_ALBUM = "Trash";
const QString FAVORITES_ALBUM = "My favorite";
}// namespace

AlbumLeftTabItem::AlbumLeftTabItem(QString str)
{
    m_albumNameStr = str;
    m_opeMode = 0;

    initUI();
    initConnections();
}

AlbumLeftTabItem::~AlbumLeftTabItem()
{

}

void AlbumLeftTabItem::initConnections()
{
    connect(m_pLineEdit, &DLineEdit::editingFinished, this, &AlbumLeftTabItem::onCheckNameValid);
}

void AlbumLeftTabItem::initUI()
{
    setFixedSize(200, 36);
    QHBoxLayout *pHBoxLayout = new QHBoxLayout();
    pHBoxLayout->setSpacing(10);

    QLabel* pLabel = new QLabel();
    pLabel->setFixedSize(24, 24);

    if (RECENT_IMPORTED_ALBUM == m_albumNameStr)
    {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_import_normal.svg", QSize(24, 24));
        pLabel->setPixmap(pixmap);
    }
    else if (TRASH_ALBUM == m_albumNameStr)
    {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_trash_normal.svg", QSize(24, 24));
        pLabel->setPixmap(pixmap);
    }
    else if (FAVORITES_ALBUM == m_albumNameStr)
    {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_collection_normal.svg", QSize(24, 24));
        pLabel->setPixmap(pixmap);
    }
    else
    {
        QPixmap pixmap;
        pixmap = utils::base::renderSVG(":/resources/images/sidebar/normal/icon_album_normal.svg", QSize(24, 24));
        pLabel->setPixmap(pixmap);
    }

    DWidget *pWidget = new DWidget();

    m_nameLabel = new QLabel(pWidget);
    m_nameLabel->setGeometry(QRect(0, 0, 150, 20));
//    m_nameLabel->setText(streElide);

    QFontMetrics elideFont(m_nameLabel->font());
    m_nameLabel->setText(elideFont.elidedText(m_albumNameStr, Qt::ElideRight, m_nameLabel->width()));
    QFont ft;
    ft.setPixelSize(14);
    m_nameLabel->setFont(ft);

    m_pLineEdit = new DLineEdit(pWidget);
    m_pLineEdit->setGeometry(QRect(0, 0, 150, 20));
    m_pLineEdit->setText(m_albumNameStr);
    m_pLineEdit->setVisible(false);
    m_pLineEdit->lineEdit()->setMaxLength(64);

    pHBoxLayout->addWidget(pLabel);
    pHBoxLayout->addWidget(pWidget);

    this->setLayout(pHBoxLayout);
}

void AlbumLeftTabItem::editAlbumEdit()
{
    m_nameLabel->setVisible(false);
    m_pLineEdit->setVisible(true);
    m_pLineEdit->lineEdit()->selectAll();
    m_pLineEdit->lineEdit()->setFocus();
}

void AlbumLeftTabItem::onCheckNameValid()
{
    QString newNameStr = m_pLineEdit->text().trimmed();

    if (DBManager::instance()->getAllAlbumNames().contains(newNameStr)
        || RECENT_IMPORTED_ALBUM == newNameStr
        || TRASH_ALBUM == newNameStr
        || FAVORITES_ALBUM == newNameStr
        || newNameStr.isEmpty())
    {
        newNameStr = m_albumNameStr;
    }

    if (OPE_MODE_RENAMEALBUM == m_opeMode)
    {
        m_nameLabel->setText(newNameStr);
        QFontMetrics elideFont(m_nameLabel->font());
        m_nameLabel->setText(elideFont.elidedText(newNameStr, Qt::ElideRight, m_nameLabel->width()));
        QFont ft;
        ft.setPixelSize(14);

        m_pLineEdit->setText(newNameStr);

        m_nameLabel->setVisible(true);
        m_pLineEdit->setVisible(false);

        DBManager::instance()->renameAlbum(m_albumNameStr, newNameStr);

        m_albumNameStr = newNameStr;
    }

    if (OPE_MODE_ADDNEWALBUM == m_opeMode)
    {
        m_nameLabel->setText(newNameStr);
        QFontMetrics elideFont(m_nameLabel->font());
        m_nameLabel->setText(elideFont.elidedText(newNameStr, Qt::ElideRight, m_nameLabel->width()));
        QFont ft;
        ft.setPixelSize(14);

        m_pLineEdit->setText(newNameStr);

        m_nameLabel->setVisible(true);
        m_pLineEdit->setVisible(false);

        DBManager::instance()->insertIntoAlbum(newNameStr, QStringList(" "));

        m_albumNameStr = newNameStr;
    }
}
