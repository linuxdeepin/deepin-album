#include "imgdeletedialog.h"
#include <DLabel>
#include <DPushButton>
#include <DFontSizeManager>
#include <DApplicationHelper>
#include <DSuggestButton>
#include <QVBoxLayout>

ImgDeleteDialog::ImgDeleteDialog(DWidget *parent, int count, bool bdeleteallonlyone)
    : DDialog(parent)
{
    setModal(true);
    setContentsMargins(0, 0, 0, 0);
    QIcon icon = QIcon::fromTheme("deepin-album");
    this->setIcon(icon);
    DLabel *m_label = new DLabel(this);
    if (1 == count && !bdeleteallonlyone) {
        m_label->setText(tr("Are you sure you want to delete this photo from the album?"));
    } else {
        QString str = tr("Are you sure you want to delete %1 photos from albums?");
        m_label->setText(str.arg(count));
    }
    m_label->setWordWrap(true);
    m_label->setAlignment(Qt::AlignHCenter);
    DWidget *contentWidget = new DWidget(this);
    contentWidget->setFixedHeight(this->height() - 80);
    contentWidget->setContentsMargins(0, 0, 0, 0);
    QVBoxLayout *layout = new QVBoxLayout(contentWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addStretch();
    layout->addWidget(m_label);
    layout->addStretch();
    addContent(contentWidget);
    insertButton(0, tr("Cancel"), false, DDialog::ButtonNormal);
    insertButton(1, tr("Delete"), true, DDialog::ButtonWarning);
}
