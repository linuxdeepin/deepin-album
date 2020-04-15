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
#include <DSuggestButton>

AlbumCreateDialog::AlbumCreateDialog(DWidget *parent)
    : DDialog(parent)
{
    m_OKClicked = false;
    initUI();
    initConnection();
}

void AlbumCreateDialog::keyPressEvent(QKeyEvent *e)
{
    qDebug() << "AlbumCreateDialog::keyPressEvent()";
    if (e->key() == Qt::Key_Escape) {
        emit sigClose();
        this->close();
    }
}

void AlbumCreateDialog::initUI()
{
    setFixedSize(380, 190);
    setModal(true);
    setContentsMargins(0, 0, 0, 0);

//内容widget
    DWidget *contentWidget = new DWidget(this);
    contentWidget->setContentsMargins(0, 0, 0, 0);
    QVBoxLayout *contentLayout = new QVBoxLayout();
    Q_UNUSED(contentLayout);

//图标
    DLabel *logoLable = new DLabel(this);
    QIcon icon = QIcon::fromTheme("deepin-album");
    logoLable->setPixmap(icon.pixmap(QSize(32, 32)));
    logoLable->move(10, 9);
    logoLable->setAlignment(Qt::AlignLeft);
//title
    const QString subStyle =
        utils::base::getFileContent(":/dialogs/qss/resources/qss/inputdialog.qss");
    DLabel *title = new DLabel(this);
    DFontSizeManager::instance()->bind(title, DFontSizeManager::T5, QFont::DemiBold);
    title->setForegroundRole(DPalette::TextTitle);
    title->setText(tr("New Album"));
//    title->setFixedSize(68,25);
    title->setFixedSize(130, 35);
    title->setObjectName("DialogTitle");
    title->setAlignment(Qt::AlignHCenter);
    title->move(130, 12);
//编辑框
    edit = new DLineEdit(contentWidget);
    edit->setEnabled(true);
    edit->setObjectName("DialogEdit");
    edit->setText(getNewAlbumName());
    edit->setContextMenuPolicy(Qt::PreventContextMenu);
    edit->setClearButtonEnabled(false);
    edit->setFixedSize(360, 36);
    edit->move(0, 23);
    edit->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));

//    contentLayout->addWidget(edit);
//    contentWidget->setLayout(contentLayout);

//按钮
    addButton(tr("Cancel"), false, DDialog::ButtonNormal);
    addButton(tr("Create"), true, DDialog::ButtonRecommend);

//    m_Cancel = new DPushButton(this);
//    m_Cancel->setText(tr("Cancel"));
//    m_Cancel->setFixedSize(170,36);
//    m_Cancel->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));
//    DPushButton *m_line = new DPushButton(this);

//    DPalette m_line_pa = DApplicationHelper::instance()->palette(m_line);
//    m_line_pa.setBrush(DPalette::TextWarning, QColor(0, 0, 0, 0));
//    m_line->setPalette(m_line_pa);

//    m_line->setFixedSize(1,28);
//    m_line->setEnabled(false);

//    QObject::connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
//    this, [ = ] {
//        DPalette m_line_pa = DApplicationHelper::instance()->palette(m_line);
//        m_line_pa.setBrush(DPalette::TextWarning, QColor(0, 0, 0, 0));
//        m_line->setPalette(m_line_pa);
//    });

//    m_OK = new DSuggestButton(this);
//    m_OK->setText(tr("Create"));
//    m_OK->setFixedSize(170,36);
//    m_OK->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));

//    m_Cancel->move(10,134);
//    m_line->move(189,138);
//    m_OK->move(200,134);

    addContent(contentWidget);
}

void AlbumCreateDialog::initConnection()
{
    connect(this, &AlbumCreateDialog::visibleChanged, this, [ = ](bool v) {
        if (! v) return;
        edit->lineEdit()->selectAll();
        edit->lineEdit()->setFocus();
    });
    connect(edit, &DLineEdit::returnPressed, this, [ = ] {
        const QString album = edit->text().trimmed();
        if (! album.isEmpty())
        {
            createAlbum(album);
            this->close();
        }
    });
//    connect(m_Cancel,&DPushButton::clicked,this, [=]{
//        deleteLater();
//        emit sigClose();
//    });

//    connect(m_OK,&DPushButton::clicked,this,[=]{
//        if (edit->text().simplified().length()!= 0)
//        {
//            createAlbum(edit->text().trimmed());
//        }
//        else
//        {
//            QString str = tr("Unnamed") + QString::number(1);
//            createAlbum(str);
//        }

//        m_OKClicked = true;
//        this->close();
//    });

    connect(this, &AlbumCreateDialog::buttonClicked, this, [ = ](int index) {
        if (0 == index) {
            deleteLater();
            emit sigClose();
        }
        if (1 == index) {
            if (edit->text().simplified().length() != 0) {
                createAlbum(edit->text().trimmed());
            } else {
                QString str = tr("Unnamed") + QString::number(1);
                createAlbum(str);
            }

            m_OKClicked = true;
            this->close();
        }
    });

    connect(this, &AlbumCreateDialog::closed, this, [ = ] {
        deleteLater();
        if (true == m_OKClicked)
        {
            m_OKClicked = false;
        } else
        {
            emit sigClose();
        }

    });
//    connect(this, &AlbumCreateDialog::buttonClicked, this, [=] (int id) {
//        if (id == 1) {
//            if (edit->text().simplified().length()!= 0)
//                createAlbum(edit->text().trimmed());
//            else
//                createAlbum(tr("Unnamed"));
//        }

//    });

}

/*!
 * \brief AlbumCreateDialog::getNewAlbumName
 * \return Return a string like "Unnamed3", &etc
 */
const QString AlbumCreateDialog::getNewAlbumName() const
{
    const QString nan = tr("Unnamed");
    int num = 1;
    QString albumName = nan + QString::number(num);
    while (DBManager::instance()->isAlbumExistInDB(albumName)) {
        num++;
        albumName = nan + QString::number(num);
    }
    return static_cast<const QString>(albumName);
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
    } else {
        m_createAlbumName = getNewAlbumName();
        DBManager::instance()->insertIntoAlbum(getNewAlbumName(), QStringList(" "));
    }

    emit albumAdded();
}
