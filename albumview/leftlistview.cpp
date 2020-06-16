#include "leftlistview.h"
#include "widgets/albumlefttabitem.h"
#include "dbmanager/dbmanager.h"
#include "application.h"
#include "controller/configsetter.h"
#include "utils/baseutils.h"
#include "dbmanager/dbmanager.h"
#include "controller/exporter.h"
#include "imageengine/imageengineapi.h"
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

LeftListView::LeftListView(DWidget *parent)
    : DWidget(parent), m_pPhotoLibLabel(nullptr), m_pPhotoLibListView(nullptr)
    , m_pCustomizeLabel(nullptr), m_pAddListBtn(nullptr), m_pCustomizeListView(nullptr)
    , m_pMountLabel(nullptr), m_pMountListView(nullptr), m_pMenu(nullptr)
    , m_pMountWidget(nullptr)
{
    m_ItemCurrentName = COMMON_STR_RECENT_IMPORTED;
    m_ItemCurrentType = COMMON_STR_RECENT_IMPORTED;
    m_ItemCurrentDataType = 0;
    initUI();
    initMenu();
    initConnections();
}

void LeftListView::initConnections()
{
    connect(m_pPhotoLibListView, &DListWidget::pressed, this, &LeftListView::onMountListView);
    connect(m_pCustomizeListView, &DListWidget::pressed, this, [ = ] {
        qDebug() << "m_pCustomizeListView, &DListWidget::pressed";
        m_pPhotoLibListView->clearSelection();
        m_pMountListView->clearSelection();
        updateAlbumItemsColor();
        QListWidgetItem *plitem = m_pCustomizeListView->currentItem();
        if (plitem)
        {
            AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pCustomizeListView->itemWidget(m_pCustomizeListView->currentItem()));
            auto list = m_pCustomizeListView->selectedItems();  //当前选中项
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
        m_pMountListView->setFocusPolicy(Qt::NoFocus);
        m_pCustomizeListView->setFocus();
        emit itemClicked();
    });

    connect(m_pMountListView, &DListWidget::pressed, this, [ = ] {
        qDebug() << "m_pMountListView, &DListWidget::pressed";
        m_pPhotoLibListView->clearSelection();
        m_pCustomizeListView->clearSelection();
        updateAlbumItemsColor();

        QListWidgetItem *plitem = m_pMountListView->currentItem();
        if (plitem)
        {
            AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pMountListView->itemWidget(m_pMountListView->currentItem()));
            auto list = m_pMountListView->selectedItems();  //当前选中项
            if (QGuiApplication::queryKeyboardModifiers() & Qt::ControlModifier && list.isEmpty()) { //ctrl取消选中
                item->oriAlbumStatus();
            } else {
                item->newAlbumStatus();
            }
            m_ItemCurrentName = item->m_albumNameStr;
            m_ItemCurrentDataType = plitem->type(); //default 0
        }

        m_ItemCurrentType = ALBUM_PATHTYPE_BY_PHONE;
        m_pPhotoLibListView->setFocusPolicy(Qt::NoFocus);
        m_pCustomizeListView->setFocusPolicy(Qt::NoFocus);

        m_pMountListView->setFocus();

        emit itemClicked();
    });

    connect(m_pPhotoLibListView, &DListWidget::currentItemChanged, this, [ = ] {
        m_pCustomizeListView->clearSelection();
        m_pMountListView->clearSelection();
        updateAlbumItemsColor();

        if (m_pPhotoLibListView->currentItem())
        {
            AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pPhotoLibListView->itemWidget(m_pPhotoLibListView->currentItem()));
            auto list = m_pMountListView->selectedItems(); //当前选中项
            // 仍有优化空间
            if (QGuiApplication::queryKeyboardModifiers() & Qt::ControlModifier && list.isEmpty()) { //ctrl取消选中
                item->oriAlbumStatus();
            } else {
                item->newAlbumStatus();
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
        }
    });

    connect(m_pCustomizeListView, &DListWidget::currentItemChanged, this, [ = ] {
        qDebug() << "m_pCustomizeListView, &DListWidget::currentItemChanged";
        if (0 < m_pCustomizeListView->count())
        {
            m_pPhotoLibListView->clearSelection();
            m_pMountListView->clearSelection();
            updateAlbumItemsColor();

            if (m_pCustomizeListView->currentItem()) {
                AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pCustomizeListView->itemWidget(m_pCustomizeListView->currentItem()));
                item->newAlbumStatus();
                m_ItemCurrentName = item->m_albumNameStr;
            }
            m_ItemCurrentType = COMMON_STR_CUSTOM;

//            emit itemClicked();
        }
    });

    connect(m_pMountListView, &DListWidget::currentItemChanged, this, [ = ] {
        if (0 < m_pMountListView->count())
        {
            m_pPhotoLibListView->clearSelection();
            m_pCustomizeListView->clearSelection();
            updateAlbumItemsColor();

            if (m_pMountListView->currentItem()) {
                AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pMountListView->itemWidget(m_pMountListView->currentItem()));
                if (nullptr != item) {
                    item->newAlbumStatus();
                    m_ItemCurrentName = item->m_albumNameStr;
                }
            }

            m_ItemCurrentType = ALBUM_PATHTYPE_BY_PHONE;
            //emit itemClicked();
        }
    });

    connect(m_pCustomizeListView, &QListView::customContextMenuRequested, this, &LeftListView::showMenu);
    connect(m_pMenu, &DMenu::triggered, this, &LeftListView::onMenuClicked);
    connect(m_pAddListBtn, &DPushButton::clicked, this, [ = ] {
        emit dApp->signalM->createAlbum(QStringList(" "));
    });
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this, [ = ] {
        if (COMMON_STR_RECENT_IMPORTED == m_ItemCurrentType)
        {
            AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pPhotoLibListView->itemWidget(m_pPhotoLibListView->currentItem()));
            item->newAlbumStatus();
            item->oriAlbumStatus();
        }
        if (COMMON_STR_TRASH == m_ItemCurrentType)
        {
            AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pPhotoLibListView->itemWidget(m_pPhotoLibListView->currentItem()));
            item->newAlbumStatus();
            item->oriAlbumStatus();
        }
        if (COMMON_STR_FAVORITES == m_ItemCurrentType)
        {
            AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pPhotoLibListView->itemWidget(m_pPhotoLibListView->currentItem()));
            item->newAlbumStatus();
            item->oriAlbumStatus();
        }
        if (COMMON_STR_CUSTOM == m_ItemCurrentType)
        {
            AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pCustomizeListView->itemWidget(m_pCustomizeListView->currentItem()));
            item->newAlbumStatus();
            item->oriAlbumStatus();
        }
        if (ALBUM_PATHTYPE_BY_PHONE == m_ItemCurrentType)
        {
            AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pMountListView->itemWidget(m_pMountListView->currentItem()));
            item->newAlbumStatus();
            item->oriAlbumStatus();
        }
    });
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [ = ] {
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType)
        {
            m_pAddListBtn->setPropertyPic(":/resources/images/sidebar/normal/add_normal.svg",
                                          ":/resources/images/sidebar/active/add_hover.svg",
                                          ":/resources/images/sidebar/active/add_press.svg",
                                          ":/resources/images/sidebar/active/add_focus.svg");
        }
        if (themeType == DGuiApplicationHelper::DarkType)
        {
            m_pAddListBtn->setPropertyPic(":/resources/images/sidebar/active/add_normal_dark.svg",
                                          ":/resources/images/sidebar/active/add_hover_dark.svg",
                                          ":/resources/images/sidebar/active/add_press_dark.svg",
                                          ":/resources/images/sidebar/active/add_focus_dark.svg");
        }
    });

    connect(m_pMountListView, &LeftListWidget::sigMousePressIsNoValid, this, [ = ] {
        setFocusPolicy(Qt::ClickFocus);
    });

    connect(SignalManager::instance(), &SignalManager::updateLeftListview, this, &LeftListView::onUpdateLeftListview);
}

void LeftListView::initUI()
{
    setFixedWidth(180);
    setBackgroundRole(DPalette::Base);
    setAutoFillBackground(true);

    QVBoxLayout *pMainLayout = new QVBoxLayout();
    pMainLayout->setContentsMargins(0, 0, 0, 0);
    //pMainLayout->setSpacing(0);

    // 照片库Title
    DWidget *photowidget = new DWidget;
    QHBoxLayout *pPhotoLibLayout = new QHBoxLayout();
    pPhotoLibLayout->setContentsMargins(20, 0, 0, 0);
    photowidget->setLayout(pPhotoLibLayout);
    photowidget->setFixedHeight(40);

    m_pPhotoLibLabel = new DLabel();
    //m_pPhotoLibLabel->setFixedHeight(40);
    DFontSizeManager::instance()->bind(m_pPhotoLibLabel, DFontSizeManager::T6, QFont::Medium);
    m_pPhotoLibLabel->setForegroundRole(DPalette::TextTips);
    m_pPhotoLibLabel->setText(tr("Gallery"));

    //pPhotoLibLayout->addSpacing(14);
    pPhotoLibLayout->addWidget(m_pPhotoLibLabel);

    // 照片库列表
    m_pPhotoLibListView = new LeftListWidget();
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
//    pAlbumLeftTabItem2->setFocusPolicy(Qt::NoFocus);
    m_pPhotoLibListView->setItemWidget(pListWidgetItem2, pAlbumLeftTabItem2);

    // 我的收藏
    QListWidgetItem *pListWidgetItem3 = new QListWidgetItem(m_pPhotoLibListView);
    pListWidgetItem3->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH_160, LEFT_VIEW_LISTITEM_HEIGHT_40));

    AlbumLeftTabItem *pAlbumLeftTabItem3 = new AlbumLeftTabItem(COMMON_STR_FAVORITES);
    pAlbumLeftTabItem3->setFixedWidth(LEFT_VIEW_LISTITEM_WIDTH_160);
    pAlbumLeftTabItem3->setFixedHeight(LEFT_VIEW_LISTITEM_HEIGHT_40);
//    pAlbumLeftTabItem3->setFocusPolicy(Qt::NoFocus);
    m_pPhotoLibListView->setItemWidget(pListWidgetItem3, pAlbumLeftTabItem3);

    // 相册列表Title
    DWidget *lableCustomixeWidget = new DWidget(this);
    lableCustomixeWidget->setFixedHeight(40);

    QHBoxLayout *pCustomizeLayout = new QHBoxLayout();
    pCustomizeLayout->setContentsMargins(20, 0, 14, 0);

    m_pCustomizeLabel = new DLabel();
    //m_pCustomizeLabel->setFixedHeight(40);
    DFontSizeManager::instance()->bind(m_pCustomizeLabel, DFontSizeManager::T6, QFont::Medium);
    m_pCustomizeLabel->setForegroundRole(DPalette::TextTips);
    m_pCustomizeLabel->setText(tr("Albums"));
    m_pAddListBtn = new AlbumImageButton();
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

    //pCustomizeLayout->addSpacing(14);
    pCustomizeLayout->addWidget(m_pCustomizeLabel, 100, Qt::AlignLeft);
//    pCustomizeLayout->addStretch();
    pCustomizeLayout->addWidget(m_pAddListBtn, 0, Qt::AlignRight | Qt::AlignVCenter);

    // 相册列表
    m_pCustomizeListView = new LeftListWidget();
    DStyledItemDelegate *itemDelegate1 = new DStyledItemDelegate(m_pCustomizeListView);
    itemDelegate1->setBackgroundType(DStyledItemDelegate::NoBackground);
    m_pCustomizeListView->setItemDelegate(itemDelegate1);

    m_pCustomizeListView->setFixedWidth(LEFT_VIEW_WIDTH_180);
//    m_pCustomizeListView->setFixedHeight(400);
    m_pCustomizeListView->setMinimumHeight(40);

    m_pCustomizeListView->setMaximumHeight(400);

//    m_pCustomizeListView->setMinimumHeight(400);
//    emit SignalManager::instance()->updateLeftListview();
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
    m_pMountWidget = new DWidget(this);
    m_pMountWidget->setMaximumHeight(200);
//    m_pMountWidget->setFixedHeight(200);

    QVBoxLayout *pMountVLayout = new QVBoxLayout();
    pMountVLayout->setContentsMargins(0, 0, 0, 0);

    // 设备Title
    DWidget *pMountWidget = new DWidget(this);
    pMountWidget->setFixedHeight(40);

    QHBoxLayout *pMountLayout = new QHBoxLayout();
    pMountLayout->setContentsMargins(20, 0, 0, 0);

    m_pMountLabel = new DLabel();
//    m_pMountLabel->setFixedHeight(40);
    DFontSizeManager::instance()->bind(m_pMountLabel, DFontSizeManager::T6, QFont::Medium);
    m_pMountLabel->setForegroundRole(DPalette::TextTips);
    m_pMountLabel->setText(tr("Device"));

    //pMountLayout->addSpacing(14);
    pMountLayout->addWidget(m_pMountLabel);
    pMountLayout->addStretch();

    // 线
//    QHBoxLayout *pLineLayout2 = new QHBoxLayout();
//    pLineLayout2->setContentsMargins(0, 0, 0, 0);

//    DLabel *pLineLabel2 = new DLabel();
//    pLineLabel2->setPixmap(QPixmap(":/resources/images/sidebar/sidebar_line_normal.svg"));

//    pLineLayout2->addSpacing(14);
//    pLineLayout2->addWidget(pLineLabel2);

    // 设备列表
    m_pMountListView = new LeftListWidget();
    //    m_pPhotoLibListView->setViewportMargins(8, 0, 8, 0);
    DStyledItemDelegate *itemDelegate2 = new DStyledItemDelegate(m_pMountListView);
    itemDelegate2->setBackgroundType(DStyledItemDelegate::NoBackground);
    m_pMountListView->setItemDelegate(itemDelegate2);

    m_pMountListView->setFixedWidth(LEFT_VIEW_WIDTH_180);
    m_pMountListView->setMaximumHeight(200);
    m_pMountListView->setSpacing(0);
    m_pMountListView->setFrameShape(DListWidget::NoFrame);

    // 添加layout/widget
//    pMainLayout->addLayout(pPhotoLibLayout);
    pMainLayout->addWidget(photowidget);
//    pMainLayout->addLayout(pLineLayout);
    pMainLayout->addWidget(m_pPhotoLibListView);
//    pMainLayout->addStretch();
    lableCustomixeWidget->setLayout(pCustomizeLayout);
    pMainLayout->addWidget(lableCustomixeWidget);
//    pMainLayout->addStretch();
//    pMainLayout->addLayout(pLineLayout1);
    pMainLayout->addWidget(m_pCustomizeListView);
//    pMainLayout->addStretch();
    pMountWidget->setLayout(pMountLayout);
    pMainLayout->addWidget(pMountWidget);
//    pMainLayout->addLayout(pLineLayout2);
    pMainLayout->addWidget(m_pMountListView);
    pMainLayout->addStretch();
    setLayout(pMainLayout);

//    pMountVLayout->addLayout(pMountLayout);
//    pMountVLayout->addWidget(m_pMountListView);
////    pMountVLayout->addStretch();
//    m_pMountWidget->setLayout(pMountVLayout);

    moveMountListWidget();
//    m_pMountWidget->raise();
//    m_pMountWidget->show();
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

void LeftListView::updateMountListView()
{

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

        const QString path = paths.first();

        emit menuOpenImage(path, paths, true, true);
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

        moveMountListWidget();
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
            } else {
                updateCustomizeListView();
                updatePhotoListView();
            }
            //移除item后更新右边视图
            //updateRightView();
            moveMountListWidget();
            emit dApp->signalM->sigAlbDelToast(str);
        });
        deletDialg->show();
        break;
    }
    }
}

void LeftListView::onUpdateLeftListview()
{
    if (m_pCustomizeListView->count() <= 13) {
        m_pCustomizeListView->setMaximumHeight(m_pCustomizeListView->count() * LEFT_VIEW_LISTITEM_HEIGHT_40);
    }
}

void LeftListView::onMountListView(QModelIndex index)
{
    Q_UNUSED(index);
    qDebug() << "m_pPhotoLibListView, &DListWidget::pressed";
    m_pCustomizeListView->clearSelection();
    m_pMountListView->clearSelection();
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
    m_pMountListView->setFocusPolicy(Qt::NoFocus);
    m_pPhotoLibListView->setFocus();

    emit itemClicked();
}

QString LeftListView::getNewAlbumName()
{
    const QString nan = tr("Unnamed");
    int num = 1;
    QString albumName = nan + QString::number(num);
    while (DBManager::instance()->isAlbumExistInDB(albumName)) {
        num++;
        albumName = nan + QString::number(num);
    }
    return albumName;
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

    if (0 < m_pMountListView->count()) {
        for (int i = 0; i < m_pMountListView->count(); i++) {
            AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pMountListView->itemWidget(m_pMountListView->item(i)));
            if (item)
                item->oriAlbumStatus();
        }
    }
}

void LeftListView::keyPressEvent(QKeyEvent *event)
{
    qDebug() << "LeftListView::keyPressEvent()";
//    if (event->key() == Qt::Key_Delete) {
//        if (COMMON_STR_CUSTOM == m_ItemCurrentType) {
//            emit sigKeyDelete();
//        }
//    } else if (event->key() == Qt::Key_F2) {
//        if (COMMON_STR_CUSTOM == m_ItemCurrentType) {
//            emit sigKeyF2();
//        }
//    }
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
    int height = this->height() - 240 - 120;
    int reheight = 0;
    if (m_pCustomizeListView->count() <= 10) {
        reheight = m_pCustomizeListView->count() * LEFT_VIEW_LISTITEM_HEIGHT_40;
    } else {
        reheight = 10 * LEFT_VIEW_LISTITEM_HEIGHT_40;
    }
    if (height > reheight)
        height = reheight;
    m_pCustomizeListView->setFixedHeight(height);
}

void LeftListView::paintEvent(QPaintEvent *event)
{
    DWidget::paintEvent(event);
    int height = this->height() - 240 - 120;
    int reheight = 0;
    if (m_pCustomizeListView->count() <= 10) {
        reheight = m_pCustomizeListView->count() * LEFT_VIEW_LISTITEM_HEIGHT_40;
    } else {
        reheight = 10 * LEFT_VIEW_LISTITEM_HEIGHT_40;
    }
    if (height > reheight)
        height = reheight;
    m_pCustomizeListView->setFixedHeight(height);
}

void LeftListView::moveMountListWidget()
{
    int iMountY = 200;

    if (11 > m_pCustomizeListView->count()) {
        iMountY = iMountY + m_pCustomizeListView->count() * 40;
    } else {
        iMountY = iMountY + 10 * 40;
    }

    emit SignalManager::instance()->updateLeftListview();
    m_pMountWidget->move(0, iMountY);

}
