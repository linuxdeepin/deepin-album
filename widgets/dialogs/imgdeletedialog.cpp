#include "imgdeletedialog.h"
#include <DLabel>
#include <DPushButton>
#include <DFontSizeManager>
#include <DApplicationHelper>

ImgDeleteDialog::ImgDeleteDialog(int count)
{
    setModal(true);
    this->setFixedSize(380,180);

    DLabel *m_pic = new DLabel(this);
    QIcon icon = QIcon::fromTheme("deepin-album");     //图片路径
    m_pic->setPixmap(icon.pixmap(QSize(32, 32)));  //图标大小
    DLabel *m_label = new DLabel(this);
    if(1 == count)
    {
        m_label->setText("您确定要从相册删除此图片吗 ?");
        DPalette pa = DApplicationHelper::instance()->palette(m_label);
        pa.setBrush(DPalette::WindowText, pa.color(DPalette::TextTitle));
        m_label->setPalette(pa);

    }
    else
    {
        QString str = tr("您确定要从相册删除%1张图片吗 ?");
        m_label->setText(str.arg(count));
        DPalette pa = DApplicationHelper::instance()->palette(m_label);
        pa.setBrush(DPalette::WindowText, pa.color(DPalette::TextTitle));
        m_label->setPalette(pa);

    }
    m_label->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));
    m_label->adjustSize();
    m_label->move((this->width() - m_label->width())/2,70);

    DPushButton *m_Cancel = new DPushButton(this);
    m_Cancel->setText("取消");
    m_Cancel->setFixedSize(170,36);
    m_Cancel->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));
    DPushButton *m_line = new DPushButton(this);
    m_line->setFixedSize(3,28);
    m_line->setEnabled(false);
    DPushButton *m_Delete = new DPushButton(this);
    m_Delete->setText("删除");
    m_Delete->setFixedSize(170,36);
    m_Delete->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));

    m_pic->move(9,9);
    m_Cancel->move(10,134);
    m_line->move(189,138);
    m_Delete->move(200,134);

    connect(m_Cancel,&DPushButton::clicked,this,&ImgDeleteDialog::deleteLater);
    connect(m_Delete,&DPushButton::clicked,this,[=]{
        emit imgdelete();
        this->close();
    });
}
