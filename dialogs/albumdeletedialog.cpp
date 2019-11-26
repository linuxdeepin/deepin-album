#include "albumdeletedialog.h"
#include "application.h"
#include "dbmanager/dbmanager.h"
#include "utils/baseutils.h"
#include <DLabel>
#include <DPushButton>
#include <DFontSizeManager>
#include "albumview/albumview.h"


AlbumDeleteDialog::AlbumDeleteDialog()
{
    setModal(true);
    this->setFixedSize(380,180);

    DLabel *m_pic = new DLabel(this);
    QIcon icon = QIcon::fromTheme("deepin-album");     //照片路径
    m_pic->setPixmap(icon.pixmap(QSize(32, 32)));  //图标大小

    DLabel *m_label = new DLabel(this);
    DFontSizeManager::instance()->bind(m_label, DFontSizeManager::T5, QFont::DemiBold);
    m_label->setForegroundRole(DPalette::TextTitle);
    m_label->setFixedSize(180,36);
    m_label->setText(tr("Are you sure you want to delete this album?"));

    DPushButton *m_Cancel = new DPushButton(this);
    m_Cancel->setText(tr("Cancel"));
    m_Cancel->setFixedSize(170,36);
    m_Cancel->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));
    DPushButton *m_line = new DPushButton(this);
    m_line->setFixedSize(3,28);
    m_line->setEnabled(false);
    DPushButton *m_Delete = new DPushButton(this);
    m_Delete->setText(tr("Delete"));
    m_Delete->setFixedSize(170,36);
    m_Delete->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));

    m_pic->move(9,9);
    m_label->move(115,60);
    m_Cancel->move(10,134);
    m_line->move(189,138);
    m_Delete->move(200,134);

    connect(m_Cancel,&DPushButton::clicked,this,&AlbumDeleteDialog::deleteLater);
    connect(m_Delete,&DPushButton::clicked,this,[=]{
        emit deleteAlbum();
        this->close();
    });
}
