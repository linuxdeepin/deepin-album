/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     ZhangYong <zhangyong@uniontech.com>
 *
 * Maintainer: ZhangYong <ZhangYong@uniontech.com>
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
#include "leftlistview.h"
#include "widgets/albumlefttabitem.h"
#include "dbmanager/dbmanager.h"
#include "application.h"
#include "controller/configsetter.h"
#include "utils/baseutils.h"
#include "dbmanager/dbmanager.h"
#include "controller/exporter.h"
#include "imageengine/imageengineapi.h"
#include "dialogs/albumcreatedialog.h"
#include "ac-desktop-define.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <DFontSizeManager>
#include <DStyledItemDelegate>

namespace {
const int OPE_MODE_ADDNEWALBUM = 0;
const int OPE_MODE_RENAMEALBUM = 1;
const int LEFT_VIEW_WIDTH_180 = 180;
const int LEFT_VIEW_LISTITEM_WIDTH_160 = 160;
const int LEFT_VIEW_LISTITEM_HEIGHT_40 = 40;
const QString SHORTCUTVIEW_GROUP = "SHORTCUTVIEW";

QString ss(const QString &text)
{
    QString str = dApp->setter->value(SHORTCUTVIEW_GROUP, text).toString();
    str.replace(" ", "");
    return str;
}
};

LeftListView::LeftListView(QWidget *parent)
    : DScrollArea(parent), m_pPhotoLibLabel(nullptr), m_pPhotoLibListView(nullptr)
    , m_pCustomizeLabel(nullptr), m_pAddListBtn(nullptr), m_pCustomizeListView(nullptr)
    , m_pMountLabel(nullptr), m_pMountListWidget(nullptr), m_pMountWidget(nullptr)
    , m_ItemCurrentName(COMMON_STR_RECENT_IMPORTED)
    , m_ItemCurrentType(COMMON_STR_RECENT_IMPORTED)
    , m_pMenu(nullptr)
{
    m_ItemCurrentDataType = 0;
    initUI();
    initMenu();
    initConnections();
}

void LeftListView::initConnections()
{
    connect(m_pPhotoLibListView, &DListWidget::pressed, this, &LeftListView::onPhotoLibListViewPressed);
    connect(m_pCustomizeListView, &DListWidget::pressed, this, &LeftListView::onCustomListViewPressed);
    connect(m_pMountListWidget, &DListWidget::pressed, this, &LeftListView::onMountListViewPressed);
    connect(m_pPhotoLibListView, &DListWidget::currentItemChanged, this, &LeftListView::onPhotoLibListViewCurrentItemChanged);
    connect(m_pCustomizeListView, &DListWidget::currentItemChanged, this, &LeftListView::onCustomizeListViewCurrentItemChanged);
    connect(m_pMountListWidget, &DListWidget::currentItemChanged, this, &LeftListView::onMountListWidgetCurrentItemChanged);
    connect(m_pCustomizeListView, &QListView::customContextMenuRequested, this, &LeftListView::showMenu);
    connect(m_pMenu, &DMenu::triggered, this, &LeftListView::onMenuClicked);
    connect(m_pAddListBtn, &DPushButton::clicked, this, &LeftListView::onAddListBtnClicked);
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this, &LeftListView::onApplicationHelperThemeTypeChanged);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &LeftListView::onGuiApplicationHelperThemeTypeChanged);
    connect(m_pMountListWidget, &LeftListWidget::sigMousePressIsNoValid, this, &LeftListView::onMousePressIsNoValid);
}

void LeftListView::initUI()
{

    setFrameShape(QFrame::NoFrame);
    setWidgetResizable(true);
    setFixedWidth(180);
    setBackgroundRole(DPalette::Base);
    setAutoFillBackground(true);

    QWidget *widget = new QWidget(this);
    setWidget(widget);
    widget->setFixedWidth(180);
    auto pMainLayout = new QVBoxLayout(widget);
    pMainLayout->setContentsMargins(0, 0, 0, 0);
    pMainLayout->setSpacing(0);

    // 照片库Title
    DWidget *photowidget = new DWidget;
    QHBoxLayout *pPhotoLibLayout = new QHBoxLayout();
    pPhotoLibLayout->setContentsMargins(20, 0, 0, 0);
    photowidget->setLayout(pPhotoLibLayout);
    photowidget->setFixedHeight(40);

    m_pPhotoLibLabel = new DLabel();
    DFontSizeManager::instance()->bind(m_pPhotoLibLabel, DFontSizeManager::T6, QFont::Medium);
    m_pPhotoLibLabel->setForegroundRole(DPalette::TextTips);
    m_pPhotoLibLabel->setText(tr("Gallery"));
    pPhotoLibLayout->addWidget(m_pPhotoLibLabel);
    pPhotoLibLayout->addStretch();
    // 照片库列表
    m_pPhotoLibListView = new LeftListWidget();
    m_pPhotoLibListView->setFocusPolicy(Qt::NoFocus);
    DStyledItemDelegate *itemDelegate0 = new DStyledItemDelegate(m_pPhotoLibListView);
    itemDelegate0->setBackgroundType(DStyledItemDelegate::NoBackground);
    m_pPhotoLibListView->setItemDelegate(itemDelegate0);

    m_pPhotoLibListView->setFixedWidth(LEFT_VIEW_WIDTH_180);
    m_pPhotoLibListView->setFixedHeight(120);
    m_pPhotoLibListView->setSpacing(0);
    m_pPhotoLibListView->setFrameShape(DListWidget::NoFrame);
    m_pPhotoLibListView->setContextMenuPolicy(Qt::CustomContextMenu);

    // 已导入
    QListWidgetItem *pListWidgetItem1 = new QListWidgetItem(m_pPhotoLibListView);
    pListWidgetItem1->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH_160, LEFT_VIEW_LISTITEM_HEIGHT_40));

    AlbumLeftTabItem *pAlbumLeftTabItem1 = new AlbumLeftTabItem(COMMON_STR_RECENT_IMPORTED);
    pAlbumLeftTabItem1->setFixedWidth(LEFT_VIEW_LISTITEM_WIDTH_160);
    pAlbumLeftTabItem1->setFixedHeight(LEFT_VIEW_LISTITEM_HEIGHT_40);

    pAlbumLeftTabItem1->setFocusPolicy(Qt::NoFocus);
    m_pPhotoLibListView->setItemWidget(pListWidgetItem1, pAlbumLeftTabItem1);

    // 最新删除
    QListWidgetItem *pListWidgetItem2 = new QListWidgetItem(m_pPhotoLibListView);
    pListWidgetItem2->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH_160, LEFT_VIEW_LISTITEM_HEIGHT_40));
    AlbumLeftTabItem *pAlbumLeftTabItem2 = new AlbumLeftTabItem(COMMON_STR_TRASH);
    pAlbumLeftTabItem2->setFixedWidth(LEFT_VIEW_LISTITEM_WIDTH_160);
    pAlbumLeftTabItem2->setFixedHeight(LEFT_VIEW_LISTITEM_HEIGHT_40);
    m_pPhotoLibListView->setItemWidget(pListWidgetItem2, pAlbumLeftTabItem2);

    // 我的收藏
    QListWidgetItem *pListWidgetItem3 = new QListWidgetItem(m_pPhotoLibListView);
    pListWidgetItem3->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH_160, LEFT_VIEW_LISTITEM_HEIGHT_40));
    AlbumLeftTabItem *pAlbumLeftTabItem3 = new AlbumLeftTabItem(COMMON_STR_FAVORITES);
    pAlbumLeftTabItem3->setFixedWidth(LEFT_VIEW_LISTITEM_WIDTH_160);
    pAlbumLeftTabItem3->setFixedHeight(LEFT_VIEW_LISTITEM_HEIGHT_40);
    m_pPhotoLibListView->setItemWidget(pListWidgetItem3, pAlbumLeftTabItem3);

    // 相册列表Title
    DWidget *lableCustomixeWidget = new DWidget(this);
    lableCustomixeWidget->setFixedHeight(40);
    QHBoxLayout *pCustomizeLayout = new QHBoxLayout();
    pCustomizeLayout->setContentsMargins(20, 0, 14, 0);
    m_pCustomizeLabel = new DLabel();
    DFontSizeManager::instance()->bind(m_pCustomizeLabel, DFontSizeManager::T6, QFont::Medium);
    m_pCustomizeLabel->setForegroundRole(DPalette::TextTips);
    m_pCustomizeLabel->setText(tr("Albums"));
    m_pAddListBtn = new AlbumImageButton();
    AC_SET_OBJECT_NAME(m_pAddListBtn, Add_Album_Button);
    AC_SET_ACCESSIBLE_NAME(m_pAddListBtn, Add_Album_Button);
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        m_pAddListBtn->setPropertyPic(":/resources/images/sidebar/normal/add_normal.svg",
                                      ":/resources/images/sidebar/active/add_hover.svg",
                                      ":/resources/images/sidebar/active/add_press.svg",
                                      ":/resources/images/sidebar/active/add_focus.svg");
    }
    if (themeType == DGuiApplicationHelper::DarkType) {
        m_pAddListBtn->setPropertyPic(":/resources/images/sidebar/active/add_normal_dark.svg",
                                      ":/resources/images/sidebar/active/add_hover_dark.svg",
                                      ":/resources/images/sidebar/active/add_press_dark.svg",
                                      ":/resources/images/sidebar/active/add_focus_dark.svg");
    }

    m_pAddListBtn->setFixedSize(34, 34);
    m_pAddListBtn->setFocusPolicy(Qt::NoFocus);
    pCustomizeLayout->addWidget(m_pCustomizeLabel, 100, Qt::AlignLeft);
    pCustomizeLayout->addWidget(m_pAddListBtn, 0, Qt::AlignRight | Qt::AlignVCenter);

    // 相册列表
    m_pCustomizeListView = new LeftListWidget();
    m_pCustomizeListView->setFocusPolicy(Qt::NoFocus);
    DStyledItemDelegate *itemDelegate1 = new DStyledItemDelegate(m_pCustomizeListView);
    itemDelegate1->setBackgroundType(DStyledItemDelegate::NoBackground);
    m_pCustomizeListView->setItemDelegate(itemDelegate1);
    m_pCustomizeListView->setSpacing(0);
    m_pCustomizeListView->setFrameShape(DListWidget::NoFrame);
    m_pCustomizeListView->setContextMenuPolicy(Qt::CustomContextMenu);

    QStringList allAlbumNames = DBManager::instance()->getAllAlbumNames();
    for (auto albumName : allAlbumNames) {
        QListWidgetItem *pListWidgetItem = new QListWidgetItem(m_pCustomizeListView, 1);
        pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH_160 /*+ 8*/, LEFT_VIEW_LISTITEM_HEIGHT_40));

        AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(albumName, COMMON_STR_CREATEALBUM);
        pAlbumLeftTabItem->setFixedWidth(LEFT_VIEW_LISTITEM_WIDTH_160 /*+ 8*/);
        pAlbumLeftTabItem->setFixedHeight(LEFT_VIEW_LISTITEM_HEIGHT_40);
        m_pCustomizeListView->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
    }
    // 设备Widget
    QVBoxLayout *pMountVLayout = new QVBoxLayout(this);
    pMountVLayout->setContentsMargins(0, 0, 0, 0);

    // 设备Title
    m_pMountWidget = new DWidget(this);
    m_pMountWidget->setVisible(false);
    m_pMountWidget->setFixedHeight(40);
    QHBoxLayout *pMountLayout = new QHBoxLayout();
    pMountLayout->setContentsMargins(20, 0, 0, 0);
    m_pMountLabel = new DLabel();
    DFontSizeManager::instance()->bind(m_pMountLabel, DFontSizeManager::T6, QFont::Medium);
    m_pMountLabel->setForegroundRole(DPalette::TextTips);
    m_pMountLabel->setText(tr("Device"));
    pMountLayout->addWidget(m_pMountLabel);
    pMountLayout->addStretch();

    // 设备列表
    m_pMountListWidget = new LeftListWidget();
    m_pMountListWidget->setFocusPolicy(Qt::NoFocus);
    m_pMountListWidget->setVisible(false);
    DStyledItemDelegate *itemDelegate2 = new DStyledItemDelegate(m_pMountListWidget);
    itemDelegate2->setBackgroundType(DStyledItemDelegate::NoBackground);
    m_pMountListWidget->setItemDelegate(itemDelegate2);

    m_pMountListWidget->setSpacing(0);
    m_pMountListWidget->setFrameShape(DListWidget::NoFrame);

    pMainLayout->addWidget(photowidget);
    pMainLayout->addWidget(m_pPhotoLibListView);

    m_pMountWidget->setLayout(pMountLayout);
    pMainLayout->addWidget(m_pMountWidget);
    pMainLayout->addWidget(m_pMountListWidget);

    lableCustomixeWidget->setLayout(pCustomizeLayout);
    pMainLayout->addWidget(lableCustomixeWidget);
    pMainLayout->addWidget(m_pCustomizeListView);
    pMainLayout->addStretch();
    onUpdateLeftListview();
}
void LeftListView::updatePhotoListView()
{
    m_pPhotoLibListView->clear();
    // 已导入
    QListWidgetItem *pListWidgetItem1 = new QListWidgetItem(m_pPhotoLibListView);
    pListWidgetItem1->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH_160, LEFT_VIEW_LISTITEM_HEIGHT_40));

    AlbumLeftTabItem *pAlbumLeftTabItem1 = new AlbumLeftTabItem(COMMON_STR_RECENT_IMPORTED);
    pAlbumLeftTabItem1->setFixedWidth(LEFT_VIEW_LISTITEM_WIDTH_160);
    pAlbumLeftTabItem1->setFixedHeight(LEFT_VIEW_LISTITEM_HEIGHT_40);
    m_pPhotoLibListView->setItemWidget(pListWidgetItem1, pAlbumLeftTabItem1);

    // 最新删除
    QListWidgetItem *pListWidgetItem2 = new QListWidgetItem(m_pPhotoLibListView);
    pListWidgetItem2->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH_160, LEFT_VIEW_LISTITEM_HEIGHT_40));

    AlbumLeftTabItem *pAlbumLeftTabItem2 = new AlbumLeftTabItem(COMMON_STR_TRASH);
    pAlbumLeftTabItem2->setFixedWidth(LEFT_VIEW_LISTITEM_WIDTH_160);
    pAlbumLeftTabItem2->setFixedHeight(LEFT_VIEW_LISTITEM_HEIGHT_40);
    m_pPhotoLibListView->setItemWidget(pListWidgetItem2, pAlbumLeftTabItem2);

    // 我的收藏
    QListWidgetItem *pListWidgetItem3 = new QListWidgetItem(m_pPhotoLibListView);
    pListWidgetItem3->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH_160, LEFT_VIEW_LISTITEM_HEIGHT_40));

    AlbumLeftTabItem *pAlbumLeftTabItem3 = new AlbumLeftTabItem(COMMON_STR_FAVORITES);
    pAlbumLeftTabItem3->setFixedWidth(LEFT_VIEW_LISTITEM_WIDTH_160);
    pAlbumLeftTabItem3->setFixedHeight(LEFT_VIEW_LISTITEM_HEIGHT_40);
    m_pPhotoLibListView->setItemWidget(pListWidgetItem3, pAlbumLeftTabItem3);

    m_pPhotoLibListView->setCurrentRow(0);
    QModelIndex index;
    emit m_pPhotoLibListView->pressed(index);
}

void LeftListView::updateCustomizeListView()
{
    m_pCustomizeListView->clear();
    QStringList allAlbumNames = DBManager::instance()->getAllAlbumNames();
    for (auto albumName : allAlbumNames) {
        QListWidgetItem *pListWidgetItem = new QListWidgetItem(m_pCustomizeListView);
        pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH_160, LEFT_VIEW_LISTITEM_HEIGHT_40));

        AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(albumName);
        pAlbumLeftTabItem->setFixedWidth(LEFT_VIEW_LISTITEM_WIDTH_160);
        pAlbumLeftTabItem->setFixedHeight(LEFT_VIEW_LISTITEM_HEIGHT_40);
        m_pCustomizeListView->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
    }
}

void LeftListView::initMenu()
{
    m_pMenu = new DMenu(this);
    m_MenuActionMap.clear();
    appendAction(IdStartSlideShow, tr("Slide show"), ss(""));
    appendAction(IdCreateAlbum, tr("New album"), ss(""));
    appendAction(IdRenameAlbum, tr("Rename"), ss(""));
    m_pMenu->addSeparator();
    appendAction(IdExport, tr("Export"), ss(""));
    appendAction(IdDeleteAlbum, tr("Delete"), ss("THROWTOTRASH_CONTEXT_MENU"));
}

void LeftListView::appendAction(int id, const QString &text, const QString &shortcut)
{
    QAction *ac = new QAction(m_pMenu);
    addAction(ac);
    ac->setText(text);
    ac->setProperty("MenuID", id);
    ac->setShortcut(QKeySequence(shortcut));
    m_MenuActionMap.insert(text, ac);
    m_pMenu->addAction(ac);
}

void LeftListView::showMenu(const QPoint &pos)
{
    if (!m_pCustomizeListView->indexAt(pos).isValid()) {
        return;
    }
    m_pMenu->setVisible(true);
    foreach (QAction *action, m_MenuActionMap.values()) {
        action->setVisible(true);
    }
    if (0 == DBManager::instance()->getImgsCountByAlbum(m_ItemCurrentName)) {
        m_MenuActionMap.value(tr("Slide show"))->setVisible(false);
    }
    if (0 == DBManager::instance()->getImgsCountByAlbum(m_ItemCurrentName)) {
        m_MenuActionMap.value(tr("Export"))->setVisible(false);
    }
    m_pMenu->popup(QCursor::pos());
}

void LeftListView::onMenuClicked(QAction *action)
{
    const int id = action->property("MenuID").toInt();
    switch (MenuItemId(id)) {
    case IdStartSlideShow: {
        auto imagelist = DBManager::instance()->getInfosByAlbum(m_ItemCurrentName);
        QStringList paths;
        for (auto image : imagelist) {
            paths << image.filePath;
        }
        if (paths.length() > 0) {
            const QString path = paths.first();
            emit menuOpenImage(path, paths, true, true);
        }
        break;
    }
    case IdCreateAlbum: {
        QListWidgetItem *pListWidgetItem = new QListWidgetItem();
        pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH_160, LEFT_VIEW_LISTITEM_HEIGHT_40));
        m_pCustomizeListView->insertItem(m_pCustomizeListView->currentRow() + 1, pListWidgetItem);
        AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(getNewAlbumName());
        m_pCustomizeListView->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
        m_pCustomizeListView->setCurrentRow(m_pCustomizeListView->currentRow() + 1);
        AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pCustomizeListView->itemWidget(m_pCustomizeListView->currentItem()));
        item->m_opeMode = OPE_MODE_ADDNEWALBUM;
        item->editAlbumEdit();
        onUpdateLeftListview();
        break;
    }
    case IdRenameAlbum: {
        AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pCustomizeListView->itemWidget(m_pCustomizeListView->currentItem()));
        item->m_opeMode = OPE_MODE_RENAMEALBUM;
        item->editAlbumEdit();
        break;
    }
    case IdExport: {
        Exporter::instance()->exportAlbum(DBManager::instance()->getPathsByAlbum(m_ItemCurrentName), m_ItemCurrentName);
        break;
    }
    case IdDeleteAlbum: {
        QListWidgetItem *item = m_pCustomizeListView->currentItem();
        AlbumLeftTabItem *pTabItem = dynamic_cast<AlbumLeftTabItem *>(m_pCustomizeListView->itemWidget(item));
        deletDialg = new AlbumDeleteDialog;
        connect(deletDialg, &AlbumDeleteDialog::deleteAlbum, this, [ = ]() {
            QString str = pTabItem->m_albumNameStr;
            QStringList paths = DBManager::instance()->getPathsByAlbum(pTabItem->m_albumNameStr);
            ImageEngineApi::instance()->moveImagesToTrash(paths);
            DBManager::instance()->removeAlbum(pTabItem->m_albumNameStr);

            if (1 < m_pCustomizeListView->count()) {
                delete item;
                emit updateCurrentItemType(1);
            } else {
                updateCustomizeListView();
                updatePhotoListView();
                emit updateCurrentItemType(0);
            }
            //移除item后更新右边视图
            //updateRightView();
            emit sigLeftTabClicked();
            onUpdateLeftListview();
            emit dApp->signalM->sigAlbDelToast(str);
        });
        deletDialg->show();
        break;
    }
    }
}

void LeftListView::onUpdateLeftListview()
{
    if (m_pCustomizeListView->count() > 0) {
        m_pCustomizeListView->setFixedHeight(m_pCustomizeListView->count() * LEFT_VIEW_LISTITEM_HEIGHT_40);
    } else {
        m_pCustomizeListView->setFixedHeight(LEFT_VIEW_LISTITEM_HEIGHT_40);
    }
}

void LeftListView::onPhotoLibListViewPressed(const QModelIndex &index)
{
    Q_UNUSED(index);
    qDebug() << "m_pPhotoLibListView, &DListWidget::pressed";
    m_pCustomizeListView->clearSelection();
    m_pMountListWidget->clearSelection();
    updateAlbumItemsColor();
    QListWidgetItem *pitem = m_pPhotoLibListView->currentItem();
    if (pitem) {
        AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pPhotoLibListView->itemWidget(m_pPhotoLibListView->currentItem()));
        auto list = m_pPhotoLibListView->selectedItems();  //当前选中项
        if (QGuiApplication::queryKeyboardModifiers() & Qt::ControlModifier && list.isEmpty()) { //ctrl取消选中
            item->oriAlbumStatus();
        } else {
            item->newAlbumStatus(); //选中状态
        }
        if (COMMON_STR_RECENT_IMPORTED == item->m_albumNameStr) {
            m_ItemCurrentName = COMMON_STR_RECENT_IMPORTED;
            m_ItemCurrentType = COMMON_STR_RECENT_IMPORTED;
        } else if (COMMON_STR_TRASH == item->m_albumNameStr) {
            m_ItemCurrentName = COMMON_STR_TRASH;
            m_ItemCurrentType = COMMON_STR_TRASH;
        } else {
            m_ItemCurrentName = COMMON_STR_FAVORITES;
            m_ItemCurrentType = COMMON_STR_FAVORITES;
        }
        m_ItemCurrentDataType = pitem->type(); //default 0
    }
    m_pCustomizeListView->setFocusPolicy(Qt::NoFocus);
    m_pMountListWidget->setFocusPolicy(Qt::NoFocus);
    m_pPhotoLibListView->setFocus();
    emit itemClicked();
}

void LeftListView::onCustomListViewPressed(const QModelIndex &index)
{
    Q_UNUSED(index);
    qDebug() << "m_pCustomizeListView, &DListWidget::pressed";
    m_pPhotoLibListView->clearSelection();
    m_pMountListWidget->clearSelection();
    updateAlbumItemsColor();
    QListWidgetItem *plitem = m_pCustomizeListView->currentItem();
    if (plitem) {
        AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pCustomizeListView->itemWidget(m_pCustomizeListView->currentItem()));
        auto list = m_pCustomizeListView->selectedItems(); //当前选中项
        if (QGuiApplication::queryKeyboardModifiers() & Qt::ControlModifier && list.isEmpty()) { //ctrl取消选中
            item->oriAlbumStatus();
        } else {
            item->newAlbumStatus();
        }
        m_ItemCurrentName = item->m_albumNameStr;
        m_ItemCurrentDataType = plitem->type(); //default 0
    }
    m_ItemCurrentType = COMMON_STR_CUSTOM;
    m_pPhotoLibListView->setFocusPolicy(Qt::NoFocus);
    m_pMountListWidget->setFocusPolicy(Qt::NoFocus);
    m_pCustomizeListView->setFocus();
    emit itemClicked();
}

void LeftListView::onMountListViewPressed(const QModelIndex &index)
{
    Q_UNUSED(index);
    qDebug() << "zy------LeftListView::onMountListViewPressed m_pMountListWidget size = " << m_pMountListWidget->count();
    m_pPhotoLibListView->clearSelection();
    m_pCustomizeListView->clearSelection();
    updateAlbumItemsColor();
    QListWidgetItem *plitem = m_pMountListWidget->currentItem();
    if (plitem) {
        AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pMountListWidget->itemWidget(m_pMountListWidget->currentItem()));
        if (item) {
            auto list = m_pMountListWidget->selectedItems(); //当前选中项
            if (QGuiApplication::queryKeyboardModifiers() & Qt::ControlModifier) { //ctrl取消选中
                item->oriAlbumStatus();
            } else {
                item->newAlbumStatus();
            }
            m_ItemCurrentName = item->m_albumNameStr;
            m_ItemCurrentDataType = plitem->type(); //default 0
        }
    }
    m_ItemCurrentType = ALBUM_PATHTYPE_BY_PHONE;
    m_pPhotoLibListView->setFocusPolicy(Qt::NoFocus);
    m_pCustomizeListView->setFocusPolicy(Qt::NoFocus);
    m_pMountListWidget->setFocus();
    emit itemClicked();
}

void LeftListView::onPhotoLibListViewCurrentItemChanged()
{
    m_pCustomizeListView->clearSelection();
    m_pMountListWidget->clearSelection();
    updateAlbumItemsColor();
    if (m_pPhotoLibListView->currentItem()) {
        AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pPhotoLibListView->itemWidget(m_pPhotoLibListView->currentItem()));
        item->newAlbumStatus();
        if (COMMON_STR_RECENT_IMPORTED == item->m_albumNameStr) {
            m_ItemCurrentName = COMMON_STR_RECENT_IMPORTED;
            m_ItemCurrentType = COMMON_STR_RECENT_IMPORTED;
        } else if (COMMON_STR_TRASH == item->m_albumNameStr) {
            m_ItemCurrentName = COMMON_STR_TRASH;
            m_ItemCurrentType = COMMON_STR_TRASH;
        } else {
            m_ItemCurrentName = COMMON_STR_FAVORITES;
            m_ItemCurrentType = COMMON_STR_FAVORITES;
        }
    }
}

void LeftListView::onCustomizeListViewCurrentItemChanged()
{
    if (0 < m_pCustomizeListView->count()) {
        m_pPhotoLibListView->clearSelection();
        m_pMountListWidget->clearSelection();
        updateAlbumItemsColor();
        if (m_pCustomizeListView->currentItem()) {
            AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pCustomizeListView->itemWidget(m_pCustomizeListView->currentItem()));
            item->newAlbumStatus();
            m_ItemCurrentName = item->m_albumNameStr;
        }
        m_ItemCurrentType = COMMON_STR_CUSTOM;
    }
}

void LeftListView::onMountListWidgetCurrentItemChanged()
{
    if (0 < m_pMountListWidget->count()) {
        m_pPhotoLibListView->clearSelection();
        m_pCustomizeListView->clearSelection();
        updateAlbumItemsColor();
        if (m_pMountListWidget->currentItem()) {
            AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pMountListWidget->itemWidget(m_pMountListWidget->currentItem()));
            if (nullptr != item) {
                item->newAlbumStatus();
                m_ItemCurrentName = item->m_albumNameStr;
            }
        }
        m_ItemCurrentType = ALBUM_PATHTYPE_BY_PHONE;
    }
}

void LeftListView::onAddListBtnClicked()
{
    emit dApp->signalM->createAlbum(QStringList(" "));
}

void LeftListView::onApplicationHelperThemeTypeChanged()
{
    if (COMMON_STR_RECENT_IMPORTED == m_ItemCurrentType) {
        AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pPhotoLibListView->itemWidget(m_pPhotoLibListView->currentItem()));
        item->newAlbumStatus();
        item->oriAlbumStatus();
    }
    if (COMMON_STR_TRASH == m_ItemCurrentType) {
        AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pPhotoLibListView->itemWidget(m_pPhotoLibListView->currentItem()));
        item->newAlbumStatus();
        item->oriAlbumStatus();
    }
    if (COMMON_STR_FAVORITES == m_ItemCurrentType) {
        AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pPhotoLibListView->itemWidget(m_pPhotoLibListView->currentItem()));
        item->newAlbumStatus();
        item->oriAlbumStatus();
    }
    if (COMMON_STR_CUSTOM == m_ItemCurrentType) {
        AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pCustomizeListView->itemWidget(m_pCustomizeListView->currentItem()));
        item->newAlbumStatus();
        item->oriAlbumStatus();
    }
    if (ALBUM_PATHTYPE_BY_PHONE == m_ItemCurrentType) {
        AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pMountListWidget->itemWidget(m_pMountListWidget->currentItem()));
        item->newAlbumStatus();
        item->oriAlbumStatus();
    }
}

void LeftListView::onGuiApplicationHelperThemeTypeChanged()
{
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        m_pAddListBtn->setPropertyPic(":/resources/images/sidebar/normal/add_normal.svg",
                                      ":/resources/images/sidebar/active/add_hover.svg",
                                      ":/resources/images/sidebar/active/add_press.svg",
                                      ":/resources/images/sidebar/active/add_focus.svg");
    }
    if (themeType == DGuiApplicationHelper::DarkType) {
        m_pAddListBtn->setPropertyPic(":/resources/images/sidebar/active/add_normal_dark.svg",
                                      ":/resources/images/sidebar/active/add_hover_dark.svg",
                                      ":/resources/images/sidebar/active/add_press_dark.svg",
                                      ":/resources/images/sidebar/active/add_focus_dark.svg");
    }
}

void LeftListView::onMousePressIsNoValid()
{
    setFocusPolicy(Qt::ClickFocus);
}



QString LeftListView::getNewAlbumName()
{
    return AlbumCreateDialog::getNewAlbumName("");
}

QString LeftListView::getItemCurrentName()
{
    return m_ItemCurrentName;
}

QString LeftListView::getItemCurrentType()
{
    return m_ItemCurrentType;
}

int LeftListView::getItemDataType()
{
    return  m_ItemCurrentDataType;
}

void LeftListView::updateAlbumItemsColor()
{
    for (int i = 0; i < m_pPhotoLibListView->count(); i++) {
        AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pPhotoLibListView->itemWidget(m_pPhotoLibListView->item(i)));
        item->oriAlbumStatus();
    }

    if (0 < m_pCustomizeListView->count()) {
        for (int i = 0; i < m_pCustomizeListView->count(); i++) {
            AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pCustomizeListView->itemWidget(m_pCustomizeListView->item(i)));
            item->oriAlbumStatus();
        }
    }

    if (0 < m_pMountListWidget->count()) {
        for (int i = 0; i < m_pMountListWidget->count(); i++) {
            AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pMountListWidget->itemWidget(m_pMountListWidget->item(i)));
            if (item)
                item->oriAlbumStatus();
        }
    }
}

void LeftListView::keyPressEvent(QKeyEvent *event)
{
    qDebug() << "LeftListView::keyPressEvent()";
    switch (event->key()) {
    case Qt::Key_Delete:
        if (COMMON_STR_CUSTOM == m_ItemCurrentType) {
            emit sigKeyDelete();
        }
        break;
    case Qt::Key_F2:
        if (COMMON_STR_CUSTOM == m_ItemCurrentType) {
            emit sigKeyF2();
        }
        break;
    default:
        break;
    }
    QWidget::keyPressEvent(event);

}

void LeftListView::resizeEvent(QResizeEvent *e)
{
    DWidget::resizeEvent(e);
    // 设备左边栏
    int deviceHeight = m_pMountListWidget->count() * LEFT_VIEW_LISTITEM_HEIGHT_40;
    m_pMountListWidget->setFixedHeight(deviceHeight);
}
