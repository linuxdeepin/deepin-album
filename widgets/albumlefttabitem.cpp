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
    m_nameLabel->setText(m_albumNameStr);

    m_pLineEdit = new DLineEdit(pWidget);
    m_pLineEdit->setGeometry(QRect(0, 0, 150, 20));
    m_pLineEdit->setText(m_albumNameStr);
    m_pLineEdit->setVisible(false);

    pHBoxLayout->addWidget(pLabel);
    pHBoxLayout->addWidget(pWidget);

    this->setLayout(pHBoxLayout);
}

void AlbumLeftTabItem::editAlbumEdit()
{
    m_nameLabel->setVisible(false);
    m_pLineEdit->setVisible(true);
    m_pLineEdit->selectAll();
}

void AlbumLeftTabItem::onCheckNameValid()
{
    if ((m_pLineEdit->text().length() > 0) && (m_pLineEdit->text().length() < 64))
    {
        if (OPE_MODE_RENAMEALBUM == m_opeMode)
        {
            m_nameLabel->setText(m_pLineEdit->text());
            m_pLineEdit->setText(m_pLineEdit->text());

            m_nameLabel->setVisible(true);
            m_pLineEdit->setVisible(false);

            DBManager::instance()->renameAlbum(m_albumNameStr, m_pLineEdit->text());

            m_albumNameStr = m_pLineEdit->text();
        }

        if (OPE_MODE_ADDNEWALBUM == m_opeMode)
        {
            if (DBManager::instance()->getAllAlbumNames().contains(m_pLineEdit->text()))
            {
                m_pLineEdit->setAlert(true);
                m_pLineEdit->showAlertMessage("相册名重复");
            }
            else
            {
                m_nameLabel->setText(m_pLineEdit->text());
                m_pLineEdit->setText(m_pLineEdit->text());

                m_nameLabel->setVisible(true);
                m_pLineEdit->setVisible(false);

                DBManager::instance()->insertIntoAlbum(m_pLineEdit->text(), QStringList(" "));

                m_albumNameStr = m_pLineEdit->text();
            }
        }
    }
    else
    {
        m_pLineEdit->setAlert(true);
        m_pLineEdit->showAlertMessage("输入字符长度必须在0-64位之间");
    }
}
