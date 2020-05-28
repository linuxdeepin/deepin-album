#include "albumdeletedialog.h"
#include "application.h"
#include "dbmanager/dbmanager.h"
#include "utils/baseutils.h"
#include <DLabel>
#include <DPushButton>
#include <DFontSizeManager>
#include "albumview/albumview.h"


AlbumDeleteDialog::AlbumDeleteDialog(DWidget *parent)
{
    Q_UNUSED(parent);
    iniUI();
    setAttribute(Qt::WA_DeleteOnClose, true);
}

void AlbumDeleteDialog::iniUI()
{
    setModal(true);

    //图标
    QIcon icon = QIcon::fromTheme("deepin-album");
    this->setIcon(icon);

    //label
    DLabel *m_label = new DLabel(this);
    DFontSizeManager::instance()->bind(m_label, DFontSizeManager::T6, QFont::DemiBold);
    m_label->setForegroundRole(DPalette::TextTitle);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setText(tr("Are you sure you want to delete this album?"));

    this->insertContent(0, m_label);

    //取消按钮
    DPushButton *m_Cancel = new DPushButton(this);
    m_Cancel->setText(tr("Cancel"));
    m_Cancel->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));

    //确认删除按钮
    DPushButton *m_Delete = new DPushButton(this);
    m_Delete->setText(tr("Delete"));
    m_Delete->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));

    this->insertButton(1, m_Cancel);
    this->insertButton(2, m_Delete);

    connect(m_Cancel, &DPushButton::clicked, this, &AlbumDeleteDialog::deleteLater);
    connect(m_Delete, &DPushButton::clicked, this, [ = ] {
        emit deleteAlbum();
        this->close();
    });
}
