/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "application.h"
#include "albumcreatedialog.h"
#include "dbmanager/dbmanager.h"
#include "utils/baseutils.h"
#include <QHBoxLayout>
#include <DLabel>
#include <DLineEdit>
#include <QKeyEvent>
#include <QDebug>
#include <DPushButton>
#include <DFontSizeManager>
#include <DApplicationHelper>

AlbumCreateDialog::AlbumCreateDialog(QWidget* parent)
    :Dialog(parent)
{
    setModal(true);

    // 添加标题
    const QString subStyle =
    utils::base::getFileContent(":/dialogs/qss/resources/qss/inputdialog.qss");
    DLabel *title = new DLabel(this);
    title->setText("新建相册");   
    DPalette pe = DApplicationHelper::instance()->palette(title);
    pe.setBrush(DPalette::WindowText,pe.color(DPalette::TextTitle));
    title->setPalette(pe);    
    title->setFixedSize(68,25);
    title->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
    title->setObjectName("DialogTitle");
    title->setAlignment(Qt::AlignHCenter);

    //添加输入文本框
    DLineEdit *edit = new DLineEdit(this);
    edit->setObjectName("DialogEdit");
    edit->setText(getNewAlbumName());
    edit->setContextMenuPolicy(Qt::PreventContextMenu);
    edit->setClearButtonEnabled(false);
    edit->setFixedSize(360, 36);
    edit->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));

    connect(this, &AlbumCreateDialog::visibleChanged, this, [=] (bool v) {
        if (! v) return;
        edit->lineEdit()->selectAll();
        edit->lineEdit()->setFocus();
    });
    connect(edit, &DLineEdit::returnPressed, this, [=] {
        const QString album = edit->text().trimmed();
        if (! album.isEmpty()) {
            createAlbum(album);
            this->close();
        }
    });

    this->setFixedSize(380,180);

    //添加图标
    DLabel *m_picture = new DLabel(this);
    QIcon icon = QIcon::fromTheme("deepin-album");     //照片路径
    m_picture->setPixmap(icon.pixmap(QSize(32, 32)));  //图标大小

    //添加OK和取消按钮
    DPushButton *m_Cancel = new DPushButton(this);
    m_Cancel->setText("取消");
    m_Cancel->setFixedSize(170,36);
    m_Cancel->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));
    DPushButton *m_line = new DPushButton(this);
    m_line->setFixedSize(3,28);
    m_line->setEnabled(false);
    DPushButton *m_OK = new DPushButton(this);
    m_OK->setText("新建");
    m_OK->setFixedSize(170,36);
    m_OK->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));

    //界面布局
    m_picture->move(9,9);
    title->move(156,12);
    edit->move(10,78);
    m_Cancel->move(10,134);
    m_line->move(189,138);
    m_OK->move(200,134);

//    QHBoxLayout *h_layout  = new QHBoxLayout(this);
//    QHBoxLayout *h_layout1 = new QHBoxLayout(this);
//    QHBoxLayout *h_layout2 = new QHBoxLayout(this);
//    QVBoxLayout *layout = new QVBoxLayout(this);
//    layout->setContentsMargins(0, 0, 0, 0);
//    h_layout->addWidget(title,Qt::AlignHCenter);
//    layout->addSpacing(12);
//    layout->addLayout(h_layout);
//    layout->addSpacing(41);
//    h_layout1->addStretch();
//    h_layout1->addWidget(edit);
//    h_layout1->addStretch();
//    layout->addLayout(h_layout1);
//    layout->addSpacing(20);
//    h_layout2->addWidget(m_Cancel);
//    h_layout2->addSpacing(9);
//    h_layout2->addWidget(m_line);
//    h_layout2->addSpacing(8);
//    h_layout2->addWidget(m_OK);
//    layout->addLayout(h_layout2);
//    setLayout(layout);
//    addContent(main);

    connect(m_Cancel,&DPushButton::clicked,this,&AlbumCreateDialog::deleteLater);
    connect(m_OK,&DPushButton::clicked,this,[=]{
        if (edit->text().simplified().length()!= 0)
        {
            createAlbum(edit->text().trimmed());
        }
        else
        {
            createAlbum(tr("Unnamed"));
        }

        this->close();
    });

//    connect(this, &AlbumCreateDialog::closed,
//            this, &AlbumCreateDialog::deleteLater);
//    connect(this, &AlbumCreateDialog::buttonClicked, this, [=] (int id) {
//        if (id == 1) {
//            if (edit->text().simplified().length()!= 0)
//                createAlbum(edit->text().trimmed());
//            else
//                createAlbum(tr("Unnamed"));
//        }

//    });

}

void AlbumCreateDialog::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape) {
        this->close();
    }
}

/*!
 * \brief AlbumCreateDialog::getNewAlbumName
 * \return Return a string like "Unnamed3", &etc
 */
const QString AlbumCreateDialog::getNewAlbumName() const
{
    const QString nan = tr("Unnamed");
       int num = 1;
       QString albumName = nan;
       while(DBManager::instance()->isAlbumExistInDB(albumName)) {
           num++;
           albumName = nan + QString::number(num);
       }
       return (const QString)(albumName);
}

const QString AlbumCreateDialog::getCreateAlbumName() const
{
    return m_createAlbumName;
}

void AlbumCreateDialog::createAlbum(const QString &newName)
{
    if (! DBManager::instance()->getAllAlbumNames().contains(newName)) {
        m_createAlbumName = newName;
        DBManager::instance()->insertIntoAlbum(newName, QStringList(" "));
    }
    else {
        m_createAlbumName = getNewAlbumName();
        DBManager::instance()->insertIntoAlbum(getNewAlbumName(), QStringList(" "));
    }

    emit albumAdded();
}
