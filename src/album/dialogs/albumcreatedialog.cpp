// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
    if (e->key() == Qt::Key_Escape) {
        emit sigClose();
        this->close();
    }
}

bool AlbumCreateDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Tab) {
            // 修改编辑框焦点方式
            edit->lineEdit()->setFocusPolicy(Qt::ClickFocus);
            // cancel sure btn tab foucs
            if (getButton(0) == obj) {
                getButton(1)->setFocus();
                return true;
            } else if (getButton(1) == obj) {
                m_allTabOrder.at(3)->setFocus();
                return true;
            } else if (obj->objectName() == "DTitlebarDWindowCloseButton") {
                edit->lineEdit()->setFocus();
                edit->lineEdit()->selectAll();
                return true;
            }
        } else if (keyEvent->key() == Qt::Key_Left || keyEvent->key() == Qt::Key_Right
                   || keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down) {
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
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
    edit->lineEdit()->setMaxLength(utils::common::ALBUM_NAME_MAX_LENGTH);

    // 相册名称不能包含正斜杠/，与文件路径分割符冲突，否则导出的相册会保存在以"/"之前的字串命名的目录中
    // 相关bug见：https://pms.uniontech.com/bug-view-163885.html
    QRegExp regExp("[^/]+");
    QRegExpValidator *pattern = new QRegExpValidator(regExp, this);
    edit->lineEdit()->setValidator(pattern);

    DFontSizeManager::instance()->bind(edit, DFontSizeManager::T6, QFont::Medium);
    addButton(tr("Cancel"), false, DDialog::ButtonNormal);
    addButton(tr("Create"), true, DDialog::ButtonRecommend);
    getButton(0)->setFixedSize(170, 35);
    getButton(1)->setFixedSize(170, 35);
    connect(edit, &DLineEdit::textEdited, this, &AlbumCreateDialog::onTextEdited);
    addContent(contentWidget);
    QWidget *closeButton =  this->findChild<QWidget *>("DTitlebarDWindowCloseButton");
    closeButton->setFocusPolicy(Qt::TabFocus);
    //键盘交互
    getButton(0)->installEventFilter(this);
    getButton(1)->installEventFilter(this);
    closeButton->installEventFilter(this);
    edit->installEventFilter(this);

    m_allTabOrder.clear();
    m_allTabOrder.insert(0, edit);
    m_allTabOrder.insert(1, getButton(0));
    m_allTabOrder.insert(2, getButton(1));
    m_allTabOrder.insert(3, closeButton);
}

void AlbumCreateDialog::initConnection()
{
    connect(this, &AlbumCreateDialog::visibleChanged, this, &AlbumCreateDialog::onVisibleChanged);
    connect(edit, &DLineEdit::returnPressed, this, &AlbumCreateDialog::onReturnPressed);
    connect(this, &AlbumCreateDialog::buttonClicked, this, &AlbumCreateDialog::onButtonClicked);
    connect(this, &AlbumCreateDialog::closed, this, &AlbumCreateDialog::onClosed);
}

//需求变更：允许相册重名，空字符串返回Unnamed，其余字符串返回本名
const QString AlbumCreateDialog::getNewAlbumName(const QString &baseName)
{
    QString albumName;
    if (baseName.isEmpty()) {
        albumName = tr("Unnamed");
    } else {
        albumName = baseName;
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

int AlbumCreateDialog::getCreateUID() const
{
    return m_createUID;
}

void AlbumCreateDialog::createAlbum(const QString &newName)
{
    m_createAlbumName = getNewAlbumName(newName);
    m_createUID = DBManager::instance()->createAlbum(m_createAlbumName, QStringList(" "));

    emit albumAdded(m_createUID);
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
    edit->lineEdit()->setFocus();
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
