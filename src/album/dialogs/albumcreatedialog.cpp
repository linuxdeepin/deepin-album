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
#include "ac-desktop-define.h"
#include <QHBoxLayout>
#include <QLabel>
#include <DLineEdit>
#include <QKeyEvent>
#include <QDebug>
#include <DPushButton>
#include <DFontSizeManager>
#include <DApplicationHelper>
#include <DSuggestButton>

AlbumCreateDialog::AlbumCreateDialog(DWidget *parent)
    : DDialog(parent), edit(nullptr), m_OKClicked(false)
{
    initUI();
    initConnection();
    getEdit();
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
    setFixedSize(380, 180);
    setModal(true);
    setContentsMargins(0, 0, 0, 0);
    setSpacing(0);
    //内容widget
    DWidget *contentWidget = new DWidget(this);
    contentWidget->setContentsMargins(0, 0, 0, 0);

    //图标
    DLabel *logoLable = new DLabel(this);
    QIcon icon = QIcon::fromTheme("deepin-album");
    logoLable->setPixmap(icon.pixmap(QSize(32, 32)));
    logoLable->move(10, 9);
    logoLable->setAlignment(Qt::AlignLeft);
//title
    const QString subStyle = utils::base::getFileContent(":/dialogs/qss/resources/qss/inputdialog.qss");
    DLabel *title = new DLabel(this);
    DFontSizeManager::instance()->bind(title, DFontSizeManager::T6, QFont::DemiBold);
    title->setForegroundRole(DPalette::TextTitle);
    title->setText(tr("New Album"));
    title->setFixedSize(293, 30);
    title->setObjectName("DialogTitle");
    title->setAlignment(Qt::AlignHCenter);
    title->move(44, 49);

//编辑框
    edit = new DLineEdit(this);
    edit->setEnabled(true);
    edit->setObjectName("DialogEdit");
    edit->setText(getNewAlbumName(""));
    edit->setContextMenuPolicy(Qt::PreventContextMenu);
    edit->setClearButtonEnabled(false);
    edit->setFixedSize(360, 36);
    edit->move(10, 79);
    DFontSizeManager::instance()->bind(edit, DFontSizeManager::T6, QFont::DemiBold);
    addButton(tr("Cancel"), false, DDialog::ButtonNormal);
    addButton(tr("Create"), true, DDialog::ButtonRecommend);
    getButton(0)->setFixedSize(170, 35);
    getButton(1)->setFixedSize(170, 35);
    connect(edit, &DLineEdit::textEdited, this, &AlbumCreateDialog::onTextEdited);
    addContent(contentWidget);
    QWidget *closeButton =  this->findChild<QWidget *>("DTitlebarDWindowCloseButton");
    closeButton->setFocusPolicy(Qt::NoFocus);
    edit->lineEdit()->setFocusPolicy(Qt::NoFocus);
    this->setTabOrder(getButton(0), getButton(1));
}

void AlbumCreateDialog::initConnection()
{
    connect(this, &AlbumCreateDialog::visibleChanged, this, &AlbumCreateDialog::onVisibleChanged);
    connect(edit, &DLineEdit::returnPressed, this, &AlbumCreateDialog::onReturnPressed);
    connect(this, &AlbumCreateDialog::buttonClicked, this, &AlbumCreateDialog::onButtonClicked);
    connect(this, &AlbumCreateDialog::closed, this, &AlbumCreateDialog::onClosed);
}

/**
 * @brief AlbumCreateDialog::getNewAlbumName
 * @param[in] baseName
 * @param[in] isWithOutSelf
 * @param[in] beforeName
 * @author DJH
 * @date 2020/6/1
 * @return const QString
 * 根据已有相册名，获取对于数据库中不重复的新相册名，当isWithOutSefl为true的时候，查询不会包含自己，用于替换型查询
 */
const QString AlbumCreateDialog::getNewAlbumName(const QString &baseName, bool isWithOutSelf, const QString &beforeName)
{
    QString nan;
    QString albumName;
    int num = 1;
    if (baseName.isEmpty()) {
        nan = tr("Unnamed");
        albumName = nan;
    } else {
        nan = baseName;
        albumName = nan + QString::number(num);
    }
    QStringList albums = DBManager::instance()->getAllAlbumNames();
    if (isWithOutSelf) {
        albums.removeOne(beforeName);
    }

    while (albums.contains(albumName)) {
        albumName = nan + QString::number(num);
        num++;
    }
    return static_cast<const QString>(albumName);
}

DLineEdit *AlbumCreateDialog::getEdit()
{
    if (edit != nullptr)
        return edit;
    return nullptr;
}

const QString AlbumCreateDialog::getCreateAlbumName() const
{
    return m_createAlbumName;
}

void AlbumCreateDialog::createAlbum(const QString &newName)
{
    if (!DBManager::instance()->isAlbumExistInDB(newName)) {
        m_createAlbumName = newName;
        DBManager::instance()->insertIntoAlbum(newName, QStringList(" "));
    } else {
        m_createAlbumName = getNewAlbumName(newName);
        DBManager::instance()->insertIntoAlbum(m_createAlbumName, QStringList(" "));
    }

    emit albumAdded();
}

void AlbumCreateDialog::onTextEdited(const QString &)
{
    if (edit->text().trimmed().isEmpty()) {
        getButton(1)->setEnabled(false);
    } else {
        getButton(1)->setEnabled(true);
    }
}

void AlbumCreateDialog::onVisibleChanged(bool v)
{
    if (! v) return;
    edit->lineEdit()->selectAll();
//    edit->lineEdit()->setFocus();
}

void AlbumCreateDialog::onReturnPressed()
{
    const QString album = edit->text().trimmed();
    if (! album.isEmpty()) {
        createAlbum(album);
        this->close();
    }
}

void AlbumCreateDialog::onButtonClicked(int index)
{
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
}

void AlbumCreateDialog::onClosed()
{
    deleteLater();
    if (true == m_OKClicked) {
        m_OKClicked = false;
    } else {
        emit sigClose();
    }
}
