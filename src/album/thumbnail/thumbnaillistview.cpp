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
#include "thumbnaillistview.h"
#include "application.h"

#include <QDebug>
#include <QDrag>
#include <QFileInfo>
#include <QImageReader>
#include <QMimeData>
#include <QScrollBar>
#include <QMutex>
#include <QScroller>
#include <QPropertyAnimation>

#include "controller/signalmanager.h"
#include "controller/wallpapersetter.h"
#include "widgets/dialogs/imgdeletedialog.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "utils/unionimage.h"
#include "imageengine/imageengineapi.h"
#include "imageengine/imageenginethread.h"
#include "ac-desktop-define.h"
#include "timelinedatewidget.h"
#include "allpicview/allpicview.h"
#include "imagedataservice.h"

namespace {
const int ITEM_SPACING = 4;
const int BASE_HEIGHT = 100;
const int ANIMATION_DRLAY = 50;

// const QString IMAGE_DEFAULTTYPE = "All pics";
const QString IMAGE_DEFAULTTYPE = "All Photos";
const QString SHORTCUTVIEW_GROUP = "SHORTCUTVIEW";

using namespace utils::common;

QString ss(const QString &text)
{
    QString str = dApp->setter->value(SHORTCUTVIEW_GROUP, text).toString();
    str.replace(" ", "");

    return str;
}
}  // namespace

ThumbnailListView::ThumbnailListView(ThumbnailDelegate::DelegateType type, const  QString &imgtype, QWidget *parent)
    :  DListView(parent), m_delegatetype(type), m_allfileslist(), updateEnableSelectionByMouseTimer(nullptr)
{
    m_model = new QStandardItemModel(this);
    m_imageType = imgtype;
    m_iBaseHeight = BASE_HEIGHT;
    m_albumMenu = nullptr;
    setResizeMode(QListView::Adjust);
    setViewMode(QListView::IconMode);
    setSpacing(ITEM_SPACING);
    setDragEnabled(false);
    //按照像素滚动，步进20
    setVerticalScrollMode(QListView::ScrollPerPixel);
    verticalScrollBar()->setSingleStep(20);

    setContextMenuPolicy(Qt::CustomContextMenu);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setMinimumWidth(500);
    m_delegate = new ThumbnailDelegate(type, this); //绑在this上面，用于自动销毁
    m_delegate->m_imageTypeStr = m_imageType;
    setItemDelegate(m_delegate);
    setModel(m_model);
    m_pMenu = new DMenu();
//    setViewportMargins(0, 0, 0, 0);
    initMenuAction();
    initConnections();
    installEventFilter(this);
    verticalScrollBar()->installEventFilter(this);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
            this, &ThumbnailListView::sltChangeDamagedPixOnThemeChanged);

    updateEnableSelectionByMouseTimer = new QTimer(this);
    updateEnableSelectionByMouseTimer->setInterval(500);
    connect(updateEnableSelectionByMouseTimer, &QTimer::timeout, [ = ]() {
        if (touchStatus == 0) { //时间到了还在等待模式,则进入框选模式
            touchStatus = 2;
        }
        updateEnableSelectionByMouseTimer->stop();
    });

    m_scrollTimer = new QTimer(this);
    m_scrollTimer->setInterval(100);
    m_scrollTimer->setSingleShot(true);
    connect(m_scrollTimer, &QTimer::timeout, this, &ThumbnailListView::onScrollTimerOut);
}

ThumbnailListView::~ThumbnailListView()
{
}

static QString myMimeType()
{
    return QStringLiteral("TestListView/text-icon-icon_hover");
}

void ThumbnailListView::mousePressEvent(QMouseEvent *event)
{
    //点击时将焦点设置到当前
    setFocus();
    //复位激活click
    activeClick = true;
    // 当事件source为MouseEventSynthesizedByQt，认为此事件为TouchBegin转换而来
    if (event->source() == Qt::MouseEventSynthesizedByQt) {
        lastTouchBeginPos = event->pos();

        // 清空触屏滚动操作，因为在鼠标按下时还不知道即将进行的是触屏滚动还是文件框选
        if (QScroller::hasScroller(this)) {
            // 不可使用 ungrab，会导致应用崩溃，或许是Qt的bug
            QScroller::scroller(this)->deleteLater();
        }

        updateEnableSelectionByMouseTimer->start();
    }

    if ((QApplication::keyboardModifiers() == Qt::ShiftModifier && event->button() == Qt::LeftButton)) {
        // 最近删除界面单指循环选中、取消选中
        if (selectionModel()->selectedIndexes().contains(this->indexAt(event->pos()))
                && selectionModel()->selectedIndexes().size() == 1 && m_imageType == COMMON_STR_TRASH) {
            if (event->button() != Qt::MouseButton::RightButton) { // 优化最近删除界面,长按时不清除选中
                clearSelection();
            }
        }
#ifdef tablet_PC
        else if (m_isSelectAllBtn && (m_imageType == COMMON_STR_VIEW_TIMELINE || m_imageType == COMMON_STR_RECENT_IMPORTED)) {
            QModelIndexList list = selectionModel()->selectedIndexes();
            DListView::mousePressEvent(event);
            for (auto item : list) {
                selectionModel()->select(item, QItemSelectionModel::Select);
            }
            if (list.contains(this->indexAt(event->pos())) && event->button() != Qt::RightButton) {
                selectionModel()->select(this->indexAt(event->pos()), QItemSelectionModel::Deselect);
            } else {
                selectionModel()->select(this->indexAt(event->pos()), QItemSelectionModel::Select);
            }
        }
#endif
    }
    QModelIndex index = this->indexAt(event->pos());
    DBImgInfo info = index.data(Qt::DisplayRole).value<DBImgInfo>();
    if ((m_imageType != COMMON_STR_VIEW_TIMELINE)
            && (m_delegatetype != ThumbnailDelegate::AllPicViewType)
            && (m_imageType != COMMON_STR_TRASH)
            && (m_imageType != ALBUM_PATHTYPE_BY_PHONE)) {
        if (info.itemType == ItemTypeImportTimeLineTitle) {
            //已导入时间线标题不可拖动
            setDragEnabled(false);
        } else if (dragDropMode() != NoDragDrop) {
            setDragDropMode(DragDrop);
        }
    } else {
        setDragEnabled(false);
    }

    DListView::mousePressEvent(event);
}

void ThumbnailListView::mouseMoveEvent(QMouseEvent *event)
{
    if (touchStatus == 0) {
        QRectF rect(QPointF((lastTouchBeginPos.x() - 30), (lastTouchBeginPos.y() - 30)),
                    QPointF((lastTouchBeginPos.x() + 30), (lastTouchBeginPos.y() + 30)));
        if (rect.contains(event->pos())) {
            return;
        } else {
            touchStatus = 1;
            activeClick = false;
        }
    }

    if (touchStatus == 1) {
        if (event->source() == Qt::MouseEventSynthesizedByQt) {
            //todo
//            emit sigNeedMoveScorll(-(event->pos() - lastTouchBeginPos).y());
        }
    }

    emit sigMouseMove();
    DListView::mouseMoveEvent(event);
}

void ThumbnailListView::startDrag(Qt::DropActions supportedActions)
{
    Q_UNUSED(supportedActions);
    QString text = "xxxxxxxxxxxxxx";
    QIcon icon = QIcon(":/resources/images/other/deepin-album.svg");
    QIcon icon_hover = QIcon(":/resources/images/other/deepin-album.svg");
    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << text << icon << icon_hover;
    QMimeData *mimeData = new QMimeData;
    mimeData->setData(myMimeType(), itemData);
    QDrag *pDrag = new QDrag(this);
    QPixmap p = QPixmap(":/resources/images/other/deepin-album.svg");
    pDrag->setMimeData(mimeData);
    pDrag->setPixmap(p);
    pDrag->exec(Qt::MoveAction);
}

void ThumbnailListView::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    //时间线使用
    emit sigShowEvent();
    //info.image = getDamagedPixmap();
    int i_totalwidth = width() - 30;
    //计算一行的个数
    m_rowSizeHint = i_totalwidth / (m_iBaseHeight + ITEM_SPACING);
    int currentwidth = (i_totalwidth - ITEM_SPACING * (m_rowSizeHint - 1)) / m_rowSizeHint;//一张图的宽度
    m_onePicWidth = currentwidth;
    if (currentwidth < 80)
        currentwidth = 80;
    int size = ImageEngineApi::instance()->m_AllImageDataVector.size();
    //出于性能考虑,因为一开始只加了第一屛，所以显示后延迟加载剩下的图片信息,m_bfirstload用来只执行一次
    if (m_delegatetype == ThumbnailDelegate::AllPicViewType && m_model->rowCount() < size && m_bfirstload) {
        QTimer::singleShot(200, this, [ = ]() {
            //m_model->rowCount()减一是为了减去顶部空白栏占用的一个ModelIndex
            int i = m_model->rowCount() - 1 < 0 ? 0 : m_model->rowCount() - 1;
            for (; i < size; i++) {
                ImageDataSt data = ImageEngineApi::instance()->m_AllImageDataVector[i];
                DBImgInfo info = data.dbi;
                if (data.imgpixmap.isNull()) {
                    info.bNotSupportedOrDamaged = true;
                    info.damagedPixmap = getDamagedPixmap();
                }
                info.image = data.imgpixmap;
                ImageEngineApi::instance()->m_AllImageMap[info.filePath] = info.image;
                info.remainDays = data.remainDays;
                info.imgWidth = m_onePicWidth;
                info.imgHeight = m_onePicWidth;
                insertThumbnail(info);
            }
            m_bfirstload = false;
            //加载完后通知所有图片页刷新状态栏显示数量
            emit sigUpdatePicNum();
        });
    }
}

void ThumbnailListView::wheelEvent(QWheelEvent *event)
{
    if (event->delta() < 0) {
        //向下滑时才可执行
        m_animationEnable = false;
    } else {
        m_animationEnable = true;
    }
    DListView::wheelEvent(event);
}

void ThumbnailListView::mouseReleaseEvent(QMouseEvent *event)
{
    //触屏状态复位
    touchStatus = 0;
    updateEnableSelectionByMouseTimer->stop();
    m_animationEnable = false;

    if (COMMON_STR_RECENT_IMPORTED  == m_imageType) {
        if (QApplication::keyboardModifiers() == Qt::NoModifier) {
            emit sigMouseRelease();
        }
    } else {
        emit sigMouseRelease();
    }

    DListView::mouseReleaseEvent(event);
    updatetimeLimeBtnText();
}

void ThumbnailListView::keyPressEvent(QKeyEvent *event)
{
    DListView::keyPressEvent(event);
    if ((event->modifiers() == Qt::ControlModifier) && (event->key() == Qt::Key_A)) {
        TimeLineSelectAllBtn();
        emit sigSelectAll();
    }
    m_dragItemPath = selectedPaths();
    if (event->key() == Qt::Key_Period) {
        if (m_dragItemPath.empty()) {
            return;
        }
        if (!DBManager::instance()->isImgExistInAlbum(COMMON_STR_FAVORITES, m_dragItemPath.first(), AlbumDBType::Favourite)) {
            DBManager::instance()->insertIntoAlbum(COMMON_STR_FAVORITES, QStringList(m_dragItemPath.first()), AlbumDBType::Favourite);
            emit dApp->signalM->insertedIntoAlbum(COMMON_STR_FAVORITES, QStringList(m_dragItemPath.first()));
        } else {
            DBManager::instance()->removeFromAlbum(COMMON_STR_FAVORITES, QStringList(m_dragItemPath.first()), AlbumDBType::Favourite);
        }
    }
}

void ThumbnailListView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(myMimeType())) {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            const QMimeData *mimeData = event->mimeData();
            if (!utils::base::checkMimeData(mimeData)) {
                return;
            }
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void ThumbnailListView::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat(myMimeType())) {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            const QMimeData *mimeData = event->mimeData();
            if (!utils::base::checkMimeData(mimeData)) {
                return;
            }
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void ThumbnailListView::dragLeaveEvent(QDragLeaveEvent *event)
{
    m_dragItemPath = selectedPaths();
    DListView::dragLeaveEvent(event);
}

void ThumbnailListView::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat("TestListView/text-icon-icon_hover"))
        return;
    DListView::dropEvent(event);
}

void ThumbnailListView::initConnections()
{
    connect(dApp->signalM, &SignalManager::sigSyncListviewModelData, this, &ThumbnailListView::onSyncListviewModelData);
    //有图片删除后，刷新列表
    connect(dApp->signalM, &SignalManager::imagesRemovedPar, this, &ThumbnailListView::updateThumbnailViewAfterDelete);
    connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, &ThumbnailListView::onScrollbarValueChanged);
    connect(this, &QListView::customContextMenuRequested, this, &ThumbnailListView::onShowMenu);
    connect(m_pMenu, &DMenu::triggered, this, &ThumbnailListView::onMenuItemClicked);
    connect(this, &ThumbnailListView::doubleClicked, this, &ThumbnailListView::onDoubleClicked);
    connect(this, &ThumbnailListView::clicked, this, &ThumbnailListView::onClicked);
    connect(dApp->signalM, &SignalManager::sigMainwindowSliderValueChg, this, &ThumbnailListView::onPixMapScale);
    connect(m_delegate, &ThumbnailDelegate::sigCancelFavorite, this, &ThumbnailListView::onCancelFavorite);

    connect(ImageEngineApi::instance(), &ImageEngineApi::sigOneImgReady, this, &ThumbnailListView::slotOneImgReady);
}

void ThumbnailListView::updateThumbnailView(QString updatePath)
{
    for (int i = 0; i < m_model->rowCount(); i++) {
        QModelIndex index = m_model->index(i, 0);
        DBImgInfo info = index.data(Qt::DisplayRole).value<DBImgInfo>();
        if (info.filePath == updatePath) {  //需要旋转的图片
            ImageDataSt data;
            ImageEngineApi::instance()->getImageData(updatePath, data);
            if (data.imgpixmap.isNull()) {
                info.bNotSupportedOrDamaged = true;
                info.damagedPixmap = getDamagedPixmap();
            }
            info.fileName = data.dbi.fileName;
            info.filePath = data.dbi.filePath;
            info.image = data.imgpixmap;
            info.remainDays = data.remainDays;
            info.itemType = data.dbi.itemType;
            info.videoDuration = data.dbi.videoDuration;

            QVariant infoVariant;
            infoVariant.setValue(info);

            m_model->setData(index, infoVariant, Qt::DisplayRole);
            m_model->setData(index, QVariant(QSize(info.imgWidth, info.imgWidth)), Qt::SizeHintRole);
            QStringList albumNames = ImageEngineApi::instance()->getImgPathAndAlbumNames().values(info.filePath);
            m_model->setData(index, QVariant(albumNames), Qt::UserRole + 2);
        }
    }
    this->setSpacing(ITEM_SPACING);
}

void ThumbnailListView::insertThumbnail(const DBImgInfo &dBImgInfo)
{
    DBImgInfo info = dBImgInfo;
    cutPixmap(info);

    QStandardItem *item = new QStandardItem;
    QVariant infoVariant;
    int height = info.imgHeight;
    if (info.itemType == ItemType::ItemTypeBlank
            || info.itemType == ItemType::ItemTypeTimeLineTitle
            || info.itemType == ItemType::ItemTypeImportTimeLineTitle) {
        info.imgWidth = this->width() - 5;
    }
    infoVariant.setValue(info);
    item->setData(infoVariant, Qt::DisplayRole);
    QStringList albumNames = ImageEngineApi::instance()->getImgPathAndAlbumNames().values(info.filePath);
    item->setData(QVariant(albumNames), Qt::UserRole + 2);
    item->setData(QVariant(QSize(info.imgWidth, /*gridItem[i][j].height*/height)),
                  Qt::SizeHintRole);
    m_model->appendRow(item);
    if (info.itemType == ItemTypeTimeLineTitle) {
        QModelIndex index = m_model->indexFromItem(item);
        TimeLineDateWidget *pCurrentDateWidget = new TimeLineDateWidget(item, info.date, info.num);
        connect(pCurrentDateWidget, &TimeLineDateWidget::sigIsSelectCurrentDatePic, this, &ThumbnailListView::slotSelectCurrentDatePic);
        this->setIndexWidget(index, pCurrentDateWidget);
    } else if (info.itemType == ItemTypeImportTimeLineTitle) {
        QModelIndex index = m_model->indexFromItem(item);
        importTimeLineDateWidget *pCurrentDateWidget = new importTimeLineDateWidget(item, info.date, info.num);
        connect(pCurrentDateWidget, &importTimeLineDateWidget::sigIsSelectCurrentDatePic, this, &ThumbnailListView::slotSelectCurrentDatePic);
        this->setIndexWidget(index, pCurrentDateWidget);
    }
}

void ThumbnailListView::stopLoadAndClear(bool bClearModel)
{
    if (bClearModel)
        m_model->clear();   //清除模型中的数据
    m_allfileslist.clear();
}
//根据显示类型，返回不同列表，如所有照片返回所有，时间线返回当前时间下内容
QStringList ThumbnailListView::getFileList(int row)
{
    m_allfileslist.clear();
    if (m_delegatetype == ThumbnailDelegate::AllPicViewType
            || m_delegatetype == ThumbnailDelegate::AlbumViewCustomType
            || m_delegatetype == ThumbnailDelegate::AlbumViewFavoriteType
            || m_delegatetype == ThumbnailDelegate::SearchViewType) {
        //遍历所有数据
        for (int i = 0; i < m_model->rowCount(); i++) {
            QModelIndex index = m_model->index(i, 0);
            DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
            if (data.itemType == ItemType::ItemTypePic) {
                m_allfileslist.append(data.filePath);
            }
        }
    } else if (m_delegatetype == ThumbnailDelegate::TimeLineViewType
               || m_delegatetype == ThumbnailDelegate::AlbumViewImportTimeLineViewType) {
        //找到当前项前面最近的一个标题
        QModelIndex titleIndex;
        for (int i = row; i >= 0; i--) {
            QModelIndex itemIndex = m_model->index(i, 0);
            DBImgInfo data = itemIndex.data(Qt::DisplayRole).value<DBImgInfo>();
            if (data.itemType == ItemTypeTimeLineTitle
                    || data.itemType == ItemTypeImportTimeLineTitle
                    || data.itemType == ItemTypeBlank) {
                titleIndex = itemIndex;
                break;
            }
        }
        int index = titleIndex.row() + 1;
        //根据找到的标题，遍历当前时间下所有数据
        for (; index < m_model->rowCount(); index++) {
            QModelIndex itemIndex = m_model->index(index, 0);
            DBImgInfo data = itemIndex.data(Qt::DisplayRole).value<DBImgInfo>();
            if (data.itemType == ItemTypeTimeLineTitle
                    || data.itemType == ItemTypeImportTimeLineTitle) {
                break;
            } else {
                if (!data.filePath.isEmpty()) {
                    m_allfileslist.append(data.filePath);
                }
            }
        }
    }
    return m_allfileslist;
}

QList<DBImgInfo> ThumbnailListView::getAllFileInfo(int row)
{
    QList<DBImgInfo> DBImgInfos;
    if (m_delegatetype == ThumbnailDelegate::AllPicViewType
            || m_delegatetype == ThumbnailDelegate::AlbumViewCustomType
            || m_delegatetype == ThumbnailDelegate::AlbumViewFavoriteType
            || m_delegatetype == ThumbnailDelegate::SearchViewType) {
        for (int i = 0; i < m_model->rowCount(); i++) {
            QModelIndex index = m_model->index(i, 0);
            DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
            if (data.itemType == ItemType::ItemTypePic) {
                DBImgInfos.append(data);
            }
        }
    } else if (m_delegatetype == ThumbnailDelegate::TimeLineViewType
               || m_delegatetype == ThumbnailDelegate::AlbumViewImportTimeLineViewType) {
        //找到当前项前面最近的一个标题
        QModelIndex titleIndex;
        for (int i = row; i >= 0; i--) {
            QModelIndex itemIndex = m_model->index(i, 0);
            DBImgInfo data = itemIndex.data(Qt::DisplayRole).value<DBImgInfo>();
            if (data.itemType == ItemTypeTimeLineTitle
                    || data.itemType == ItemTypeImportTimeLineTitle
                    || data.itemType == ItemTypeBlank) {
                titleIndex = itemIndex;
                break;
            }
        }
        int index = titleIndex.row() + 1;
        //根据找到的标题，遍历当前时间下所有数据
        for (; index < m_model->rowCount(); index++) {
            QModelIndex itemIndex = m_model->index(index, 0);
            DBImgInfo data = itemIndex.data(Qt::DisplayRole).value<DBImgInfo>();
            if (data.itemType == ItemTypeTimeLineTitle
                    || data.itemType == ItemTypeImportTimeLineTitle) {
                break;
            } else {
                DBImgInfos.append(data);
            }
        }
    }
    return DBImgInfos;
}

int ThumbnailListView::getRow(const QString &path)
{
    int row = -1;
    for (int i = 0; i < m_model->rowCount(); i++) {
        QModelIndex index = m_model->index(i, 0);
        DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
        if (data.itemType == ItemType::ItemTypePic) {
            if (data.filePath == path) {
                row = i;
            }
        }
    }
    return row;
}

void ThumbnailListView::onShowMenu(const QPoint &pos)
{
    //外接设备显示照片时，禁用鼠标右键菜单
    QModelIndex index = this->indexAt(pos);
    if (!index.isValid() || ALBUM_PATHTYPE_BY_PHONE == m_imageType) {
        return;
    }
    //标题项和空白项不显示右键菜单
    DBImgInfo info = index.data(Qt::DisplayRole).value<DBImgInfo>();
    if (info.itemType == ItemTypeBlank
            || info.itemType == ItemTypeTimeLineTitle
            || info.itemType == ItemTypeImportTimeLineTitle) {
        return;
    }
#ifdef tablet_PC
    //区分最近删除界面和其他界面
    if (this->m_imageType == COMMON_STR_TRASH) {
        return;
    } else {
        emit sigMouseRelease();
        updateMenuContents();
        m_pMenu->popup(QCursor::pos());
    }
#else
    emit sigMouseRelease();
    updateMenuContents();
    m_pMenu->popup(QCursor::pos());
#endif
}

void ThumbnailListView::updateMenuContents()
{
    QStringList paths = selectedPaths();
    if (paths.empty()) {
        return;
    }
#ifndef tablet_PC
    if (m_imageType.compare(COMMON_STR_TRASH) == 0) {
        if (1 == paths.length())
            m_MenuActionMap.value(tr("Photo info"))->setVisible(true);
        else
            m_MenuActionMap.value(tr("Photo info"))->setVisible(false);
        return;
    }
#endif
    for (QAction *action : m_MenuActionMap.values()) {
        action->setVisible(true);
        action->setEnabled(true);
    }
    //源文件不存在时的菜单
    if ((1 == paths.length()) && (!QFileInfo(paths[0]).exists()) && (COMMON_STR_TRASH != m_imageType)) {
#ifndef tablet_PC
        m_MenuActionMap.value(tr("View"))->setEnabled(true);
        m_MenuActionMap.value(tr("Fullscreen"))->setEnabled(false);
        m_MenuActionMap.value(tr("Slide show"))->setEnabled(false);
        m_MenuActionMap.value(tr("Export"))->setEnabled(false);
        m_albumMenu->deleteLater();
        m_albumMenu = createAlbumMenu();
        if (m_albumMenu) {
            QAction *action = m_MenuActionMap.value(tr("Export"));
            action->setEnabled(false);
            m_albumMenu->setEnabled(false);
            m_pMenu->insertMenu(action, m_albumMenu);
        }
        m_MenuActionMap.value(tr("Copy"))->setEnabled(false);
#endif
        m_MenuActionMap.value(tr("Delete"))->setEnabled(false);
        m_MenuActionMap.value(tr("Remove from album"))->setVisible(false);
#ifndef tablet_PC
        m_MenuActionMap.value(tr("Print"))->setVisible(false);
#endif
        if (DBManager::instance()->isImgExistInAlbum(COMMON_STR_FAVORITES, paths[0], AlbumDBType::Favourite)) {
            m_MenuActionMap.value(tr("Favorite"))->setVisible(false);
            m_MenuActionMap.value(tr("Unfavorite"))->setEnabled(false);
        } else {
            m_MenuActionMap.value(tr("Unfavorite"))->setVisible(false);
            m_MenuActionMap.value(tr("Favorite"))->setEnabled(false);
        }
#ifndef tablet_PC
        m_MenuActionMap.value(tr("Rotate clockwise"))->setEnabled(false);
        m_MenuActionMap.value(tr("Rotate counterclockwise"))->setEnabled(false);
        m_MenuActionMap.value(tr("Display in file manager"))->setEnabled(false);
        m_MenuActionMap.value(tr("Photo info"))->setEnabled(false);
        m_MenuActionMap.value(tr("Set as wallpaper"))->setEnabled(false);
#endif
        return;
    }
#ifndef tablet_PC
    if (1 != paths.length()) {
        m_MenuActionMap.value(tr("View"))->setVisible(false);
        m_MenuActionMap.value(tr("Fullscreen"))->setVisible(false);
    }
#endif
    if (COMMON_STR_TRASH == m_imageType) {
        m_MenuActionMap.value(tr("Delete"))->setVisible(false);
    } else {
        if (m_albumMenu)
            m_albumMenu->deleteLater();
        m_albumMenu = createAlbumMenu();
        if (m_albumMenu) {
#ifndef tablet_PC
            QAction *action = m_MenuActionMap.value(tr("Export"));
#else
            QAction *action = m_MenuActionMap.value(tr("Delete"));
#endif
            m_pMenu->insertMenu(action, m_albumMenu);
        }
    }
    if (1 == paths.length() && COMMON_STR_TRASH != m_imageType) {

        if (DBManager::instance()->isImgExistInAlbum(COMMON_STR_FAVORITES, paths[0], AlbumDBType::Favourite)) {
            m_MenuActionMap.value(tr("Favorite"))->setVisible(false);
        } else {
            m_MenuActionMap.value(tr("Unfavorite"))->setVisible(false);
        }
        m_pMenu->addSeparator();
    } else {
        m_MenuActionMap.value(tr("Unfavorite"))->setVisible(false);
        m_MenuActionMap.value(tr("Favorite"))->setVisible(false);
    }
    //非自定义相册，隐藏从相册中移除菜单
    if (m_delegatetype != ThumbnailDelegate::AlbumViewCustomType) {
        m_MenuActionMap.value(tr("Remove from album"))->setVisible(false);
    }
#ifndef tablet_PC
    bool bflag_imageSupportSave = false;      //图片是否可以保存标志
    if (1 == paths.length()) { //单张照片
        if (UnionImage_NameSpace::isImageSupportRotate(paths[0]))
            bflag_imageSupportSave = true;
    }
    if (bflag_imageSupportSave) {
        int flag_isRW = 0;
        if (1 == paths.length()) {
            if (QFileInfo(paths[0]).isReadable() && !QFileInfo(paths[0]).isWritable()) {
                flag_isRW = 1;
            }
        }
        if (flag_isRW == 1) {
            m_MenuActionMap.value(tr("Rotate clockwise"))->setDisabled(true);
            m_MenuActionMap.value(tr("Rotate counterclockwise"))->setDisabled(true);
        } else {
            m_MenuActionMap.value(tr("Rotate clockwise"))->setDisabled(false);
            m_MenuActionMap.value(tr("Rotate counterclockwise"))->setDisabled(false);
        }
    } else {
        m_MenuActionMap.value(tr("Rotate clockwise"))->setVisible(false);
        m_MenuActionMap.value(tr("Rotate counterclockwise"))->setVisible(false);
    }
    if (1 != paths.length()) {
        m_MenuActionMap.value(tr("Display in file manager"))->setVisible(false);
        m_MenuActionMap.value(tr("Photo info"))->setVisible(false);
    }
    if ((1 == paths.length() || QFileInfo(paths[0]).suffix().contains("gif"))) {
        m_MenuActionMap.value(tr("Set as wallpaper"))->setVisible(true);
    } else {
        m_MenuActionMap.value(tr("Set as wallpaper"))->setVisible(false);
    }
#endif
}

void ThumbnailListView::appendAction(int id, const QString &text, const QString &shortcut)
{
    QAction *ac = new QAction(this);
    addAction(ac);
    ac->setText(text);
    ac->setProperty("MenuID", id);
    //如果是查看照片，需要响应Enter键，而Enter键有两个Key-Enter和Return
    if (text.compare(tr("View")) == 0) {
        QList<QKeySequence> shortcuts;
        shortcuts.append(QKeySequence(ENTER_SHORTCUT));
        shortcuts.append(QKeySequence(RETURN_SHORTCUT));
        ac->setShortcuts(shortcuts);
    } else {
        ac->setShortcut(QKeySequence(shortcut));
    }
    ac->setShortcutContext(Qt::WidgetShortcut);
    m_MenuActionMap.insert(text, ac);
    m_pMenu->addAction(ac);
}

void ThumbnailListView::initMenuAction()
{
    m_pMenu->clear();
    if (m_imageType.compare(COMMON_STR_TRASH) == 0) {
        appendAction(IdMoveToTrash, tr("Delete"), ss(""));
        appendAction(IdTrashRecovery, tr("Restore"), ss(BUTTON_RECOVERY));
        appendAction(IdImageInfo, tr("Photo info"), ss(ImageInfo_CONTEXT_MENU));
        return;
    }

    m_MenuActionMap.clear();
#ifndef tablet_PC
    appendAction(IdView, tr("View"), ss(VIEW_CONTEXT_MENU));
    appendAction(IdFullScreen, tr("Fullscreen"), ss(FULLSCREEN_CONTEXT_MENU));
    appendAction(IdPrint, tr("Print"), ss(PRINT_CONTEXT_MENU));
    appendAction(IdStartSlideShow, tr("Slide show"), ss(SLIDESHOW_CONTEXT_MENU));
    m_pMenu->addSeparator();
    appendAction(IdExport, tr("Export"), ss(EXPORT_CONTEXT_MENU));
    appendAction(IdCopyToClipboard, tr("Copy"), ss(COPYTOCLIPBOARD_CONTEXT_MENU));
#endif
    appendAction(IdMoveToTrash, tr("Delete"), ss(""));
    appendAction(IdRemoveFromAlbum, tr("Remove from album"), ss(""));

    m_pMenu->addSeparator();
    appendAction(IdAddToFavorites, tr("Favorite"), "");
    appendAction(IdRemoveFromFavorites, tr("Unfavorite"), "");
#ifndef tablet_PC
    m_pMenu->addSeparator();
    appendAction(IdRotateClockwise, tr("Rotate clockwise"), ss(ROTATECLOCKWISE_CONTEXT_MENU));
    appendAction(IdRotateCounterclockwise, tr("Rotate counterclockwise"),
                 ss(ROTATECOUNTERCLOCKWISE_CONTEXT_MENU));
    m_pMenu->addSeparator();
    appendAction(IdSetAsWallpaper, tr("Set as wallpaper"), ss(SETASWALLPAPER_CONTEXT_MENU));
    appendAction(IdDisplayInFileManager, tr("Display in file manager"), ss(DISPLAYINFILEMANAGER_CONTEXT_MENU));
    appendAction(IdImageInfo, tr("Photo info"), ss(ImageInfo_CONTEXT_MENU));
#endif
}

DMenu *ThumbnailListView::createAlbumMenu()
{
    DMenu *am = new DMenu(tr("Add to album"));
    QStringList albums = DBManager::instance()->getAllAlbumNames();
    QAction *ac1 = new QAction(am);
    ac1->setProperty("MenuID", IdAddToAlbum);
    ac1->setText(tr("New album"));
    ac1->setData("Add to new album");
    am->addAction(ac1);
    am->addSeparator();
    QModelIndexList indexList = selectionModel()->selectedIndexes();
    QStringList albumNames;
    // 单选
    if (indexList.count() == 1) {
        albumNames = indexList.first().model()->data(indexList.first(), Qt::UserRole + 2).toStringList();
    }
    // 多选,以第一个作标准
    else if (indexList.count() > 1) {
        albumNames = indexList.first().model()->data(indexList.first(), Qt::UserRole + 2).toStringList();
        for (int idx = 1; idx < indexList.count(); idx++) {
            QStringList tempList = indexList.at(idx).model()->data(indexList.at(idx), Qt::UserRole + 2).toStringList();
            for (int i = 0; i < albumNames.count(); i++) {
                // 不存在相册名
                if (!tempList.contains(albumNames.at(i))) {
                    albumNames.removeAll(albumNames.at(i));
                    break;
                }
            }
        }
    }
    for (QString album : albums) {
        QAction *ac = new QAction(am);
        ac->setProperty("MenuID", IdAddToAlbum);
        ac->setText(
            fontMetrics().elidedText(QString(album).replace("&", "&&"), Qt::ElideMiddle, 200));
        ac->setData(album);
        am->addAction(ac);
        if (albumNames.contains(album)) {
            ac->setEnabled(false);
        }
    }
    return am;
}

void ThumbnailListView::onMenuItemClicked(QAction *action)
{
    QStringList paths = selectedPaths();
    menuItemDeal(paths, action);
}
//获取选中路径
QStringList ThumbnailListView::selectedPaths()
{
    QStringList paths;
    for (QModelIndex index : selectionModel()->selectedIndexes()) {
        if (isRowHidden(index.row())) {
            continue;
        }
        DBImgInfo info = index.data(Qt::DisplayRole).value<DBImgInfo>();
        if ((info.itemType != ItemTypeBlank
                || info.itemType != ItemTypeTimeLineTitle
                || info.itemType != ItemTypeImportTimeLineTitle)
                && !info.filePath.isEmpty()) {
            paths << info.filePath;
        }
    }
    return paths;
}

QStringList ThumbnailListView::getDagItemPath()
{
    return m_dragItemPath;
}

void ThumbnailListView::menuItemDeal(QStringList paths, QAction *action)
{
    paths.removeAll(QString(""));
    if (paths.isEmpty()) {
        return;
    }
    struct Listolditem {
        int row;
        int column;
    };
    const QString path = paths.first();
    const int id = action->property("MenuID").toInt();
    switch (MenuItemId(id)) {
    case IdView:
        //调用双击打开信号
        emit openImage(this->currentIndex().row(), path, false);
        break;
    case IdFullScreen:
        emit openImage(this->currentIndex().row(), path, true);
        break;
    case IdPrint:
        PrintHelper::getIntance()->showPrintDialog(paths, this);
        break;
    case IdStartSlideShow:
        emit sigSlideShow(path);
        break;
    case IdAddToAlbum: {
        const QString album = action->data().toString();
        if (album != "Add to new album") {
            if (1 == paths.count()) {
                if (!DBManager::instance()->isImgExistInAlbum(album, paths[0])) {
                    emit dApp->signalM->sigAddToAlbToast(album);
                }
            } else {
                emit dApp->signalM->sigAddToAlbToast(album);
            }
            DBManager::instance()->insertIntoAlbum(album, paths);
            emit dApp->signalM->insertedIntoAlbum(album, paths);
            // 相册照片更新时的．更新路径相册名缓存,用于listview的setdata userrole + 2
            ImageEngineApi::instance()->setImgPathAndAlbumNames(DBManager::instance()->getAllPathAlbumNames());
            // 只更新部分，即将照片添加或者删除相册时
            updateModelRoleData(album, IdAddToAlbum);

        } else {
            emit dApp->signalM->createAlbum(paths);
        }
        break;
    }
    case IdCopyToClipboard:
        utils::base::copyImageToClipboard(paths);
        break;
    case IdMoveToTrash: {
        qDebug() << "---" << __FUNCTION__ << "---" << "IdMoveToTrash";
        clearSelection();
        this->removeSelectToTrash(paths);
    }
    break;
    case IdAddToFavorites:
        DBManager::instance()->insertIntoAlbum(COMMON_STR_FAVORITES, paths, AlbumDBType::Favourite);
        emit dApp->signalM->insertedIntoAlbum(COMMON_STR_FAVORITES, paths);
        break;
    case IdRemoveFromFavorites:
        DBManager::instance()->removeFromAlbum(COMMON_STR_FAVORITES, paths, AlbumDBType::Favourite);
        break;
    case IdRemoveFromAlbum: {
        if (IMAGE_DEFAULTTYPE != m_imageType && COMMON_STR_VIEW_TIMELINE != m_imageType &&
                COMMON_STR_RECENT_IMPORTED != m_imageType && COMMON_STR_TRASH != m_imageType) {
            // 只更新部分，从相册移出时
            updateModelRoleData(m_imageType, IdRemoveFromAlbum);
            DBManager::instance()->removeFromAlbum(m_imageType, paths);
            // 相册照片更新时的．更新路径相册名缓存,用于listview的setdata userrole + 2
            ImageEngineApi::instance()->setImgPathAndAlbumNames(DBManager::instance()->getAllPathAlbumNames());
        }
    }
    break;
    case IdRotateClockwise: {
        //发送给子线程旋转图片
        emit ImageEngineApi::instance()->sigRotateImageFile(90, path);
        dApp->m_imageloader->updateImageLoader(paths);
    }
    break;
    case IdRotateCounterclockwise: {
        //发送给子线程旋转图片
        emit ImageEngineApi::instance()->sigRotateImageFile(-90, path);
        dApp->m_imageloader->updateImageLoader(paths);
    }
    break;
    case IdSetAsWallpaper:
        dApp->wpSetter->setBackground(path);
        break;
    case IdDisplayInFileManager:
        utils::base::showInFileManager(path);
        break;
    case IdImageInfo:
        emit dApp->signalM->showImageInfo(path);
        break;
    case IdExport:
        emit dApp->signalM->exportImage(paths);
        break;
    case IdTrashRecovery:
        emit trashRecovery();
        break;
    default:
        break;
    }
}

void ThumbnailListView::onPixMapScale(int value)
{
//    if (!this->isVisible())
//        return;
    switch (value) {
    case 0:
        m_iBaseHeight = 80;
        break;
    case 1:
        m_iBaseHeight = 90;
        break;
    case 2:
        m_iBaseHeight = 100;
        break;
    case 3:
        m_iBaseHeight = 110;
        break;
    case 4:
        m_iBaseHeight = 120;
        break;
    case 5:
        m_iBaseHeight = 130;
        break;
    case 6:
        m_iBaseHeight = 140;
        break;
    case 7:
        m_iBaseHeight = 150;
        break;
    case 8:
        m_iBaseHeight = 160;
        break;
    case 9:
        m_iBaseHeight = 170;
        break;
    default:
        m_iBaseHeight = 80;
        break;
    }
    resizeEventF();
}

void ThumbnailListView::onCancelFavorite(const QModelIndex &index)
{
    QStringList str;
    DBImgInfo info = index.data(Qt::DisplayRole).value<DBImgInfo>();
    str << info.filePath;
    //通知其它界面更新取消收藏
    DBManager::instance()->removeFromAlbum(COMMON_STR_FAVORITES, str, AlbumDBType::Favourite);
    emit dApp->signalM->updateFavoriteNum();
    m_model->removeRow(index.row());
    updateThumbnailView();
}

void ThumbnailListView::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED(e)
    //改变第一个空白项和标题项的宽度
    for (int i = 0; i < m_model->rowCount(); i++) {
        QModelIndex index = m_model->index(i, 0);
        DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
        if (data.itemType == ItemType::ItemTypeBlank
                || data.itemType == ItemType::ItemTypeTimeLineTitle
                || data.itemType == ItemType::ItemTypeImportTimeLineTitle) {
            data.imgWidth = this->width() - 5;
            QVariant meta;
            meta.setValue(data);
            m_model->setData(index, meta, Qt::DisplayRole);
            m_model->setData(index, QVariant(QSize(data.imgWidth, data.imgWidth)), Qt::SizeHintRole);
        }
    }

    resizeEventF();

//    if (m_model->rowCount() > 0) {
//        for (int i = 0; i < m_model->rowCount(); i++) {
//            QModelIndex index = m_model->index(i, 0);
//            QRect rect = this->visualRect(index);
//            qDebug() << "------" << __FUNCTION__ << "---row = " << i << "---rect = " << rect;
//        }
//        qDebug() << "------" << __FUNCTION__ << "--------rect = " << this->rect();
//    }
}

bool ThumbnailListView::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == verticalScrollBar()) {
        if (e->type() == QEvent::MouseButtonPress) {
            //刷新鼠标在滚动条上按下标志位
            QRect scrollGlobaRect(verticalScrollBar()->mapToGlobal(QPoint(0, 0)),
                                  QSize(verticalScrollBar()->geometry().width(), verticalScrollBar()->geometry().height()));
            if (scrollGlobaRect.contains(QCursor::pos())) {
                m_animationEnable = true;
            }
        } else if (e->type() == QEvent::MouseButtonRelease) {
            //刷新鼠标在滚动条上释放标志位
            m_animationEnable = false;
        }
    }

    if (e->type() == QEvent::Wheel && QApplication::keyboardModifiers() == Qt::ControlModifier) {
        return true;
    } else if (e->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
        if (keyEvent->key() == Qt::Key_PageDown) {
            DScrollBar *vb = this->verticalScrollBar();
            int posValue = vb->value();
            posValue += this->height();
            vb->setValue(posValue);
            return true;
        } else if (keyEvent->key() == Qt::Key_PageUp) {
            DScrollBar *vb = this->verticalScrollBar();
            int posValue = vb->value();
            posValue -= this->height();
            vb->setValue(posValue);
            return true;
        }
    }
    return false;
}

QPixmap ThumbnailListView::getDamagedPixmap()
{
    return utils::image::getDamagePixmap(DApplicationHelper::instance()->themeType() == DApplicationHelper::LightType);
}

QStringList ThumbnailListView::getCurrentIndexTime(const QModelIndex &index)
{
    // index对应的是图片，获取所属时间线的时间和数量
    QStringList list;
    if (index.isValid()) {
        for (int i = index.row(); i >= 0; i--) {
            QModelIndex idx = m_model->index(i, 0);
            DBImgInfo tempdata = idx.data(Qt::DisplayRole).value<DBImgInfo>();
            if (tempdata.itemType == ItemTypeBlank || tempdata.itemType == ItemTypeTimeLineTitle
                    || tempdata.itemType == ItemTypeImportTimeLineTitle) {
                list.append(tempdata.date);
                list.append(tempdata.num);
                break;
            }
        }
    }
    return list;
}

bool ThumbnailListView::getCurrentIndexSelectStatus(const QModelIndex &index, bool isPic)
{
    if (!index.isValid()) {
        return false;
    }
    QModelIndexList list = selectionModel()->selectedIndexes();
    if (isPic) {
        //图片，向前循环判断选中状态
        for (int i = index.row(); i > 0; i--) {
            QModelIndex idx = m_model->index(i, 0);
            DBImgInfo tempdata = idx.data(Qt::DisplayRole).value<DBImgInfo>();
            if (tempdata.itemType == ItemTypeBlank || tempdata.itemType == ItemTypeTimeLineTitle
                    || tempdata.itemType == ItemTypeImportTimeLineTitle) {
                break;
            }
            if (!list.contains(idx)) {
                return false;
            }
        }
        //图片，向后循环判断选中状态
        for (int j = index.row(); j < m_model->rowCount(); j++) {
            QModelIndex idx = m_model->index(j, 0);
            DBImgInfo tempdata = idx.data(Qt::DisplayRole).value<DBImgInfo>();
            if (tempdata.itemType == ItemTypeBlank || tempdata.itemType == ItemTypeTimeLineTitle
                    || tempdata.itemType == ItemTypeImportTimeLineTitle) {
                break;
            }
            if (!list.contains(idx)) {
                return false;
            }
        }
    } else {//标题，+1进入第一张
        for (int j = index.row() + 1; j < m_model->rowCount(); j++) {
            QModelIndex idx = m_model->index(j, 0);
            DBImgInfo tempdata = idx.data(Qt::DisplayRole).value<DBImgInfo>();
            if (tempdata.itemType == ItemTypeBlank || tempdata.itemType == ItemTypeTimeLineTitle
                    || tempdata.itemType == ItemTypeImportTimeLineTitle) {
                break;
            }
            if (!list.contains(idx)) {
                return false;
            }
        }
    }
    return true;
}

bool ThumbnailListView::isAllAppointType(ItemType type)
{
    bool isAllAppointType = true;
    for (int i = 0; i < m_model->rowCount(); i++) {
        QModelIndex index = m_model->index(i, 0);
        DBImgInfo info = index.data(Qt::DisplayRole).value<DBImgInfo>();
        if (type == ItemTypePic) {
            if (info.itemType == ItemTypeVideo) {
                isAllAppointType = false;
                break;
            }
        } else if (type == ItemTypeVideo) {
            if (info.itemType == ItemTypePic) {
                isAllAppointType = false;
                break;
            }
        }
    }
    qDebug() << __FUNCTION__ << "---isAllAppointType = " << isAllAppointType;
    return isAllAppointType;
}

void ThumbnailListView::hideAllAppointType(ItemType type)
{
    for (int i = 0; i < m_model->rowCount(); i++) {
        QModelIndex index = m_model->index(i, 0);
        DBImgInfo info = index.data(Qt::DisplayRole).value<DBImgInfo>();
        if (info.itemType == type) {
            setRowHidden(i, true);
        } else {
            setRowHidden(i, false);
        }
    }
    //有可能出现图片或者视频删除好，时间线页面两个标题在一起的情况
    for (int i = 0; i < m_model->rowCount(); i++) {
        QModelIndex index = m_model->index(i, 0);
        DBImgInfo info = index.data(Qt::DisplayRole).value<DBImgInfo>();
        if (info.itemType == ItemTypeTimeLineTitle | info.itemType == ItemTypeImportTimeLineTitle) {
            //上面一项如果是空白项，则隐藏当前标题
            if ((i - 1 >= 0)) {
                QModelIndex preIndex = m_model->index((i - 1), 0);
                DBImgInfo preInfo = preIndex.data(Qt::DisplayRole).value<DBImgInfo>();
                if (preInfo.itemType == ItemTypeBlank) {
                    setRowHidden(i, true);
                }
            }
            //如果下一项是标题，则隐藏当前项
            if ((i + 1) < m_model->rowCount()) {
                QModelIndex nextIndex = m_model->index((i + 1), 0);
                DBImgInfo nextInfo = nextIndex.data(Qt::DisplayRole).value<DBImgInfo>();
                if (nextInfo.itemType == ItemTypeTimeLineTitle | nextInfo.itemType == ItemTypeImportTimeLineTitle) {
                    setRowHidden(i, true);
                }
            }
        }
    }
}

int ThumbnailListView::getRow(QPoint point)
{
    return indexAt(point).row();
}

void ThumbnailListView::setListViewUseFor(ThumbnailListView::ListViewUseFor usefor)
{
    m_useFor = usefor;
}

//add start 3975
int ThumbnailListView::getListViewHeight()
{
    return m_height;
}
//add end 3975

void ThumbnailListView::sltChangeDamagedPixOnThemeChanged()
{
    for (int i = 0; i < m_model->rowCount(); i++) {
        QModelIndex index = m_model->index(i, 0);
        DBImgInfo info = index.data(Qt::DisplayRole).value<DBImgInfo>();

        const bool &bNotSuppOrDmg = info.bNotSupportedOrDamaged;
        if (bNotSuppOrDmg) {
            info.damagedPixmap = getDamagedPixmap();
            QVariant infoVariant;
            infoVariant.setValue(info);
            m_model->item(i, 0)->setData(infoVariant, Qt::DisplayRole);
        }
    }
}
//有文件删除，刷新所有列表
void ThumbnailListView::updateThumbnailViewAfterDelete(const DBImgInfoList &infos)
{
    //列表上移除所有图片项
    if (m_delegatetype == ThumbnailDelegate::AllPicViewType
            || m_delegatetype == ThumbnailDelegate::AlbumViewCustomType
            || m_delegatetype == ThumbnailDelegate::AlbumViewFavoriteType
            || m_delegatetype == ThumbnailDelegate::TimeLineViewType
            || m_delegatetype == ThumbnailDelegate::AlbumViewImportTimeLineViewType) {
        foreach (auto info, infos) {
            for (int i = (m_model->rowCount() - 1); i >= 0; i--) {
                QModelIndex index = m_model->index(i, 0);
                DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
                if (info.filePath == data.filePath) {
                    m_model->removeRow(i);
                    break;
                }
            }
        }
    }
    if (m_delegatetype == ThumbnailDelegate::TimeLineViewType
            || m_delegatetype == ThumbnailDelegate::AlbumViewImportTimeLineViewType) {
        //给需要移除的项添加标志
        for (int i = 0; i < m_model->rowCount(); i++) {
            QModelIndex index = m_model->index(i, 0);
            DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
            if (i < (m_model->rowCount() - 1)) {
                QModelIndex indexNext = m_model->index((i + 1), 0);
                DBImgInfo dataNext = indexNext.data(Qt::DisplayRole).value<DBImgInfo>();
                //当前项和下一项都是标题，则当前项标记为需要删除
                if ((data.itemType == ItemTypeTimeLineTitle && dataNext.itemType == ItemTypeTimeLineTitle)
                        || (data.itemType == ItemTypeImportTimeLineTitle && dataNext.itemType == ItemTypeImportTimeLineTitle)) {
                    data.bNeedDelete = true;
                    QVariant meta;
                    meta.setValue(data);
                    m_model->setData(index, meta, Qt::DisplayRole);
                }
            } else if (i == (m_model->rowCount() - 1)) {
                //最后一项是标题，标记需要删除
                if (data.itemType == ItemTypeTimeLineTitle || data.itemType == ItemTypeImportTimeLineTitle) {
                    data.bNeedDelete = true;
                    QVariant meta;
                    meta.setValue(data);
                    m_model->setData(index, meta, Qt::DisplayRole);
                }
            }
            if (i == 1) {
                //第二项是标题，标记需要删除
                if (data.itemType == ItemTypeTimeLineTitle || data.itemType == ItemTypeImportTimeLineTitle) {
                    data.bNeedDelete = true;
                    QVariant meta;
                    meta.setValue(data);
                    m_model->setData(index, meta, Qt::DisplayRole);
                }
            }
        }
        //移除添加了标志的项
        for (int i = 0; i < m_model->rowCount(); i++) {
            QModelIndex index = m_model->index(i, 0);
            DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
            if (data.bNeedDelete) {
                m_model->removeRow(i);
                //移除后先减1，后面加1的时候才不会遗漏
                i--;
            }
        }
    }
}

void ThumbnailListView::slotSelectCurrentDatePic(bool isSelect, QStandardItem *item)
{
    if (item == nullptr || !m_model->indexFromItem(item).isValid()) {
        return;
    }
    int index = m_model->indexFromItem(item).row() + 1;
    for (; index < m_model->rowCount(); index++) {
        QModelIndex itemIndex = m_model->index(index, 0);
        DBImgInfo data = itemIndex.data(Qt::DisplayRole).value<DBImgInfo>();
        if (data.itemType == ItemTypeTimeLineTitle || data.itemType == ItemTypeImportTimeLineTitle) {
            break;
        } else {
            if (isSelect) {
                selectionModel()->select(itemIndex, QItemSelectionModel::Select);
            } else {
                selectionModel()->select(itemIndex, QItemSelectionModel::Deselect);
            }
        }
    }
}
//刷新所有标题中选择按钮的状态
void ThumbnailListView::slotChangeAllSelectBtnVisible(bool visible)
{
    if (m_delegatetype == ThumbnailDelegate::TimeLineViewType) {
        TimeLineDateWidget *w = nullptr;
        for (int i = 0; i < m_model->rowCount(); i++) {
            QModelIndex idx = m_model->index(i, 0);
            DBImgInfo data = idx.data(Qt::DisplayRole).value<DBImgInfo>();
            if (data.itemType == ItemTypeTimeLineTitle) {
                w = static_cast<TimeLineDateWidget *>(this->indexWidget(idx));
                if (w != nullptr)  {
                    w->onChangeChooseBtnVisible(visible);
                }
            }
        }
    } else if (m_delegatetype == ThumbnailDelegate::AlbumViewImportTimeLineViewType) {
        importTimeLineDateWidget *w = nullptr;
        for (int i = 0; i < m_model->rowCount(); i++) {
            QModelIndex idx = m_model->index(i, 0);
            DBImgInfo data = idx.data(Qt::DisplayRole).value<DBImgInfo>();
            if (data.itemType == ItemTypeImportTimeLineTitle) {
                w = static_cast<importTimeLineDateWidget *>(this->indexWidget(idx));
                if (w != nullptr)  {
                    w->onChangeChooseBtnVisible(visible);
                }
            }
        }
    }
}

// 选中重复导入的图片
void ThumbnailListView::selectDuplicatePhotos(QStringList paths)
{
    QModelIndex firstIndex;
    if (paths.count() > 0) {
        this->clearSelection();
        for (int i = 0; i < m_model->rowCount(); i++) {
            QModelIndex index = m_model->index(i, 0);
            DBImgInfo info = index.data(Qt::DisplayRole).value<DBImgInfo>();
            for (int j = 0; j < paths.count(); j++) {
                if (info.filePath == paths.at(j)) {
                    // 选中
                    selectionModel()->select(index, QItemSelectionModel::Select);
                    if (!firstIndex.isValid()) {
                        firstIndex = index;
                    }
                }
            }
        }
    }
    // 定位第一个重复导入的照片
    if (firstIndex.isValid()) {
        this->scrollTo(firstIndex, ScrollHint::PositionAtCenter);
    }
}

void ThumbnailListView::updateModelRoleData(QString albumName, int actionType)
{
    // listview存在多个，状态model不同，所以先处理已知的，再通知其他model
    // 本listview对象不再处理其他listviw对象model的同步问题！
    if (actionType == IdRemoveFromAlbum) {
        // remove from album
        for (QModelIndex index : selectedIndexes()) {
            QStringList datas = index.model()->data(index, Qt::UserRole + 2).toStringList();
            datas.removeAll(albumName);
            QMap<int, QVariant> tempData;
            tempData.insert(Qt::UserRole + 2, datas);
            m_model->setItemData(index, tempData);
        }
    } else if (actionType == IdAddToAlbum) {
        // add to album
        for (QModelIndex index : selectedIndexes()) {
            QStringList datas = index.model()->data(index, Qt::UserRole + 2).toStringList();
            datas.append(albumName);
            QMap<int, QVariant> tempData;
            tempData.insert(Qt::UserRole + 2, datas);
            m_model->setItemData(index, tempData);
        }
    } else if (actionType == IdMoveToTrash) {
        // delete photos
        //        for (QModelIndex index : selectedIndexes()) {
        //            QStringList datas;
        //            datas.clear();
        //            QMap<int, QVariant> tempData;
        //            tempData.insert(Qt::UserRole + 2, datas);
        //            m_model->setItemData(index, tempData);
        //        }
    }
    QStringList paths;
    paths.clear();
    for (QModelIndex index : selectedIndexes()) {
        paths.append(index.data(Qt::DisplayRole).value<DBImgInfo>().filePath);
    }
    emit SignalManager::instance()->sigSyncListviewModelData(paths, albumName, actionType);
}

void ThumbnailListView::selectFirstPhoto()
{
    if (m_model->rowCount() < 1)
        return;
    QModelIndex idx = m_model->index(0, 0);
    selectionModel()->setCurrentIndex(idx, QItemSelectionModel::Select);
    selectionModel()->select(idx, QItemSelectionModel::Select);
}

bool ThumbnailListView::isFirstPhotoSelected()
{
    QModelIndexList idx = selectionModel()->selectedIndexes();
    for (QModelIndex index : selectionModel()->selectedIndexes()) {
        if (index.row() == 0) {
            return true;
        }
    }
    return  false;
}

bool ThumbnailListView::isNoPhotosSelected()
{
    QModelIndexList indexList = selectionModel()->selectedIndexes();
    if (indexList.count() > 0)
        return false;
    else
        return true;
}

void ThumbnailListView::clearAll()
{
    m_model->clear();
}
//插入一个空白项，ItemTypeBlank
//插入时间线标题，ItemTypeTimeLineTitle
//插入已导入时间线标题，ItemTypeImportTimeLineTitle
void ThumbnailListView::insertBlankOrTitleItem(ItemType type, const QString &date, const QString &num, int height)
{
    DBImgInfo info;
    info.itemType = type;
    info.imgWidth = this->width();
    if (type == ItemTypeBlank) {
        m_blankItemHeight = height;
    }
    info.imgHeight = height;
    info.date = date;
    info.num = num;
    insertThumbnail(info);
}
//更新空白栏高度
void ThumbnailListView::resetBlankItemHeight(int height)
{

    for (int i = 0; i < m_model->rowCount(); i++) {
        QModelIndex idx = m_model->index(i, 0);
        DBImgInfo data = idx.data(Qt::DisplayRole).value<DBImgInfo>();
        if (data.itemType == ItemTypeBlank) {
            m_blankItemHeight = height;
            data.imgHeight = height;

            QVariant infoVariant;
            infoVariant.setValue(data);
            m_model->setData(idx, infoVariant, Qt::DisplayRole);
            break;
        }
    }
    doItemsLayout();
}

void ThumbnailListView::insertThumbnailByImgInfos(DBImgInfoList infoList)
{
    for (int i = 0; i < infoList.size(); ++i) {
        DBImgInfo imgData = infoList.at(i);
        DBImgInfo info = imgData;
        if (ImageEngineApi::instance()->m_AllImageMap[imgData.filePath].isNull()) {
            info.damagedPixmap = getDamagedPixmap();
        } else {
            info.image = ImageEngineApi::instance()->m_AllImageMap[imgData.filePath];
        }
        info.imgWidth = m_onePicWidth;
        info.imgHeight = m_onePicWidth;
        insertThumbnail(info);
    }
}
//判断所有图片是否全选中
bool ThumbnailListView::isAllSelected(ItemType type)
{
    bool isAllSelected = true;
    qDebug() << __FUNCTION__ << "---type = " << type;

    if (this->selectionModel()->selection().size() == 0) {
        isAllSelected = false;
        return isAllSelected;
    }
    for (int i = 0; i < m_model->rowCount(); i++) {
        QModelIndex idx = m_model->index(i, 0);
        DBImgInfo data = idx.data(Qt::DisplayRole).value<DBImgInfo>();
        //全选
        if (type == ItemType::ItemTypeNull) {
            if ((data.itemType == ItemTypeVideo || data.itemType == ItemTypePic)
                    && !this->selectionModel()->selection().contains(idx)) {
                isAllSelected = false;
                break;
            }
        } else if (type == ItemTypePic) {
            if (data.itemType == ItemTypePic && !this->selectionModel()->selection().contains(idx)) {
                isAllSelected = false;
                break;
            }
        } else if (type == ItemTypeVideo) {
            if (data.itemType == ItemTypeVideo && !this->selectionModel()->selection().contains(idx)) {
                isAllSelected = false;
                break;
            }
        }
    }
    return isAllSelected;
}
//判断选中图片是否都可旋转
bool ThumbnailListView::isAllSelectedSupportRotate()
{
    QStringList list = selectedPaths();
    int count = list.size();
    bool isAllSelectedSupportRotate = true;
    if (count <= 0) {
        isAllSelectedSupportRotate = false;
    }
    for (int i = 0; i < count; i++) {
        QString path = list.at(i);
        if (!UnionImage_NameSpace::isImageSupportRotate(path)) {
            isAllSelectedSupportRotate = false;
            break;
        }
    }
    return isAllSelectedSupportRotate;
}

//删除到相册已删除
void ThumbnailListView::removeSelectToTrash(QStringList paths)
{
    ImgDeleteDialog *dialog = new ImgDeleteDialog(this, paths.length());
    dialog->setObjectName("deteledialog");
    if (dialog->exec() > 0) {
        // 只更新部分，删除
        updateModelRoleData("", IdMoveToTrash);
        if (COMMON_STR_VIEW_TIMELINE == m_imageType || COMMON_STR_RECENT_IMPORTED == m_imageType) {
            emit sigMoveToTrash();
        }
        (COMMON_STR_TRASH == m_imageType) ? ImageEngineApi::instance()->moveImagesToTrash(paths, true, false)
        : ImageEngineApi::instance()->moveImagesToTrash(paths);
    }
}
//更新时间线界面内各个按钮的text状态，单选/框选
void ThumbnailListView::updatetimeLimeBtnText()
{
    QModelIndexList indexs = selectionModel()->selectedIndexes();
    bool isSelectAll = true;
    if (m_delegatetype == ThumbnailDelegate::TimeLineViewType) {
        TimeLineDateWidget *w = nullptr;
        for (int i = 0; i < m_model->rowCount(); i++) {
            QModelIndex idx = m_model->index(i, 0);
            DBImgInfo data = idx.data(Qt::DisplayRole).value<DBImgInfo>();
            if (data.itemType == ItemTypeBlank) {
                continue;
            }
            if (data.itemType == ItemTypeTimeLineTitle) {
                if (w == nullptr) { //说明是第一个时间线
                    // 悬浮时间线标题内的按钮状态同步
                    emit sigTimeLineDataAndNum("", "", isSelectAll ? QObject::tr("Unselect") : QObject::tr("Select"));
                } else {
                    w->onTimeLinePicSelectAll(isSelectAll);
                }
                w = static_cast<TimeLineDateWidget *>(this->indexWidget(idx));
                isSelectAll = true;
                continue;
            }
            if (!indexs.contains(idx)) {
                isSelectAll = false;
            }
        }
        //最后一个时间线的相关逻辑
        if (w) {
            w->onTimeLinePicSelectAll(isSelectAll);
        } else { //有且只有一个时间线时的逻辑
            emit sigTimeLineDataAndNum("", "", isSelectAll ? QObject::tr("Unselect") : QObject::tr("Select"));
        }
    }  else if (m_delegatetype == ThumbnailDelegate::AlbumViewImportTimeLineViewType) {
        importTimeLineDateWidget *iw = nullptr;
        isSelectAll = true;
        for (int i = 0; i < m_model->rowCount(); i++) {
            QModelIndex idx = m_model->index(i, 0);
            DBImgInfo data = idx.data(Qt::DisplayRole).value<DBImgInfo>();
            if (data.itemType == ItemTypeBlank) {
                continue;
            }
            if (data.itemType == ItemTypeImportTimeLineTitle) {
                if (iw == nullptr) { //说明是第一个时间线
                    // 悬浮时间线标题内的按钮状态同步
                    emit sigTimeLineDataAndNum("", "", isSelectAll ? QObject::tr("Unselect") : QObject::tr("Select"));
                } else {
                    iw->onTimeLinePicSelectAll(isSelectAll);
                }
                iw = static_cast<importTimeLineDateWidget *>(this->indexWidget(idx));
                isSelectAll = true;
                continue;
            }
            if (!indexs.contains(idx)) {
                isSelectAll = false;
            }
        }
        //最后一个时间线的相关逻辑
        if (iw) {
            iw->onTimeLinePicSelectAll(isSelectAll);
        } else { //有且只有一个时间线时的逻辑
            emit sigTimeLineDataAndNum("", "", isSelectAll ? QObject::tr("Unselect") : QObject::tr("Select"));
        }
    }
}

void ThumbnailListView::showAppointTypeItem(ItemType type)
{
    //切换显示之前先清除选中
    this->clearSelection();
    qDebug() << __FUNCTION__ << "---";
    if (type == ItemTypePic) {
        //显示所有图片
        if (isAllAppointType(ItemTypeVideo)) {
            //如果全是视频，显示无结果
            //todo
            emit sigNoPicOrNoVideo(true);
        } else {
            //隐藏视频项
            hideAllAppointType(ItemTypeVideo);
            emit sigNoPicOrNoVideo(false);
        }
    } else if (type == ItemTypeVideo) {
        //显示所有视频
        if (isAllAppointType(ItemTypePic)) {
            //如果全是图片，显示无结果
            //todo
            emit sigNoPicOrNoVideo(true);
        } else {
            //隐藏图片项
            hideAllAppointType(ItemTypePic);
            emit sigNoPicOrNoVideo(false);
        }
    } else if (type == ItemTypeNull) {
        //恢复显示所有
        hideAllAppointType(ItemTypeNull);
        emit sigNoPicOrNoVideo(false);
    }
}
//显示类型数量
int ThumbnailListView::getAppointTypeItemCount(ItemType type)
{
    int count = 0;
    for (int i = 0;  i < m_model->rowCount(); i++) {
        QModelIndex index = m_model->index(i, 0);
        DBImgInfo pdata = index.data(Qt::DisplayRole).value<DBImgInfo>();
        //全选
        if (type == ItemType::ItemTypeNull) {
            if (pdata.itemType == ItemTypeVideo || pdata.itemType == ItemTypePic) {
                count++;
            }
        } else if (type == pdata.itemType) {
            count++;
        }
    }
    return count;
}
//显示指定类型选中项数量
int ThumbnailListView::getAppointTypeSelectItemCount(ItemType type)
{
    int count = 0;
    for (int i = 0;  i < selectedIndexes().size(); i++) {
        QModelIndex index = selectedIndexes().at(i);
        DBImgInfo pdata = index.data(Qt::DisplayRole).value<DBImgInfo>();
        //全选
        if (type == ItemType::ItemTypeNull) {
            if (pdata.itemType == ItemTypeVideo || pdata.itemType == ItemTypePic) {
                count++;
            }
        } else if (type == pdata.itemType) {
            count++;
        }
    }
    return count;
}
//按类型选择
void ThumbnailListView::selectAllByItemType(ItemType type)
{
    qDebug() << __FUNCTION__ << "---type = " << type;
    this->selectAll();
    emit sigSelectAll();
    //因为性能问题，未根据类型选择，这里做全选处理，全选后，在获取选中项处过滤隐藏项
//    if (type == ItemTypeNull) {
//        this->selectAll();
//    } else {
//        for (int i = 0;  i < m_model->rowCount(); i++) {
//            QModelIndex index = m_model->index(i, 0);
//            DBImgInfo pdata = index.data(Qt::DisplayRole).value<DBImgInfo>();
//            if (pdata.itemType  == type) {
//                selectionModel()->select(index, QItemSelectionModel::Select);
//            }
//        }
//    }
}

void ThumbnailListView::TimeLineSelectAllBtn()
{
    if (m_delegatetype == ThumbnailDelegate::TimeLineViewType || m_delegatetype == ThumbnailDelegate:: AlbumViewImportTimeLineViewType) {
        for (int i = 0; i < m_model->rowCount(); i++) {
            QModelIndex idx = m_model->index(i, 0);
            DBImgInfo data = idx.data(Qt::DisplayRole).value<DBImgInfo>();
            if (data.itemType == ItemTypeTimeLineTitle) {
                TimeLineDateWidget *w = static_cast<TimeLineDateWidget *>(this->indexWidget(idx));
                if (w) {
                    w->onTimeLinePicSelectAll(true);
                }
            } else if (data.itemType == ItemTypeImportTimeLineTitle) {
                importTimeLineDateWidget *w = static_cast<importTimeLineDateWidget *>(this->indexWidget(idx));
                if (w) {
                    w->onTimeLinePicSelectAll(true);
                }
            }
        }
    }
}

void ThumbnailListView::reloadImage()
{
    //加载上下两百张
    if (m_loadTimer == nullptr) {
        m_loadTimer = new QTimer(this);
        m_loadTimer->setInterval(50);
        m_loadTimer->setSingleShot(true);
        connect(m_loadTimer, &QTimer::timeout, this, [ = ] {
            QModelIndex load = indexAt(QPoint(100, 100));
            if (!load.isValid())
            {
                load = indexAt(QPoint(120, 130));
            }
            if (!load.isValid())
            {
                load = indexAt(QPoint(120, 140));
            }
            if (!load.isValid())
            {
                load = m_model->index(0, 0);
            }
            if (!load.isValid())
            {
                return;
            }
            QStringList pathlist;
            int count = 0;
            for (int i = load.row(); i >= 0; i--)
            {
                QModelIndex index = m_model->index(i, 0);
                DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
                pathlist << data.filePath;
                count++;
                if (count >= 200) {
                    break;
                }
            }
            count = 0;
            for (int i = load.row(); i < m_model->rowCount(); i++)
            {
                QModelIndex index = m_model->index(i, 0);
                DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
                pathlist << data.filePath;
                count++;
                if (count >= 200) {
                    break;
                }
            }
            qDebug() << __FUNCTION__ << "---";
            ImageDataService::instance()->readThumbnailByPaths(pathlist);
            this->update();
        });
    }
    if (m_loadTimer->isActive()) {
        m_loadTimer->stop();
    }
    m_loadTimer->start();
}

void ThumbnailListView::slotLoad80ThumbnailsFinish()
{
    // 将缩略图信息插入到listview中
    int size = ImageEngineApi::instance()->m_AllImageDataVector.size();
    if (size <= 0) {
        //通知所有照片界面刷新，如果没有图片则显示导入界面
        emit sigDBImageLoaded();
    }
    //加空白栏
    if (m_delegatetype == ThumbnailDelegate::AllPicViewType) {
        insertBlankOrTitleItem(ItemTypeBlank, "", "", AllPicView::SUSPENSION_WIDGET_HEIGHT);
    }
    for (int i = 0; i < ImageEngineApi::instance()->m_FirstPageScreen && i < size; i++) {
        ImageDataSt data = ImageEngineApi::instance()->m_AllImageDataVector[i];
        DBImgInfo info = data.dbi;
        if (data.imgpixmap.isNull()) {
            info.bNotSupportedOrDamaged = true;
            info.damagedPixmap = getDamagedPixmap();
        }
        info.image = data.imgpixmap;
        ImageEngineApi::instance()->m_AllImageMap[info.filePath] = info.image;
        ImageEngineApi::instance()->m_AllImageData[info.filePath].imgpixmap = info.image;
        //        info.bNotSupportedOrDamaged = data.imgpixmap.isNull();
        info.remainDays = data.remainDays;
        info.imgWidth = m_onePicWidth;
        info.imgHeight = m_onePicWidth;
        info.itemType = data.dbi.itemType;
        info.videoDuration = data.dbi.videoDuration;
        insertThumbnail(info);
    }
}

void ThumbnailListView::slotOneImgReady(const QString &path, QPixmap pix)
{
    for (int i = 0; i < m_model->rowCount(); i++) {
        QModelIndex index = m_model->index(i, 0);
        DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
        if (data.filePath == path) {
            data.image = pix;
            cutPixmap(data);
            QVariant meta;
            meta.setValue(data);
            ImageEngineApi::instance()->m_AllImageMap[data.filePath] = data.image;
            ImageEngineApi::instance()->m_AllImageData[data.filePath].imgpixmap = pix;
            m_model->setData(index, meta, Qt::DisplayRole);
            break;
        }
    }
}

void ThumbnailListView::onScrollTimerOut()
{
//    disconnect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, &ThumbnailListView::onScrollbarValueChanged);
//    QPropertyAnimation *animation = new QPropertyAnimation(verticalScrollBar(), "value");
//    int value = verticalScrollBar()->value();
//    animation->setDuration(ANIMATION_DRLAY);
//    animation->setEasingCurve(QEasingCurve::InOutQuad);//OutCubic  InOutQuad
//    animation->setStartValue(value);
//    animation->setEndValue(value + 10);
//    connect(animation, &QPropertyAnimation::finished,
//            animation, &QPropertyAnimation::deleteLater);
//    connect(animation, &QPropertyAnimation::finished, this, [ = ]() {
//        connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, &ThumbnailListView::onScrollbarValueChanged);
//    });
//    animation->start();

    disconnect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, &ThumbnailListView::onScrollbarValueChanged);
    m_animation->start();
    connect(m_animation, &QPropertyAnimation::finished, this, [ = ]() {
        connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, &ThumbnailListView::onScrollbarValueChanged);
    });


}

void ThumbnailListView::onSyncListviewModelData(QStringList paths, QString albumName, int actionType)
{
    if (paths.count() < 1)
        return ;
    if (sender() != this) {
        // listview存在多个，状态model不同，所以先处理已知的，再通知其他model
        // 本listview对象不再处理其他listviw对象model的同步问题！
        if (actionType == IdRemoveFromAlbum) {
            // remove from album
            if (paths.count() == 1 && !this->getFileList().contains(paths.at(0)))
                return;
            for (int i = 0; i < m_model->rowCount(); i++) {
                QModelIndex idx = m_model->index(i, 0);
                DBImgInfo info = idx.data(Qt::DisplayRole).value<DBImgInfo>();
                for (int j = 0; j < paths.count(); j++) {
                    if (info.filePath == paths.at(j)) {
                        QStringList datas = idx.model()->data(idx, Qt::UserRole + 2).toStringList();
                        datas.removeAll(albumName);
                        QMap<int, QVariant> tempData;
                        tempData.insert(Qt::UserRole + 2, datas);
                        m_model->setItemData(idx, tempData);
                    }
                }
            }
        } else if (actionType == IdAddToAlbum) {
            // add to album
            if (paths.count() == 1 && !this->getFileList().contains(paths.at(0)))
                return;
            for (int i = 0; i < m_model->rowCount(); i++) {
                QModelIndex idx = m_model->index(i, 0);
                DBImgInfo info = idx.data(Qt::DisplayRole).value<DBImgInfo>();
                for (int j = 0; j < paths.count(); j++) {
                    if (info.filePath == paths.at(j)) {
                        QStringList datas = idx.model()->data(idx, Qt::UserRole + 2).toStringList();
                        datas.append(albumName);
                        QMap<int, QVariant> tempData;
                        tempData.insert(Qt::UserRole + 2, datas);
                        m_model->setItemData(idx, tempData);
                    }
                }
            }
        } else if (actionType == IdMoveToTrash) {
            // delete photos
            //            if (paths.count() == 1 && !this->getAllFileList().contains(paths.at(0)))
            //                return;
            //            for (int i = 0; i < m_model->rowCount(); i++) {
            //                QModelIndex idx = m_model->index(i, 0);
            //                DBImgInfo info = idx.data(Qt::DisplayRole).value<DBImgInfo>();
            //                for (int j = 0; j < paths.count(); j++) {
            //                    if (info.path == paths.at(j)) {
            //                        QStringList datas;
            //                        datas.clear();
            //                        QMap<int, QVariant> tempData;
            //                        tempData.insert(Qt::UserRole + 2, datas);
            //                        m_model->setItemData(idx, tempData);
            //                    }
            //                }
            //            }
        }
    }
}

void ThumbnailListView::onScrollbarValueChanged(int value)
{
    Q_UNUSED(value)
    //加载上下两百张
    reloadImage();
    //滚动条向下滑动
    if (m_animation && m_animation->state() == QPropertyAnimation::Running) {
        m_animation->stop();
    }
    if (m_scrollTimer->isActive()) {
        m_scrollTimer->stop();
    }

    QModelIndex index = this->indexAt(QPoint(80, m_blankItemHeight));
    DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();

    //无选中照片，直接返回
    QModelIndexList list = selectionModel()->selectedIndexes();
    if (list.isEmpty()) {
        emit sigTimeLineDataAndNum(data.date, data.num, QObject::tr("Select"));
        return;
    }

    //先判断当前的类型，是标题则获取是否全选；是图片，则找到它所在时间线的数量和时间以及是否全选
    if (data.itemType == ItemTypeBlank || data.itemType == ItemTypeTimeLineTitle
            || data.itemType == ItemTypeImportTimeLineTitle) {
        bool isSelect = getCurrentIndexSelectStatus(index, false);
        emit sigTimeLineDataAndNum(data.date, data.num, isSelect ? QObject::tr("Unselect") : QObject::tr("Select"));
    } else {
        bool isSelect = getCurrentIndexSelectStatus(index, true);
        QStringList list = getCurrentIndexTime(index);
        if (list.size() == 2) {
            emit sigTimeLineDataAndNum(list.at(0), list.at(1), isSelect ? QObject::tr("Unselect") : QObject::tr("Select"));
        }
    }
//TODO 时间上滑动画
//    if (data.itemType == ItemTypeTimeLineTitle || data.itemType == ItemTypeBlank || data.itemType == ItemTypeImportTimeLineTitle) {
//        if (!m_animationEnable) {
//            //计算滚动距离，使用动画滑动
//            QRect rect = rectForIndex(index);
//            if (m_animation == nullptr) {
//                m_animation = new QPropertyAnimation(verticalScrollBar(), "value", this);
//                m_animation->setDuration(ANIMATION_DRLAY);
//                m_animation->setEasingCurve(QEasingCurve::InOutQuad);
//            }
//            m_animation->setStartValue(verticalScrollBar()->value());
//            m_animation->setEndValue(rect.top());

//            disconnect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, &ThumbnailListView::onScrollbarValueChanged);
//            m_animation->start();
//            connect(m_animation, &QPropertyAnimation::finished, this, [ = ]() {
//                connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, &ThumbnailListView::onScrollbarValueChanged);
//            });
//        }
//    }
}

void ThumbnailListView::onDoubleClicked(const QModelIndex &index)
{
    if (m_isSelectAllBtn) {
        return;
    }
    if (ALBUM_PATHTYPE_BY_PHONE != m_imageType && m_imageType.compare(COMMON_STR_TRASH) != 0) {
        DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
        if (data.itemType == ItemTypeBlank
                || data.itemType == ItemTypeTimeLineTitle
                || data.itemType == ItemTypeImportTimeLineTitle) {
            return;
        }
        emit openImage(index.row(), data.filePath, false);
    }
}

void ThumbnailListView::onClicked(const QModelIndex &index)
{
    if (m_isSelectAllBtn) {
        return;
    }
    DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
    qDebug() << __FUNCTION__ << "---" << ImageDataService::instance()->imageIsLoaded(data.filePath);
    qDebug() << __FUNCTION__ << "---" << ImageDataService::instance()->getMovieDurationStrByPath(data.filePath);
#ifdef tablet_PC
    if (activeClick && ALBUM_PATHTYPE_BY_PHONE != m_imageType) {
        if (m_imageType.compare(COMMON_STR_TRASH) != 0) {
            emit openImage(index.row());
        }
    }
#endif
    Q_UNUSED(index)
}

//裁剪图片尺寸
void ThumbnailListView::cutPixmap(DBImgInfo &DBImgInfo)
{
    int width = DBImgInfo.image.width();
    if (width == 0)
        width = m_iBaseHeight;
    int height = DBImgInfo.image.height();
    if (abs((width - height) * 10 / width) >= 1) {
        QRect rect = DBImgInfo.image.rect();
        int x = rect.x() + width / 2;
        int y = rect.y() + height / 2;
        if (width > height) {
            x = x - height / 2;
            y = 0;
            DBImgInfo.image = DBImgInfo.image.copy(x, y, height, height);
        } else {
            y = y - width / 2;
            x = 0;
            DBImgInfo.image = DBImgInfo.image.copy(x, y, width, width);
        }
    }
}

void ThumbnailListView::timeLimeFloatBtnClicked(const QString &date, bool isSelect)
{
    QString tmpdate = date;
    for (int i = 0; i < m_model->rowCount(); i++) {
        QModelIndex index = m_model->index(i, 0);
        DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
        //循环找到时间
        if (data.itemType == ItemTypePic || data.itemType == ItemTypeVideo || data.itemType == ItemTypeMountImg) {
            continue;
        }
        //时间线
        if (m_delegatetype == ThumbnailDelegate::TimeLineViewType && (data.itemType == ItemTypeTimeLineTitle || data.itemType == ItemTypeBlank)
                && data.date == tmpdate) {
            //先同步设置本时间线选择按钮状态
            TimeLineDateWidget *w = static_cast<TimeLineDateWidget *>(this->indexWidget(index));
            if (w) {
                w->onTimeLinePicSelectAll(isSelect);
            }
            //取选中范围
            bool bFirstSelect = true;
            QModelIndex first_idx, end_idx;
            for (int j = i + 1; j < m_model->rowCount(); j++) {
                index = m_model->index(j, 0);
                DBImgInfo pdata = index.data(Qt::DisplayRole).value<DBImgInfo>();
                if (pdata.itemType  == ItemTypeTimeLineTitle) {
                    break;//到达下一个时间线，跳出
                }
                if (pdata.itemType  == ItemTypePic) {
                    if (bFirstSelect) {
                        first_idx = index;
                        bFirstSelect = false;
                    }
                    end_idx = index;
                }
            }
            QItemSelection selection(first_idx, end_idx);
            if (isSelect) {//设置选中还是取消选中
                selectionModel()->select(selection, QItemSelectionModel::Select);
            } else {
                selectionModel()->select(selection, QItemSelectionModel::Deselect);
            }
            break;
        } else if (m_delegatetype == ThumbnailDelegate::AlbumViewImportTimeLineViewType && (data.itemType == ItemTypeImportTimeLineTitle || data.itemType == ItemTypeBlank)
                   && data.date == tmpdate) { //已导入时间线
            //先同步设置本时间线选择按钮状态
            importTimeLineDateWidget *w = static_cast<importTimeLineDateWidget *>(this->indexWidget(index));
            if (w) {
                w->onTimeLinePicSelectAll(isSelect);
            }
            //取选中范围
            bool bFirstSelect = true;
            QModelIndex first_idx, end_idx;
            for (int j = i + 1; j < m_model->rowCount(); j++) {
                index = m_model->index(j, 0);
                DBImgInfo pdata = index.data(Qt::DisplayRole).value<DBImgInfo>();
                if (pdata.itemType  == ItemTypeImportTimeLineTitle) {
                    break;//到达下一个时间线，跳出
                } else if (pdata.itemType  == ItemTypePic) {
                    if (bFirstSelect) {
                        first_idx = index;
                        bFirstSelect = false;
                    }
                    end_idx = index;
                }
            }
            QItemSelection selection(first_idx, end_idx);
            if (isSelect) {//设置选中还是取消选中
                selectionModel()->select(selection, QItemSelectionModel::Select);
            } else {
                selectionModel()->select(selection, QItemSelectionModel::Deselect);
            }
            break;//已完成，跳出循环，后面index的不需要处理
        }
    }
}

void ThumbnailListView::resizeEventF()
{
    int i_totalwidth = width() - 30;
    //计算一行的个数
    m_rowSizeHint = i_totalwidth / (m_iBaseHeight + ITEM_SPACING);
    int currentwidth = (i_totalwidth - ITEM_SPACING * (m_rowSizeHint - 1)) / m_rowSizeHint;//一张图的宽度
    m_onePicWidth = currentwidth;
    if (currentwidth < 80)
        currentwidth = 80;

    if (nullptr != m_item) {
        m_item->setSizeHint(QSize(this->width(), getListViewHeight() + 8 + 27)/*this->size()*/);
        this->resize(QSize(this->width(), m_height + 27 + 8)/*this->size()*/);
    }
    if (m_delegate) {
        m_delegate->setItemSize(QSize(m_onePicWidth, m_onePicWidth));
    }
    this->setSpacing(ITEM_SPACING);
}
