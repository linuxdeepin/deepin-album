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
#include <QLabel>
#include <QLineEdit>
#include <QKeyEvent>
#include <QDebug>
#include <DPushButton>

AlbumCreateDialog::AlbumCreateDialog(QWidget* parent)
    :Dialog(parent)
{
    setModal(true);

//    setIconPixmap(QPixmap(":/dialogs/images/resources/images/album_bg_normal.png"));
//    addButton(tr("Cancel"), false, DDialog::ButtonNormal);
//    addButton(tr("OK"), true, DDialog::ButtonRecommend);

    // 添加标题
    const QString subStyle =
    utils::base::getFileContent(":/dialogs/qss/resources/qss/inputdialog.qss");
    QLabel *title = new QLabel(tr("新建相册"));
    QFont ft;
    ft.setPixelSize(17);
    title->setFont(ft);
    title->setObjectName("DialogTitle");
    title->setAlignment(Qt::AlignHCenter);

    //添加输入文本框
    QLineEdit *edit = new QLineEdit;
//    edit->setStyleSheet(subStyle);
    edit->setObjectName("DialogEdit");
    edit->setText(getNewAlbumName());
    edit->setContextMenuPolicy(Qt::PreventContextMenu);
    edit->setFixedSize(360, 36);
    QFont ft1;
    ft1.setPixelSize(14);         //设置字体大小
    edit->setFont(ft1);

    connect(this, &AlbumCreateDialog::visibleChanged, this, [=] (bool v) {
        if (! v) return;
        edit->setFocus();
        edit->selectAll();
    });
    connect(edit, &QLineEdit::returnPressed, this, [=] {
        const QString album = edit->text().trimmed();
        if (! album.isEmpty()) {
            createAlbum(album);
            this->close();
        }
    });

    QWidget *mainWidget = new QWidget;
    mainWidget->setFixedSize(380,180);

    //添加图标
    QLabel *m_picture = new QLabel(mainWidget);
    QIcon icon = QIcon::fromTheme(":/resources/images/other/deepin-album.svg");  //图片路径
    m_picture->setPixmap(icon.pixmap(QSize(32, 32)));                            //图标大小
    m_picture->move(10,9);

    //添加OK和取消按钮
    DPushButton *m_Cancel = new DPushButton("Cancel");
    m_Cancel->setFixedSize(170,36);
    m_Cancel->setFont(ft1);
    DPushButton *m_OK = new DPushButton("OK");
    m_OK->setFixedSize(170,36);
    m_OK->setFont(ft1);

    QHBoxLayout *h_layout  = new QHBoxLayout;
    QHBoxLayout *h_layout1 = new QHBoxLayout;
    QHBoxLayout *h_layout2 = new QHBoxLayout;
    QVBoxLayout *layout = new QVBoxLayout();

    //界面布局
    layout->setContentsMargins(0, 0, 0, 0);
    h_layout->addWidget(title,Qt::AlignHCenter);
    layout->addSpacing(12);
    layout->addLayout(h_layout);
    layout->addSpacing(41);
    h_layout1->addStretch();
    h_layout1->addWidget(edit);
    h_layout1->addStretch();
    layout->addLayout(h_layout1);
    layout->addSpacing(20);
    h_layout2->addWidget(m_Cancel);
    h_layout2->addSpacing(20);
    h_layout2->addWidget(m_OK);
    layout->addLayout(h_layout2);
    mainWidget->setLayout(layout);
    addContent(mainWidget);

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
