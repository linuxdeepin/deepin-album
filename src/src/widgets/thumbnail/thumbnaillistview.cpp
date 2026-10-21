// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumbnaillistview.h"
#include "ddlog.h"
//#include "application.h"

#include <DApplication>

#include <QDebug>
#include <QDrag>
#include <QFileInfo>
#include <QImageReader>
#include <QMimeData>
#include <QScrollBar>
#include <QMutex>
#include <QScroller>
#include <QPropertyAnimation>
#include <QDesktopServices>
#include <QProcess>
#include <QElapsedTimer>

//#include "controller/signalmanager.h"
//#include "controller/wallpapersetter.h"
//#include "widgets/dialogs/imgdeletedialog.h"
#include "unionimage/baseutils.h"
#include "unionimage/imageutils.h"
#include "unionimage/unionimage.h"
#include "globalstatus.h"
#include "albumControl.h"
//#include "imageengine/imageengineapi.h"
//#include "imageengine/imageenginethread.h"
//#include "ac-desktop-define.h"
//#include "allpicview/allpicview.h"
#include "imageengine/imagedataservice.h"
//#include "batchoperatewidget.h"

namespace {
const int ITEM_SPACING = 4;
const int BASE_HEIGHT = 100;
const int MINIMUN_WIDTH = 442;
const int STATUSBAR_HEIGHT = 27;
const int RIGHT_MARGIN = 10;

// const QString IMAGE_DEFAULTTYPE = "All pics";
const QString IMAGE_DEFAULTTYPE = "All Photos";
const QString SHORTCUTVIEW_GROUP = "SHORTCUTVIEW";

using namespace Libutils::common;

QString ss(const QString &text)
{
    QString str/* = dApp->setter->value(SHORTCUTVIEW_GROUP, text).toString()*/;
    str.replace(" ", "");

    return str;
}
}  // namespace

ThumbnailListView::ThumbnailListView(ThumbnailDelegate::DelegateType type, int UID, const QString &imgtype, QWidget *parent)
    :  DListView(parent), m_delegatetype(type), m_allfileslist(), updateEnableSelectionByMouseTimer(nullptr)
{
    qCDebug(albumLog) << "Initializing ThumbnailListView with type:" << type << "UID:" << UID << "imageType:" << imgtype;
    this->setObjectName("ThumbnailListView");
    this->setAccessibleName("ThumbnailListView");
    m_model = new QStandardItemModel(this);
    m_imageType = imgtype;
    m_currentUID = UID;
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
    setMinimumWidth(MINIMUN_WIDTH);
    m_delegate = new ThumbnailDelegate(type, this); //绑在this上面，用于自动销毁
    m_delegate->m_imageTypeStr = m_imageType;
    setItemDelegate(m_delegate);
    setModel(m_model);
    m_pMenu = new DMenu();
    m_pMenu->setObjectName("PMenu");
    m_pMenu->setAccessibleName("PMenu");
    setViewportMargins(0, 0, 0, 0);
    initMenuAction();
    initConnections();
    installEventFilter(this);
    verticalScrollBar()->installEventFilter(this);

    updateEnableSelectionByMouseTimer = new QTimer(this);
    updateEnableSelectionByMouseTimer->setInterval(500);
    connect(updateEnableSelectionByMouseTimer, &QTimer::timeout, [ = ]() {
        if (touchStatus == 0) { //时间到了还在等待模式,则进入框选模式
            qCDebug(albumLog) << "Touch status timeout - entering selection mode";
            touchStatus = 2;
        }
        updateEnableSelectionByMouseTimer->stop();
    });

    m_scrollTimer = new QTimer(this);
    m_scrollTimer->setInterval(100);
    m_scrollTimer->setSingleShot(true);
    connect(m_scrollTimer, &QTimer::timeout, this, &ThumbnailListView::onScrollTimerOut);

    m_importTimer = new QTimer(this);
    connect(m_importTimer, &QTimer::timeout, this, [this]() {
        this->update();
        m_importActiveCount--;
        if (m_importActiveCount <= 0) {
            qCDebug(albumLog) << "Import timer completed";
            m_importTimer->stop();
        }
    });

    connect(ImageDataService::instance(), &ImageDataService::sigeUpdateListview, this, &ThumbnailListView::onUpdateListview, Qt::ConnectionType::QueuedConnection);
    connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, &ThumbnailListView::onScrollbarValueChanged);
    connect(selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ThumbnailListView::onSelectionChanged);

    connect(GlobalStatus::instance(), &GlobalStatus::sigKeyPressFromQml, this, &ThumbnailListView::onKeyPressFromQml);

    viewport()->setAttribute(Qt::WA_Hover);
    qCDebug(albumLog) << "ThumbnailListView initialization completed";
}

ThumbnailListView::~ThumbnailListView()
{
    // qCDebug(albumLog) << "Destroying ThumbnailListView";
    if (m_loadTimer) {
        if (m_loadTimer->isActive()) {
            // qCDebug(albumLog) << "Stopping active load timer";
            m_loadTimer->stop();
        }
        m_loadTimer->deleteLater();
    }
    // qCDebug(albumLog) << "ThumbnailListView::~ThumbnailListView - Exit: destruction completed";
}

static QString myMimeType()
{
    // qCDebug(albumLog) << "ThumbnailListView::~ThumbnailListView - Entry: destruction completed";
    return QStringLiteral("TestListView/text-icon-icon_hover");
}

void ThumbnailListView::mousePressEvent(QMouseEvent *event)
{
    // qCDebug(albumLog) << "ThumbnailListView::mousePressEvent - Entry";
    QTime curTime = QTime::currentTime();
    // qCDebug(albumLog) << "Mouse press event at position:" << event->pos() << "button:" << event->button();

    //点击时将焦点设置到当前
    setFocus();
    //复位激活click
    activeClick = true;
    // 当事件source为MouseEventSynthesizedByQt，认为此事件为TouchBegin转换而来
    if (event->source() == Qt::MouseEventSynthesizedByQt) {
        // qCDebug(albumLog) << "Touch event detected - setting touch begin position";
        lastTouchBeginPos = event->pos();
        // qCDebug(albumLog) << "Touch event detected at position:" << lastTouchBeginPos;

        // 清空触屏滚动操作，因为在鼠标按下时还不知道即将进行的是触屏滚动还是文件框选
        if (QScroller::hasScroller(this)) {
            // qCDebug(albumLog) << "Clearing existing scroller";
            // 不可使用 ungrab，会导致应用崩溃，或许是Qt的bug
            QScroller::scroller(this)->deleteLater();
        }

        updateEnableSelectionByMouseTimer->start();
    }

    if ((QApplication::keyboardModifiers() == Qt::ShiftModifier && event->button() == Qt::LeftButton)) {
        // qCDebug(albumLog) << "Shift+Left click detected";
        // 最近删除界面单指循环选中、取消选中
        if (selectionModel()->selectedIndexes().contains(this->indexAt(event->pos()))
                && selectionModel()->selectedIndexes().size() == 1 && m_imageType == COMMON_STR_TRASH) {
            if (event->button() != Qt::MouseButton::RightButton) { // 优化最近删除界面,长按时不清除选中
                // qCDebug(albumLog) << "Clearing selection in trash view";
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
            // qCDebug(albumLog) << "Disabling drag for import timeline title";
            setDragEnabled(false);
        } else if (dragDropMode() != NoDragDrop) {
            // qCDebug(albumLog) << "Enabling drag drop mode";
            setDragDropMode(DragDrop);
        }
    } else {
        // qCDebug(albumLog) << "Disabling drag for special view types";
        setDragEnabled(false);
    }
    m_pressed = event->pos();

    if (event->button() == Qt::LeftButton) {
        if (m_lastPressTime.msecsTo(curTime) < 200) {
            // qCDebug(albumLog) << "Double click detected";
            emit doubleClicked(index);
        }
        m_lastPressTime = curTime;
    }

    DListView::mousePressEvent(event);
    // qCDebug(albumLog) << "ThumbnailListView::mousePressEvent - Exit: event processed";
}

void ThumbnailListView::mouseMoveEvent(QMouseEvent *event)
{
    // qCDebug(albumLog) << "ThumbnailListView::mouseMoveEvent - Entry: pos=" << event->pos();
    QRectF rect(QPointF((lastTouchBeginPos.x() - 30), (lastTouchBeginPos.y() - 30)),
                QPointF((lastTouchBeginPos.x() + 30), (lastTouchBeginPos.y() + 30)));
    if (rect.contains(event->pos())) {
        // qCDebug(albumLog) << "Mouse move within touch tolerance area - ignoring";
        return;
    }

    if (touchStatus == 0) {
        // qCDebug(albumLog) << "Touch status changed to 1";
        touchStatus = 1;
        activeClick = false;
    }

    if (touchStatus == 1) {
        if (event->source() == Qt::MouseEventSynthesizedByQt) {
            // qCDebug(albumLog) << "Touch move event detected";
        }
    }

    emit sigMouseMove();
    DListView::mouseMoveEvent(event);
    // qCDebug(albumLog) << "ThumbnailListView::mouseMoveEvent - Exit: event processed";
}

void ThumbnailListView::startDrag(Qt::DropActions supportedActions)
{
    return;
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
    qCDebug(albumLog) << "ThumbnailListView show event";
    Q_UNUSED(event);
    int i_totalwidth = width() - RIGHT_MARGIN;
    //计算一行的个数
    m_rowSizeHint = i_totalwidth / (m_iBaseHeight + ITEM_SPACING);
    m_onePicWidth = (i_totalwidth - ITEM_SPACING * (m_rowSizeHint - 1)) / m_rowSizeHint;//一张图的宽度
    qCDebug(albumLog) << "Calculated row size:" << m_rowSizeHint << "pic width:" << m_onePicWidth;
}

void ThumbnailListView::hideEvent(QHideEvent *event)
{
    // qCDebug(albumLog) << "ThumbnailListView::hideEvent - Entry: widget hiding";
    // 控件隐藏时，若有焦点，清空焦点，可以避免焦点自动跳转到下一控件
    QWidget* fw = QApplication::focusWidget();
    if (fw == this) {
        // qCDebug(albumLog) << "Clearing focus from hidden widget";
        this->clearFocus();
    }

    DListView::hideEvent(event);
    // qCDebug(albumLog) << "ThumbnailListView::hideEvent - Exit: widget hidden";
}

void ThumbnailListView::wheelEvent(QWheelEvent *event)
{
    // qCDebug(albumLog) << "ThumbnailListView::wheelEvent - Entry: delta=" << event->angleDelta().y();
    if (event->angleDelta().y() < 0) {
        //向下滑时才可执行
        // qCDebug(albumLog) << "Wheel down - disabling animation";
        m_animationEnable = false;
    } else {
        // qCDebug(albumLog) << "Wheel up - enabling animation";
        m_animationEnable = true;
    }

    if (DApplication::keyboardModifiers() == Qt::ControlModifier) {
        // qCDebug(albumLog) << "Ctrl+Wheel detected - zooming";
        GlobalStatus::instance()->sigZoomInOutFromQWidget(event->angleDelta().y());
    }

    DListView::wheelEvent(event);
    // qCDebug(albumLog) << "ThumbnailListView::wheelEvent - Exit: wheel event processed";
}

QRect ThumbnailListView::visualRect(const QModelIndex &index) const
{
    // qCDebug(albumLog) << "ThumbnailListView::visualRect - Entry: index=" << index;
    //获取当前视图中的item项,找到视图中的index,注意：获取到的index可能不是最大值,但已经足够了
    return QListView::visualRect(index);
}

void ThumbnailListView::sltReloadAfterFilterEnd(const QVector<DBImgInfo> &allImageDataVector)
{
    qCDebug(albumLog) << "Reloading after filter with" << allImageDataVector.size() << "items";
    if (m_delegatetype != ThumbnailDelegate::AllPicViewType) {
        qCDebug(albumLog) << "Skipping reload - not AllPicViewType";
        return;
    }

    for (int i = (m_model->rowCount() - 1); i >= 0; i--) {
        QModelIndex index = m_model->index(i, 0);
        DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
        if ((data.itemType == ItemTypePic || data.itemType == ItemTypeVideo) && !QFile::exists(data.filePath)) {
            qCDebug(albumLog) << "Removing non-existent file:" << data.filePath;
            m_model->removeRow(index.row());
        }
    }

    for (int j = (m_model->rowCount() - 1); j < allImageDataVector.size(); j++) {
        insertThumbnail(allImageDataVector.at(j));
    }

    // 添加底栏空白区域
    insertBlankOrTitleItem(ItemTypeBlank, "", "", STATUSBAR_HEIGHT);

    //加载完后通知所有图片页刷新状态栏显示数量
    emit sigUpdatePicNum();
    qCDebug(albumLog) << "Reload completed";
}

void ThumbnailListView::onUpdateListview()
{
    // qCDebug(albumLog) << "ThumbnailListView::onUpdateListview - Entry";
    if (this->isVisible()) {
        // qCDebug(albumLog) << "Updating listview";
        this->update();
    }
}

void ThumbnailListView::mouseReleaseEvent(QMouseEvent *event)
{
    // qCDebug(albumLog) << "ThumbnailListView::mouseReleaseEvent - Entry";
    //触屏状态复位
    touchStatus = 0;
    updateEnableSelectionByMouseTimer->stop();
    m_animationEnable = false;

    if (COMMON_STR_RECENT_IMPORTED  == m_imageType) {
        // qCDebug(albumLog) << "COMMON_STR_RECENT_IMPORTED";
        if (QApplication::keyboardModifiers() == Qt::NoModifier) {
            emit sigMouseRelease();
        }
    } else {
        // qCDebug(albumLog) << "COMMON_STR_RECENT_IMPORTED";
        emit sigMouseRelease();
    }

    DListView::mouseReleaseEvent(event);

    //每次选中需要更新右键菜单，以便快捷键响应，多选不响应，单选响应
    //BUG#101481 调整为在list view响应事件后刷新菜单
    updateMenuContents();
    // qCDebug(albumLog) << "ThumbnailListView::mouseReleaseEvent - Exit";
}

void ThumbnailListView::keyPressEvent(QKeyEvent *event)
{
    // qCDebug(albumLog) << "ThumbnailListView::keyPressEvent - Entry";
    DListView::keyPressEvent(event);
    m_dragItemPath = selectedPaths();
    int id = -1;
    if (event->key() == Qt::Key_F11) {
        id = Types::IdFullScreen;
    } else if (event->key() == Qt::Key_F5) {
        id = Types::IdStartSlideShow;
    } else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        id = Types::IdView;
    } else if (event->key() == Qt::Key_Period) {
        if (AlbumControl::instance()->canFavorite(m_dragItemPath))
            id = Types::IdAddToFavorites;
        else
            id = Types::IdRemoveFromFavorites;
    }else if (event->modifiers() == Qt::ControlModifier) {
        if (event->key() == Qt::Key_E) {
            id =Types::IdExport;
        } else if (event->key() == Qt::Key_I) {
            if (m_dragItemPath.isEmpty())
                return;
            if (Libutils::image::isVideo(LibUnionImage_NameSpace::localPath(m_dragItemPath.first())))
                id = Types::IdVideoInfo;
            else
                id = Types::IdImageInfo;
        } else if (event->key() == Qt::Key_F9) {
            id = Types::IdSetAsWallpaper;
        }
    } else if (event->modifiers() == Qt::AltModifier) {
        if (event->key() == Qt::Key_D)
            id = Types::IdDisplayInFileManager;
    }

    if (id != -1) {
        // qCDebug(albumLog) << "Emitting menu item clicked signal with id:" << id;
        GlobalStatus::instance()->sigMenuItemClickedFromQWidget(id);
    }
    // qCDebug(albumLog) << "ThumbnailListView::keyPressEvent - Exit";
}

void ThumbnailListView::dragEnterEvent(QDragEnterEvent *event)
{
    // qCDebug(albumLog) << "ThumbnailListView::dragEnterEvent - Entry";
    if (event->mimeData()->hasFormat(myMimeType())) {
        // qCDebug(albumLog) << "Mime data has format";
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            if (!Libutils::base::checkMimeUrls(event->mimeData()->urls())) {
                qCWarning(albumLog) << "Invalid mime URLs in drag enter event";
                return;
            }
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
    // qCDebug(albumLog) << "ThumbnailListView::dragEnterEvent - Exit";
}

void ThumbnailListView::dragMoveEvent(QDragMoveEvent *event)
{
    // qCDebug(albumLog) << "ThumbnailListView::dragMoveEvent - Entry";
    if (event->mimeData()->hasFormat(myMimeType())) {
        // qCDebug(albumLog) << "Mime data has format";
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            if (!Libutils::base::checkMimeUrls(event->mimeData()->urls())) {
                qCWarning(albumLog) << "Invalid mime URLs in drag move event";
                return;
            }
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
    // qCDebug(albumLog) << "ThumbnailListView::dragMoveEvent - Exit";
}

void ThumbnailListView::dragLeaveEvent(QDragLeaveEvent *event)
{
    // qCDebug(albumLog) << "ThumbnailListView::dragLeaveEvent - Entry";
    m_dragItemPath = selectedPaths();
    DListView::dragLeaveEvent(event);
}

void ThumbnailListView::dropEvent(QDropEvent *event)
{
    // qCDebug(albumLog) << "ThumbnailListView::dropEvent - Entry";
    if (event->mimeData()->hasFormat("TestListView/text-icon-icon_hover"))
        return;
    DListView::dropEvent(event);
    // qCDebug(albumLog) << "ThumbnailListView::dropEvent - Exit";
}

void ThumbnailListView::initConnections()
{
    qCDebug(albumLog) << "Initializing ThumbnailListView connections";
    connect(ImageDataService::instance(), &ImageDataService::sigeUpdateListview, this, &ThumbnailListView::onUpdateListview, Qt::ConnectionType::QueuedConnection);
    connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, &ThumbnailListView::onScrollbarValueChanged);
    connect(this, &QListView::customContextMenuRequested, this, &ThumbnailListView::onShowMenu);
    connect(m_pMenu, &DMenu::triggered, this, &ThumbnailListView::onMenuItemClicked);
    connect(this, &ThumbnailListView::doubleClicked, this, &ThumbnailListView::onDoubleClicked);
    connect(this, &ThumbnailListView::clicked, this, &ThumbnailListView::onClicked);
    connect(GlobalStatus::instance(), &GlobalStatus::cellBaseWidthChanged, this, &ThumbnailListView::onCellBaseWidthChanged);
    connect(m_delegate, &ThumbnailDelegate::sigCancelFavorite, this, &ThumbnailListView::onCancelFavorite);
    qCDebug(albumLog) << "ThumbnailListView connections initialized";
}

void ThumbnailListView::insertThumbnail(const DBImgInfo &dBImgInfo)
{
    qCDebug(albumLog) << "Inserting thumbnail for file:" << dBImgInfo.filePath;
    DBImgInfo info = dBImgInfo;

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
    item->setData(QVariant(QSize(info.imgWidth, height)),
                  Qt::SizeHintRole);
    m_model->appendRow(item);
    if (info.itemType == ItemTypeTimeLineTitle) {
        item->setSelectable(false);
        QModelIndex index = m_model->indexFromItem(item);
        TimeLineDateWidget *pCurrentDateWidget = new TimeLineDateWidget(item, info.date, info.num);
        connect(pCurrentDateWidget, &TimeLineDateWidget::sigIsSelectCurrentDatePic, this, &ThumbnailListView::slotSelectCurrentDatePic);
        connect(this, &ThumbnailListView::sigShowCheckBox, pCurrentDateWidget, &TimeLineDateWidget::onShowCheckBox);
        this->setIndexWidget(index, pCurrentDateWidget);
    } else if (info.itemType == ItemTypeImportTimeLineTitle) {
        item->setSelectable(false);
        QModelIndex index = m_model->indexFromItem(item);
        importTimeLineDateWidget *pCurrentDateWidget = new importTimeLineDateWidget(item, info.date, info.num);
        connect(pCurrentDateWidget, &importTimeLineDateWidget::sigIsSelectCurrentDatePic, this, &ThumbnailListView::slotSelectCurrentDatePic);
        connect(this, &ThumbnailListView::sigShowCheckBox, pCurrentDateWidget, &importTimeLineDateWidget::onShowCheckBox);
        this->setIndexWidget(index, pCurrentDateWidget);
    }
    qCDebug(albumLog) << "Thumbnail inserted successfully";
}

void ThumbnailListView::insertThumbnails(const DBImgInfoList &infos)
{
    qCDebug(albumLog) << "ThumbnailListView::insertThumbnails - Entry: count=" << infos.size();
    std::vector<std::pair<QModelIndex, DWidget *>> timelines;

    for (const auto &dBImgInfo : infos) {
        DBImgInfo info = dBImgInfo;

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
        item->setData(QVariant(QSize(info.imgWidth, height)),
                      Qt::SizeHintRole);
        m_model->appendRow(item);
        if (info.itemType == ItemTypeTimeLineTitle) {
            item->setSelectable(false);
            QModelIndex index = m_model->indexFromItem(item);
            TimeLineDateWidget *pCurrentDateWidget = new TimeLineDateWidget(item, info.date, info.num);
            connect(pCurrentDateWidget, &TimeLineDateWidget::sigIsSelectCurrentDatePic, this, &ThumbnailListView::slotSelectCurrentDatePic);
            connect(this, &ThumbnailListView::sigShowCheckBox, pCurrentDateWidget, &TimeLineDateWidget::onShowCheckBox);
            timelines.push_back(std::make_pair(index, pCurrentDateWidget));
        } else if (info.itemType == ItemTypeImportTimeLineTitle) {
            item->setSelectable(false);
            QModelIndex index = m_model->indexFromItem(item);
            importTimeLineDateWidget *pCurrentDateWidget = new importTimeLineDateWidget(item, info.date, info.num);
            connect(pCurrentDateWidget, &importTimeLineDateWidget::sigIsSelectCurrentDatePic, this, &ThumbnailListView::slotSelectCurrentDatePic);
            connect(this, &ThumbnailListView::sigShowCheckBox, pCurrentDateWidget, &importTimeLineDateWidget::onShowCheckBox);
            timelines.push_back(std::make_pair(index, pCurrentDateWidget));
        }
    }

    //最后再来统一插入控件，速度要快个几十倍
    for (const auto &eachLine : timelines) {
        this->setIndexWidget(eachLine.first, eachLine.second);
    }
    qCDebug(albumLog) << "ThumbnailListView::insertThumbnails - Exit: all thumbnails inserted successfully";
}

void ThumbnailListView::stopLoadAndClear(bool bClearModel)
{
    qCDebug(albumLog) << "ThumbnailListView::stopLoadAndClear - Entry: bClearModel=" << bClearModel;
    if (bClearModel)
        m_model->clear();   //清除模型中的数据
    m_allfileslist.clear();
    qCDebug(albumLog) << "ThumbnailListView::stopLoadAndClear - Exit: cleanup completed";
}
//根据显示类型，返回不同列表，如所有照片返回所有，时间线返回当前时间下内容
QStringList ThumbnailListView::getFileList(int row, ItemType type)
{
    qCDebug(albumLog) << "Getting file list for row:" << row << "type:" << type;
    m_allfileslist.clear();
    if (m_delegatetype == ThumbnailDelegate::AllPicViewType
            || m_delegatetype == ThumbnailDelegate::AlbumViewCustomType
            || m_delegatetype == ThumbnailDelegate::AlbumViewFavoriteType
            || m_delegatetype == ThumbnailDelegate::SearchViewType
            || m_delegatetype == ThumbnailDelegate::AlbumViewPhoneType) {
        //遍历所有数据
        qCDebug(albumLog) << "Processing all files view type";
        for (int i = 0; i < m_model->rowCount(); i++) {
            QModelIndex index = m_model->index(i, 0);
            DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
            if (type == ItemType::ItemTypeNull) {
                if (data.itemType == ItemType::ItemTypePic || data.itemType == ItemType::ItemTypeVideo) {
                    m_allfileslist.append(data.filePath);
                }
            } else {
                if (data.itemType == type) {
                    m_allfileslist.append(data.filePath);
                }
            }
        }
    } else if (m_delegatetype == ThumbnailDelegate::TimeLineViewType
               || m_delegatetype == ThumbnailDelegate::AlbumViewImportTimeLineViewType) {
        //找到当前项前面最近的一个标题
        qCDebug(albumLog) << "Processing timeline view type";
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
                if (type == ItemType::ItemTypeNull) {
                    if (!data.filePath.isEmpty()) {
                        m_allfileslist.append(data.filePath);
                    }
                } else {
                    if (data.itemType == type) {
                        if (!data.filePath.isEmpty()) {
                            m_allfileslist.append(data.filePath);
                        }
                    }
                }
            }
        }
    }
    qCDebug(albumLog) << "Found" << m_allfileslist.size() << "files in list";
    return m_allfileslist;
}

QList<DBImgInfo> ThumbnailListView::getAllFileInfo(int row)
{
    qCDebug(albumLog) << "ThumbnailListView::getAllFileInfo - Entry: row=" << row;
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
    qCDebug(albumLog) << "ThumbnailListView::getAllFileInfo - Exit: returning" << DBImgInfos.size() << "files";
    return DBImgInfos;
}

int ThumbnailListView::getRow(const QString &path)
{
    qCDebug(albumLog) << "ThumbnailListView::getRow - Entry: path=" << path;
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
    qCDebug(albumLog) << "ThumbnailListView::getRow - Exit: row=" << row;
    return row;
}

void ThumbnailListView::onShowMenu(const QPoint &pos)
{
    qCDebug(albumLog) << "ThumbnailListView::onShowMenu - Entry: pos=" << pos;
    //外接设备显示照片时，禁用鼠标右键菜单
    QModelIndex index = this->indexAt(pos);
    if (!index.isValid() || ALBUM_PATHTYPE_BY_PHONE == m_imageType) {
        qCDebug(albumLog) << "Invalid index or phone album type - skipping menu";
        return;
    }
    //标题项和空白项不显示右键菜单
    DBImgInfo info = index.data(Qt::DisplayRole).value<DBImgInfo>();
    if (info.itemType == ItemTypeBlank
            || info.itemType == ItemTypeTimeLineTitle
            || info.itemType == ItemTypeImportTimeLineTitle) {
        qCDebug(albumLog) << "Skipping menu for blank/title item";
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
    qCDebug(albumLog) << "ThumbnailListView::onShowMenu - Exit: menu displayed";
}

void ThumbnailListView::updateMenuContents()
{
    qCDebug(albumLog) << "Updating menu contents";
    QStringList paths = selectedPaths();
    if (paths.empty()) {
        qCDebug(albumLog) << "No paths selected - skipping menu update";
        return;
    }
#ifndef tablet_PC
    if (m_imageType.compare(COMMON_STR_TRASH) == 0) {
        if (1 == paths.length()) {
            DBImgInfo data = selectedIndexes().at(0).data(Qt::DisplayRole).value<DBImgInfo>();
            if (data.itemType == ItemTypePic) {
                m_MenuActionMap.value(tr("Photo info"))->setVisible(true);
                m_MenuActionMap.value(tr("Video info"))->setVisible(false);
            } else {
                m_MenuActionMap.value(tr("Photo info"))->setVisible(false);
                m_MenuActionMap.value(tr("Video info"))->setVisible(true);
            }
        } else {
            m_MenuActionMap.value(tr("Photo info"))->setVisible(false);
            m_MenuActionMap.value(tr("Video info"))->setVisible(false);
        }
        return;
    }
#endif
    for (QAction *action : m_MenuActionMap.values()) {
        action->setVisible(true);
        action->setEnabled(true);
    }
    //源文件不存在时的菜单
    if (1 == paths.length()) {
        QFileInfo info(paths[0]);
        if (!info.exists() && COMMON_STR_TRASH != m_imageType) {
            qCDebug(albumLog) << "File does not exist:" << paths[0];
#ifndef tablet_PC
            m_MenuActionMap.value(tr("View"))->setEnabled(true);
            m_MenuActionMap.value(tr("Fullscreen"))->setEnabled(false);
            m_MenuActionMap.value(tr("Slide show"))->setEnabled(false);
            m_MenuActionMap.value(tr("Export"))->setEnabled(false);
            if (m_albumMenu)
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
            if (QFileInfo(info.dir(), info.dir().path()).isWritable()) {
                m_MenuActionMap.value(tr("Delete"))->setEnabled(true);
            } else {
                m_MenuActionMap.value(tr("Delete"))->setEnabled(false);
            }

            m_MenuActionMap.value(tr("Remove from album"))->setVisible(false);
#ifndef tablet_PC
            m_MenuActionMap.value(tr("Print"))->setVisible(false);
#endif
            if (DBManager::instance()->isImgExistInAlbum(DBManager::SpUID::u_Favorite, paths[0])) {
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
            m_MenuActionMap.value(tr("Video info"))->setVisible(false);
            m_MenuActionMap.value(tr("Set as wallpaper"))->setEnabled(false);
#endif
            return;
        }
#ifndef tablet_PC
        if (!info.permission(QFile::ReadOwner)) {
            m_MenuActionMap.value(tr("Set as wallpaper"))->setEnabled(false);
            m_MenuActionMap.value(tr("Set as wallpaper"))->setVisible(false);
        }
#endif
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
        if (!isSelectedCanUseDelete()) {
            m_MenuActionMap.value(tr("Delete"))->setVisible(false);
        } else {
            m_MenuActionMap.value(tr("Delete"))->setVisible(true);
        }

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

    //非最近删除的单选
    if (1 == paths.length() && COMMON_STR_TRASH != m_imageType) {
        if (DBManager::instance()->isImgExistInAlbum(DBManager::SpUID::u_Favorite, paths[0])) {
            m_MenuActionMap.value(tr("Favorite"))->setVisible(false);
        } else {
            m_MenuActionMap.value(tr("Unfavorite"))->setVisible(false);
        }
        m_pMenu->addSeparator();
    } else {
        if (!m_batchOperateWidget && !DBManager::instance()->isAllImgExistInAlbum(DBManager::SpUID::u_Favorite, paths, AlbumDBType::Favourite)) {
            m_MenuActionMap.value(tr("Unfavorite"))->setVisible(false);
            m_MenuActionMap.value(tr("Favorite"))->setVisible(true);
        } else {
            m_MenuActionMap.value(tr("Favorite"))->setVisible(false);
            m_MenuActionMap.value(tr("Unfavorite"))->setVisible(true);
        }
    }
    //非自定义相册、自动导入路径，隐藏从相册中移除菜单
    if (m_delegatetype != ThumbnailDelegate::AlbumViewCustomType || DBManager::instance()->getAlbumDBTypeFromUID(m_currentUID) == AutoImport) {
        m_MenuActionMap.value(tr("Remove from album"))->setVisible(false);
    }
#ifndef tablet_PC
    bool bflag_imageSupportSave = false;      //图片是否可以保存标志
    if (1 == paths.length()) { //单张照片
        if (LibUnionImage_NameSpace::isImageSupportRotate(paths[0]))
            bflag_imageSupportSave = true;
    }
    int flag_isRW = 0;
    if (1 == paths.length()) {
        if (QFileInfo(paths[0]).isReadable() && QFileInfo(paths[0]).isWritable()) {
            flag_isRW = 1;
        }
    }
    if (bflag_imageSupportSave && flag_isRW == 1) {
        m_MenuActionMap.value(tr("Rotate clockwise"))->setEnabled(true);
        m_MenuActionMap.value(tr("Rotate counterclockwise"))->setEnabled(true);
    } else {
        m_MenuActionMap.value(tr("Rotate clockwise"))->setVisible(false);
        m_MenuActionMap.value(tr("Rotate counterclockwise"))->setVisible(false);
    }
    if (1 == paths.length()) {
        DBImgInfo data = selectedIndexes().at(0).data(Qt::DisplayRole).value<DBImgInfo>();
        if (data.itemType == ItemTypePic) {
            m_MenuActionMap.value(tr("Photo info"))->setVisible(true);
            m_MenuActionMap.value(tr("Video info"))->setVisible(false);
            m_MenuActionMap.value(tr("Fullscreen"))->setVisible(true);
        } else {
            m_MenuActionMap.value(tr("Video info"))->setVisible(true);
            m_MenuActionMap.value(tr("Print"))->setVisible(false);
            m_MenuActionMap.value(tr("Slide show"))->setVisible(false);
            m_MenuActionMap.value(tr("Export"))->setVisible(false);
            m_MenuActionMap.value(tr("Photo info"))->setVisible(false);
            m_MenuActionMap.value(tr("Fullscreen"))->setVisible(false);
            m_MenuActionMap.value(tr("Set as wallpaper"))->setVisible(false);
        }
    } else {
        m_MenuActionMap.value(tr("Display in file manager"))->setVisible(false);
        m_MenuActionMap.value(tr("Photo info"))->setVisible(false);
        m_MenuActionMap.value(tr("Video info"))->setVisible(false);
        for (int i = 0; i < paths.length(); i++) {
            QString path = paths.at(i);
            if (Libutils::image::isVideo(path)) {
                m_MenuActionMap.value(tr("Print"))->setVisible(false);
                m_MenuActionMap.value(tr("Slide show"))->setVisible(false);
                m_MenuActionMap.value(tr("Export"))->setVisible(false);
                break;
            }
        }
        //有图片则可以显示幻灯片
        for (int i = 0; i < paths.length(); i++) {
            QString path = paths.at(i);
            if (!Libutils::image::isVideo(path)) {
                m_MenuActionMap.value(tr("Slide show"))->setVisible(true);
                break;
            }
        }
        //多选状态下应当支持批量旋转
        if (!isAllSelectedSupportRotate()) {
            m_MenuActionMap.value(tr("Rotate clockwise"))->setVisible(false);
            m_MenuActionMap.value(tr("Rotate counterclockwise"))->setVisible(false);
        } else {
            m_MenuActionMap.value(tr("Rotate clockwise"))->setVisible(true);
            m_MenuActionMap.value(tr("Rotate counterclockwise"))->setVisible(true);
        }
    }

    if ((1 == paths.length() || QFileInfo(paths[0]).suffix().contains("gif"))) {
        DBImgInfo data = selectedIndexes().at(0).data(Qt::DisplayRole).value<DBImgInfo>();
        if (data.itemType == ItemTypePic && QFileInfo(paths[0]).isReadable() && Libutils::base::isSupportWallpaper(paths[0])) {
            m_MenuActionMap.value(tr("Set as wallpaper"))->setVisible(true);
        } else {
            m_MenuActionMap.value(tr("Set as wallpaper"))->setVisible(false);
        }
    } else {
        m_MenuActionMap.value(tr("Set as wallpaper"))->setVisible(false);
    }
#endif
    qCDebug(albumLog) << "ThumbnailListView::onShowMenu - Exit";
}

void ThumbnailListView::appendAction(int id, const QString &text, const QString &shortcut)
{
    qCDebug(albumLog) << "ThumbnailListView::appendAction - Entry: id=" << id << "text=" << text << "shortcut=" << shortcut;
    QAction *ac = new QAction(this);
    addAction(ac);
    ac->setText(text);
    ac->setProperty("MenuID", id);
    //如果是查看照片，需要响应Enter键，而Enter键有两个Key-Enter和Return
    if (text.compare(tr("View")) == 0) {
        qCDebug(albumLog) << "text is View";
        QList<QKeySequence> shortcuts;
        shortcuts.append(QKeySequence(ENTER_SHORTCUT));
        shortcuts.append(QKeySequence(RETURN_SHORTCUT));
        ac->setShortcuts(shortcuts);
    } else {
        qCDebug(albumLog) << "text is not View";
        ac->setShortcut(QKeySequence(shortcut));
    }
    ac->setShortcutContext(Qt::WidgetShortcut);
    m_MenuActionMap.insert(text, ac);
    m_pMenu->addAction(ac);
    qCDebug(albumLog) << "ThumbnailListView::appendAction - Exit";
}

void ThumbnailListView::initMenuAction()
{
    qCDebug(albumLog) << "ThumbnailListView::initMenuAction - Entry";
    m_pMenu->clear();
    if (m_imageType.compare(COMMON_STR_TRASH) == 0) {
        appendAction(Types::IdMoveToTrash, tr("Delete"), ss(""));
        appendAction(Types::IdTrashRecovery, tr("Restore"), ss(BUTTON_RECOVERY));
        appendAction(Types::IdImageInfo, tr("Photo info"), ss(ImageInfo_CONTEXT_MENU));
        appendAction(Types::IdVideoInfo, tr("Video info"), ss(VideoInfo_CONTEXT_MENU));
        return;
    }

    m_MenuActionMap.clear();
#ifndef tablet_PC
    appendAction(Types::IdView, tr("View"), ss(VIEW_CONTEXT_MENU));
    appendAction(Types::IdFullScreen, tr("Fullscreen"), ss(FULLSCREEN_CONTEXT_MENU));
    appendAction(Types::IdPrint, tr("Print"), ss(PRINT_CONTEXT_MENU));
    appendAction(Types::IdStartSlideShow, tr("Slide show"), ss(SLIDESHOW_CONTEXT_MENU));
    m_pMenu->addSeparator();
    appendAction(Types::IdExport, tr("Export"), ss(EXPORT_CONTEXT_MENU));
    appendAction(Types::IdCopyToClipboard, tr("Copy"), ss(COPYTOCLIPBOARD_CONTEXT_MENU));
#endif
    appendAction(Types::IdMoveToTrash, tr("Delete"), ss(""));
    appendAction(Types::IdRemoveFromAlbum, tr("Remove from album"), ss(""));

    m_pMenu->addSeparator();
    appendAction(Types::IdAddToFavorites, tr("Favorite"), "");
    appendAction(Types::IdRemoveFromFavorites, tr("Unfavorite"), "");
#ifndef tablet_PC
    m_pMenu->addSeparator();
    appendAction(Types::IdRotateClockwise, tr("Rotate clockwise"), ss(ROTATECLOCKWISE_CONTEXT_MENU));
    appendAction(Types::IdRotateCounterclockwise, tr("Rotate counterclockwise"),
                 ss(ROTATECOUNTERCLOCKWISE_CONTEXT_MENU));
    m_pMenu->addSeparator();
    appendAction(Types::IdSetAsWallpaper, tr("Set as wallpaper"), ss(SETASWALLPAPER_CONTEXT_MENU));
    appendAction(Types::IdDisplayInFileManager, tr("Display in file manager"), ss(DISPLAYINFILEMANAGER_CONTEXT_MENU));
    appendAction(Types::IdImageInfo, tr("Photo info"), ss(ImageInfo_CONTEXT_MENU));
    appendAction(Types::IdVideoInfo, tr("Video info"), ss(VideoInfo_CONTEXT_MENU));
#endif
    qCDebug(albumLog) << "ThumbnailListView::initMenuAction - Exit";
}

DMenu *ThumbnailListView::createAlbumMenu()
{
    qCDebug(albumLog) << "ThumbnailListView::createAlbumMenu - Entry";
    DMenu *am = new DMenu(tr("Add to album"));
    am->setObjectName("AlbumMenu");
    am->setAccessibleName("AlbumMenu");
    auto albums = DBManager::instance()->getAllAlbumNames();
    QAction *ac1 = new QAction(am);
    ac1->setProperty("MenuID", Types::IdAddToAlbum);
    ac1->setText(tr("New album"));
    ac1->setData(-1); //设置UID为-1表示为新建相册选项
    am->addAction(ac1);
    am->addSeparator();
    QModelIndexList indexList = selectionModel()->selectedIndexes();
    QList<int> albumNames;
    //选中的全部路径
    QStringList paths = this->selectedPaths();

    //注意：album.first是UID，album.second是album name
    for (const auto &album : albums) {
        if (DBManager::instance()->isAllImgExistInAlbum(album.first, paths, AlbumDBType::Custom)) {
            albumNames.append(album.first);
        }
    }

    for (const auto &album : albums) {
        QAction *ac = new QAction(am);
        ac->setProperty("MenuID", Types::IdAddToAlbum);
        ac->setText(
            fontMetrics().elidedText(QString(album.second).replace("&", "&&"), Qt::ElideMiddle, 200));
        ac->setData(album.first);
        am->addAction(ac);
        if (albumNames.contains(album.first)) {
            ac->setEnabled(false);
        }
    }
    qCDebug(albumLog) << "ThumbnailListView::createAlbumMenu - Exit";
    return am;
}

void ThumbnailListView::onMenuItemClicked(QAction *action)
{
    qCDebug(albumLog) << "Menu item clicked:" << action->text();
    QStringList paths = selectedPaths();
    menuItemDeal(paths, action);
}

//获取选中路径
QStringList ThumbnailListView::selectedPaths()
{
    qCDebug(albumLog) << "Getting selected paths";
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
    qCDebug(albumLog) << "Found" << paths.size() << "selected paths";
    return paths;
}

QVariantList ThumbnailListView::selectedUrls()
{
    qCDebug(albumLog) << "ThumbnailListView::selectedUrls - Entry";
    QVariantList urls;
    for (QModelIndex index : selectionModel()->selectedIndexes()) {
        if (isRowHidden(index.row())) {
            continue;
        }
        DBImgInfo info = index.data(Qt::DisplayRole).value<DBImgInfo>();
        if ((info.itemType != ItemTypeBlank
             || info.itemType != ItemTypeTimeLineTitle
             || info.itemType != ItemTypeImportTimeLineTitle)
            && !info.filePath.isEmpty()) {
            urls << QUrl::fromLocalFile(info.filePath);
        }
    }
    qCDebug(albumLog) << "ThumbnailListView::selectedUrls - Exit";
    return urls;
}

QVariantList ThumbnailListView::allUrls()
{
    qCDebug(albumLog) << "ThumbnailListView::allUrls - Entry";
    QVariantList urls;
    for (int i = 0; i < m_model->rowCount(); i++) {
        QModelIndex index = m_model->index(i, 0);
        if (isRowHidden(index.row())) {
            continue;
        }
        DBImgInfo info = index.data(Qt::DisplayRole).value<DBImgInfo>();
        if ((info.itemType != ItemTypeBlank
             || info.itemType != ItemTypeTimeLineTitle
             || info.itemType != ItemTypeImportTimeLineTitle)
            && !info.filePath.isEmpty()) {
            urls << QUrl::fromLocalFile(info.filePath);
        }
    }
    qCDebug(albumLog) << "ThumbnailListView::allUrls - Exit";
    return urls;
}

void ThumbnailListView::menuItemDeal(QStringList paths, QAction *action)
{
    qCDebug(albumLog) << "Processing menu item action for" << paths.size() << "paths";
    paths.removeAll(QString(""));
    if (paths.isEmpty()) {
        qCDebug(albumLog) << "No valid paths to process";
        return;
    }

    const QString path = paths.first();
    const int id = action->property("MenuID").toInt();
    int UID = action->data().toInt();
    if (id == Types::IdAddToAlbum) {
        if (UID == -1) {
            qCDebug(albumLog) << "Creating new album";
            GlobalStatus::instance()->sigMenuItemClickedFromQWidget(Types::IdNewAlbum);
        } else {
            qCDebug(albumLog) << "Adding to album with UID:" << UID;
            GlobalStatus::instance()->sigMenuItemClickedFromQWidget(id, UID);
        }
    } else {
        qCDebug(albumLog) << "Emitting menu item clicked signal with id:" << id;
        GlobalStatus::instance()->sigMenuItemClickedFromQWidget(id);
    }
}

void ThumbnailListView::onThumbnailSizeLevelChanged()
{
    qCDebug(albumLog) << "Thumbnail size level changed";
    int value = GlobalStatus::instance()->thumbnailSizeLevel();
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
    qCDebug(albumLog) << "New base height:" << m_iBaseHeight;
    resizeEventF();
}

void ThumbnailListView::onCellBaseWidthChanged()
{
    qCDebug(albumLog) << "Cell base width changed";
    m_iBaseHeight = GlobalStatus::instance()->cellBaseWidth();
    resizeEventF();
}

void ThumbnailListView::onSelectionChanged()
{
    qCDebug(albumLog) << "Selection changed";
    updatetimeLimeBtnText();
    GlobalStatus::instance()->setSelectedPaths(selectedUrls());
    qCDebug(albumLog) << "ThumbnailListView::onSelectionChanged - Exit: selection update completed";
}

void ThumbnailListView::onCancelFavorite(const QModelIndex &index)
{
    qCDebug(albumLog) << "Canceling favorite for index:" << index.row();
    QStringList str;
    DBImgInfo info = index.data(Qt::DisplayRole).value<DBImgInfo>();
    str << info.filePath;
    //通知其它界面更新取消收藏
    DBManager::instance()->removeFromAlbum(DBManager::SpUID::u_Favorite, str, AlbumDBType::Favourite);
    qCDebug(albumLog) << "ThumbnailListView::onCancelFavorite - Exit: favorite removed";
}

void ThumbnailListView::resizeEvent(QResizeEvent *e)
{
    // qCDebug(albumLog) << "Resize event";
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
    this->verticalScrollBar()->setFixedHeight(this->height());
    // qCDebug(albumLog) << "ThumbnailListView::resizeEvent - Exit: resize completed";
}

bool ThumbnailListView::eventFilter(QObject *obj, QEvent *e)
{
    // qCDebug(albumLog) << "ThumbnailListView::eventFilter - Entry";
    if (obj == verticalScrollBar()) {
        if (e->type() == QEvent::MouseButtonPress) {
            //刷新鼠标在滚动条上按下标志位
            QRect scrollGlobaRect(verticalScrollBar()->mapToGlobal(QPoint(0, 0)),
                                  QSize(verticalScrollBar()->geometry().width(), verticalScrollBar()->geometry().height()));
            if (scrollGlobaRect.contains(QCursor::pos())) {
                qCDebug(albumLog) << "Scroll bar pressed - enabling animation";
                m_animationEnable = true;
            }
        } else if (e->type() == QEvent::MouseButtonRelease) {
            //刷新鼠标在滚动条上释放标志位
            m_animationEnable = false;
        }
    }

    if (e->type() == QEvent::Wheel && QApplication::keyboardModifiers() == Qt::ControlModifier) {
        // qCDebug(albumLog) << "ThumbnailListView::eventFilter - Exit: wheel event";
        return true;
    } else if (e->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
        if (keyEvent->key() == Qt::Key_PageDown) {
            qCDebug(albumLog) << "Page down key pressed";
            DScrollBar *vb = this->verticalScrollBar();
            int posValue = vb->value();
            posValue += this->height();
            vb->setValue(posValue);
            return true;
        } else if (keyEvent->key() == Qt::Key_PageUp) {
            qCDebug(albumLog) << "Page up key pressed";
            DScrollBar *vb = this->verticalScrollBar();
            int posValue = vb->value();
            posValue -= this->height();
            vb->setValue(posValue);
            return true;
        }
    }
    // qCDebug(albumLog) << "ThumbnailListView::eventFilter - Exit";
    return false;
}

QStringList ThumbnailListView::getCurrentIndexTime(const QModelIndex &index)
{
    // index对应的是图片，获取所属时间线的时间和数量
    qCDebug(albumLog) << "Getting current index time for row:" << index.row();
    QStringList list;
    if (index.isValid()) {
        for (int i = index.row(); i >= 0; i--) {
            QModelIndex idx = m_model->index(i, 0);
            DBImgInfo tempdata = idx.data(Qt::DisplayRole).value<DBImgInfo>();
            if (tempdata.itemType == ItemTypeBlank || tempdata.itemType == ItemTypeTimeLineTitle
                    || tempdata.itemType == ItemTypeImportTimeLineTitle) {
                list.append(tempdata.date);
                list.append(tempdata.num);
                qCDebug(albumLog) << "Found timeline data - date:" << tempdata.date << "num:" << tempdata.num;
                break;
            }
        }
    } else {
        qCDebug(albumLog) << "Invalid index provided";
    }
    return list;
}

bool ThumbnailListView::getCurrentIndexSelectStatus(const QModelIndex &index, bool isPic)
{
    qCDebug(albumLog) << "ThumbnailListView::getCurrentIndexSelectStatus - Entry";
    if (!index.isValid()) {
        qCDebug(albumLog) << "Invalid index provided";
        return false;
    }
    if (isPic) {
        //图片，向前循环判断选中状态
        for (int i = index.row(); i > 0; i--) {
            QModelIndex idx = m_model->index(i, 0);
            DBImgInfo tempdata = idx.data(Qt::DisplayRole).value<DBImgInfo>();
            if (tempdata.itemType == ItemTypeBlank || tempdata.itemType == ItemTypeTimeLineTitle
                    || tempdata.itemType == ItemTypeImportTimeLineTitle) {
                break;
            }
            if (!selectionModel()->isSelected(idx)) {
                qCDebug(albumLog) << "Not selected";
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
            if (!selectionModel()->isSelected(idx)) {
                qCDebug(albumLog) << "Not selected";
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
            if (!selectionModel()->isSelected(idx)) {
                qCDebug(albumLog) << "Not selected";
                return false;
            }
        }
    }
    qCDebug(albumLog) << "ThumbnailListView::getCurrentIndexSelectStatus - Exit";
    return true;
}

bool ThumbnailListView::isAllAppointType(ItemType type)
{
    qCDebug(albumLog) << "ThumbnailListView::isAllAppointType - Entry";
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
    qCDebug(albumLog) << __FUNCTION__ << "---isAllAppointType = " << isAllAppointType;
    return isAllAppointType;
}

void ThumbnailListView::hideAllAppointType(ItemType type)
{
    qCDebug(albumLog) << "ThumbnailListView::hideAllAppointType - Entry";
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
        //空白栏下第一个未隐藏的是标题，隐藏标题，同时将标题中的时间和数量传递给空白栏
        if (info.itemType == ItemTypeBlank) {
            bool nextVisibleIsTitle = false;
            for (int j = i + 1; j < m_model->rowCount(); j++) {
                QModelIndex nextIndex = m_model->index(j, 0);
                DBImgInfo nextInfo = nextIndex.data(Qt::DisplayRole).value<DBImgInfo>();
                if (!isRowHidden(j) && (nextInfo.itemType == ItemTypePic || nextInfo.itemType == ItemTypeVideo)) {
                    break;
                }
                if (!isRowHidden(j)) {
                    if (nextInfo.itemType == ItemTypeTimeLineTitle || nextInfo.itemType == ItemTypeImportTimeLineTitle) {
                        setRowHidden(j, true);
                        bool isSelect = getCurrentIndexSelectStatus(nextIndex, false);
                        emit sigTimeLineDataAndNum(nextInfo.date, nextInfo.num, isSelect ? QObject::tr("Unselect") : QObject::tr("Select"));
                        nextVisibleIsTitle = true;
                        break;
                    }
                }
            }
            if (nextVisibleIsTitle) {
                i--;
                continue;
            }
        } else if (info.itemType == ItemTypeTimeLineTitle
                   || info.itemType == ItemTypeImportTimeLineTitle) {
            //当前是标题，下一个未隐藏的也是标题时，隐藏当前标题
            for (int j = i + 1; j < m_model->rowCount(); j++) {
                QModelIndex nextIndex = m_model->index(j, 0);
                DBImgInfo nextInfo = nextIndex.data(Qt::DisplayRole).value<DBImgInfo>();
                if (!isRowHidden(j) && (nextInfo.itemType == ItemTypePic || nextInfo.itemType == ItemTypeVideo)) {
                    break;
                }
                if (!isRowHidden(j)) {
                    if (nextInfo.itemType == ItemTypeTimeLineTitle || nextInfo.itemType == ItemTypeImportTimeLineTitle || nextInfo.itemType == ItemTypeBlank) {
                        setRowHidden(i, true);
                        break;
                    }
                }
                //当前是最后一个标题
                if (isRowHidden(j) && (j == (m_model->rowCount() - 1))) {
                    setRowHidden(i, true);
                }
            }
        }
    }

    flushTopTimeLine(8); //筛完了要刷新顶部
    qCDebug(albumLog) << "ThumbnailListView::hideAllAppointType - Exit";
}

int ThumbnailListView::getRow(QPoint point)
{
    // qCDebug(albumLog) << "ThumbnailListView::getRow - Entry: point=" << point;
    return indexAt(point).row();
}

void ThumbnailListView::setListViewUseFor(ThumbnailListView::ListViewUseFor usefor)
{
    // qCDebug(albumLog) << "ThumbnailListView::setListViewUseFor - Entry";
    m_useFor = usefor;
}

//add start 3975
int ThumbnailListView::getListViewHeight()
{
    // qCDebug(albumLog) << "ThumbnailListView::getListViewHeight - Entry";
    return m_height;
}
//add end 3975
//有文件删除，刷新所有列表
void ThumbnailListView::updateThumbnailViewAfterDelete(const QStringList &paths)
{
    qCDebug(albumLog) << "ThumbnailListView::updateThumbnailViewAfterDelete - Entry";
    //列表上移除所有图片项
    if (m_delegatetype == ThumbnailDelegate::AllPicViewType
            || m_delegatetype == ThumbnailDelegate::AlbumViewCustomType
            || m_delegatetype == ThumbnailDelegate::AlbumViewFavoriteType
            || m_delegatetype == ThumbnailDelegate::TimeLineViewType
            || m_delegatetype == ThumbnailDelegate::AlbumViewImportTimeLineViewType) {
        qCDebug(albumLog) << "type is AllPicViewType, AlbumViewCustomType, AlbumViewFavoriteType, TimeLineViewType, AlbumViewImportTimeLineViewType";
        for (int i = (m_model->rowCount() - 1); i >= 0; i--) {
            QModelIndex index = m_model->index(i, 0);
            DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
            if (paths.contains(data.filePath)) { //你永远可以相信QStringList::contains
                m_model->removeRow(i);
                break;
            }
        }
    }
    if (m_delegatetype == ThumbnailDelegate::TimeLineViewType
            || m_delegatetype == ThumbnailDelegate::AlbumViewImportTimeLineViewType) {
        qCDebug(albumLog) << "type is TimeLineViewType, AlbumViewImportTimeLineViewType";
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
    qCDebug(albumLog) << "ThumbnailListView::updateThumbnailViewAfterDelete - Exit: update completed";
}

void ThumbnailListView::slotSelectCurrentDatePic(bool isSelect, QStandardItem *item)
{
    qCDebug(albumLog) << "ThumbnailListView::slotSelectCurrentDatePic - Entry";
    if (item == nullptr || !m_model->indexFromItem(item).isValid()) {
        return;
    }

    //disconnect(this->selectionModel(), &QItemSelectionModel::selectionChanged, m_batchOperateWidget, &BatchOperateWidget::sltSelectionChanged);

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

    //connect(this->selectionModel(), &QItemSelectionModel::selectionChanged, m_batchOperateWidget, &BatchOperateWidget::sltSelectionChanged);

    QItemSelection selected;
    QItemSelection deselected;
    emit this->selectionModel()->selectionChanged(selected, deselected);
    flushTopTimeLine(8);
    qCDebug(albumLog) << "ThumbnailListView::slotSelectCurrentDatePic - Exit";
}

void ThumbnailListView::onKeyPressFromQml(const QString &key)
{
    qCDebug(albumLog) << "Key pressed from QML:" << key;
    if (key == "Esc") {
        qCDebug(albumLog) << "Clearing selection";
        clearSelection();
    } else if (key == "Ctrl+A") {
        qCDebug(albumLog) << "Selecting all items";
        selectAll();
    } else if (key == "Page Up") {
        qCDebug(albumLog) << "Scrolling page up";
        DScrollBar *vb = this->verticalScrollBar();
        int posValue = vb->value();
        posValue -= this->height();
        vb->setValue(posValue);
    } else if (key == "Page Down") {
        qCDebug(albumLog) << "Scrolling page down";
        DScrollBar *vb = this->verticalScrollBar();
        int posValue = vb->value();
        posValue += this->height();
        vb->setValue(posValue);
    }
    qCDebug(albumLog) << "ThumbnailListView::onKeyPressFromQml - Exit";
}

//刷新所有标题中选择按钮的状态
void ThumbnailListView::slotChangeAllSelectBtnVisible(bool visible)
{
    qCDebug(albumLog) << "ThumbnailListView::slotChangeAllSelectBtnVisible - Entry";
//    if (m_delegatetype == ThumbnailDelegate::TimeLineViewType) {
//        TimeLineDateWidget *w = nullptr;
//        for (int i = 0; i < m_model->rowCount(); i++) {
//            QModelIndex idx = m_model->index(i, 0);
//            DBImgInfo data = idx.data(Qt::DisplayRole).value<DBImgInfo>();
//            if (data.itemType == ItemTypeTimeLineTitle) {
//                w = static_cast<TimeLineDateWidget *>(this->indexWidget(idx));
//                if (w != nullptr)  {
//                    w->onChangeChooseBtnVisible(visible);
//                }
//            }
//        }
//    } else if (m_delegatetype == ThumbnailDelegate::AlbumViewImportTimeLineViewType) {
//        importTimeLineDateWidget *w = nullptr;
//        for (int i = 0; i < m_model->rowCount(); i++) {
//            QModelIndex idx = m_model->index(i, 0);
//            DBImgInfo data = idx.data(Qt::DisplayRole).value<DBImgInfo>();
//            if (data.itemType == ItemTypeImportTimeLineTitle) {
//                w = static_cast<importTimeLineDateWidget *>(this->indexWidget(idx));
//                if (w != nullptr)  {
//                    w->onChangeChooseBtnVisible(visible);
//                }
//            }
//        }
//    }
}

// 选中重复导入的图片
void ThumbnailListView::selectUrls(const QStringList &urls)
{
    qCDebug(albumLog) << "Selecting URLs - count:" << urls.size();
    clearSelection();
    selectPhotos(AlbumControl::instance()->urls2localPaths(urls));
}

// 选中重复导入的图片
void ThumbnailListView::selectPhotos(const QStringList &paths)
{
    qCDebug(albumLog) << "Selecting photos - count:" << paths.size();
    QModelIndex firstIndex;

    if (!paths.isEmpty()) {
        this->clearSelection();
        QElapsedTimer timer_4s;
        timer_4s.start();
        for (int i = 0; i < m_model->rowCount(); i++) {
            QModelIndex index = m_model->index(i, 0);
            DBImgInfo info = index.data(Qt::DisplayRole).value<DBImgInfo>();

            if (paths.contains(info.filePath)) {
                // 选中
                selectionModel()->select(index, QItemSelectionModel::Select); //主要耗时点
                if (!firstIndex.isValid()) {
                    firstIndex = index;
                    qCDebug(albumLog) << "First selected index at row:" << i;
                }
            }

            if (timer_4s.elapsed() > 4000) { //只给4秒时间，能选多少选多少
                qCDebug(albumLog) << "Selection timeout after 4 seconds";
                break;
            }
        }
    }

    // 定位第一个重复导入的照片
    if (firstIndex.isValid()) {
        qCDebug(albumLog) << "Scrolling to first selected index";
        this->scrollTo(firstIndex, ScrollHint::PositionAtCenter);
    }
}

void ThumbnailListView::selectFirstPhoto()
{
    qCDebug(albumLog) << "Selecting first photo";
    if (m_model->rowCount() < 1) {
        qCDebug(albumLog) << "No photos available to select";
        return;
    }
    QModelIndex idx = m_model->index(0, 0);
    selectionModel()->setCurrentIndex(idx, QItemSelectionModel::Select);
    selectionModel()->select(idx, QItemSelectionModel::Select);
}

bool ThumbnailListView::isFirstPhotoSelected()
{
    qCDebug(albumLog) << "Checking if first photo is selected";
    QModelIndexList idx = selectionModel()->selectedIndexes();
    for (QModelIndex index : selectionModel()->selectedIndexes()) {
        if (index.row() == 0) {
            qCDebug(albumLog) << "First photo is selected";
            return true;
        }
    }
    qCDebug(albumLog) << "First photo is not selected";
    return false;
}

bool ThumbnailListView::isNoPhotosSelected()
{
    qCDebug(albumLog) << "Checking if no photos are selected";
    QModelIndexList indexList = selectionModel()->selectedIndexes();
    if (indexList.count() > 0)
        return false;
    else
        return true;
}

void ThumbnailListView::clearAll()
{
    qCDebug(albumLog) << "Clearing all items from model";
    m_model->clear();
}
//插入一个空白项，ItemTypeBlank
//插入时间线标题，ItemTypeTimeLineTitle
//插入已导入时间线标题，ItemTypeImportTimeLineTitle
void ThumbnailListView::insertBlankOrTitleItem(ItemType type, const QString &date, const QString &num, int height)
{
    qCDebug(albumLog) << "Inserting blank/title item - type:" << type << "date:" << date << "num:" << num << "height:" << height;
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
    qCDebug(albumLog) << "ThumbnailListView::insertBlankOrTitleItem - Exit: blank/title item inserted";
}
//更新空白栏高度
void ThumbnailListView::resetBlankItemHeight(int height)
{
    qCDebug(albumLog) << "Resetting blank item height to:" << height;
    for (int i = 0; i < m_model->rowCount(); i++) {
        QModelIndex idx = m_model->index(i, 0);
        DBImgInfo data = idx.data(Qt::DisplayRole).value<DBImgInfo>();
        if (data.itemType == ItemTypeBlank) {
            m_blankItemHeight = height;
            data.imgHeight = height;

            QVariant infoVariant;
            infoVariant.setValue(data);
            m_model->setData(idx, infoVariant, Qt::DisplayRole);
            qCDebug(albumLog) << "Updated blank item height at row:" << i;
            break;
        }
    }
    doItemsLayout();
    qCDebug(albumLog) << "ThumbnailListView::resetBlankItemHeight - Exit: blank item height reset completed";
}

void ThumbnailListView::insertThumbnailByImgInfos(DBImgInfoList infoList)
{
    qCDebug(albumLog) << "Inserting thumbnails - count:" << infoList.size();
    QDateTime currentTime = QDateTime::currentDateTime();
    for (int i = 0; i < infoList.size(); ++i) {
        DBImgInfo info = infoList.at(i);
        info.imgWidth = m_onePicWidth;
        info.imgHeight = m_onePicWidth;
        insertThumbnail(info);
    }
    //如果不是显示全部，则重新过滤显示
    if (m_currentShowItemType != ItemTypeNull) {
        qCDebug(albumLog) << "Filtering items by type:" << m_currentShowItemType;
        this->showAppointTypeItem(m_currentShowItemType);
    }

    //启动主动update机制
    qCDebug(albumLog) << "Starting import timer";
    m_importTimer->start(100);
    m_importActiveCount = 150;
    qCDebug(albumLog) << "ThumbnailListView::insertThumbnailByImgInfos - Exit: thumbnails inserted and timer started";
}
//判断所有图片是否全选中
bool ThumbnailListView::isAllSelected(ItemType type)
{
    bool isAllSelected = true;
    qCDebug(albumLog) << __FUNCTION__ << "---type = " << type;

    if (this->selectionModel()->selection().size() == 0) {
        qCDebug(albumLog) << "ThumbnailListView::isAllSelected - Exit: no selection, returning false";
        return false;
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
    qCDebug(albumLog) << "ThumbnailListView::isAllSelected - Exit: returning" << isAllSelected;
    return isAllSelected;
}

//判断选中图片是否都可旋转
bool ThumbnailListView::isAllSelectedSupportRotate()
{
    qCDebug(albumLog) << "ThumbnailListView::isAllSelectedSupportRotate - Entry: checking rotate support";
    QStringList list = selectedPaths();
    if (list.isEmpty()) {
        qCDebug(albumLog) << "ThumbnailListView::isAllSelectedSupportRotate - Exit: no selection, returning false";
        return false;
    }
    bool isAllSelectedSupportRotate = true;
    for (const auto &path : list) {
        if (!(LibUnionImage_NameSpace::isImageSupportRotate(path) && QFileInfo(path).isReadable() && QFileInfo(path).isWritable())) { //只要有一个不能转，就都不能转
            isAllSelectedSupportRotate = false;
            break;
        }
    }
    qCDebug(albumLog) << "ThumbnailListView::isAllSelectedSupportRotate - Exit: returning" << isAllSelectedSupportRotate;
    return isAllSelectedSupportRotate;
}

bool ThumbnailListView::isSelectedCanUseFavorite()
{
    qCDebug(albumLog) << "ThumbnailListView::isSelectedCanUseFavorite - Entry: checking favorite support";
    QStringList list = selectedPaths();
    if (list.isEmpty()) {
        qCDebug(albumLog) << "ThumbnailListView::isSelectedCanUseFavorite - Exit: no selection, returning false";
        return false;
    }
    bool canUseFavorite = true;
    for (const auto &path : list) {
        if (!QFile::exists(path)) { //当有文件不存在时，不可使用收藏
            canUseFavorite = false;
            break;
        }
    }
    qCDebug(albumLog) << "ThumbnailListView::isSelectedCanUseFavorite - Exit: returning" << canUseFavorite;
    return canUseFavorite;
}

bool ThumbnailListView::isSelectedCanUseDelete()
{
    qCDebug(albumLog) << "ThumbnailListView::isSelectedCanUseDelete - Entry: checking delete support";
    QStringList list = selectedPaths();
    if (list.isEmpty()) {
        qCDebug(albumLog) << "ThumbnailListView::isSelectedCanUseDelete - Exit: no selection, returning false";
        return false;
    }
    bool isSelectedCanUseDelete = false;
    for (const auto &path : list) {
        QFileInfo info(path);
        if (info.isSymLink()) {
            info = QFileInfo(info.readSymLink());
        }
        //路径无写权限
        if (!QFileInfo(info.dir(), info.dir().path()).isWritable()) {
            continue;
        } else {  //只要有一个能写或不存在，就可以删
            isSelectedCanUseDelete = true;
            break;
        }
    }

    qCDebug(albumLog) << "ThumbnailListView::isSelectedCanUseDelete - Exit: returning" << isSelectedCanUseDelete;
    return isSelectedCanUseDelete;
}

//删除到相册已删除
void ThumbnailListView::removeSelectToTrash(QStringList paths)
{
    qCDebug(albumLog) << "ThumbnailListView::removeSelectToTrash - Entry: paths count=" << paths.size();
    DBImgInfoList infos;
    DBImgInfo info;
    int count = 0;
    for (int i = 0; i < paths.size(); i++) {
        QFileInfo infox(paths.at(i));
        if (infox.isSymLink()) {
            infox = QFileInfo(infox.readSymLink());
        }
        //文件或者文件夹不可写
        if (!QFileInfo(infox.dir(), infox.dir().path()).isWritable()) {
            continue;
        } else {
            count++;
        }
    }
//    ImgDeleteDialog *dialog = new ImgDeleteDialog(this, count, COMMON_STR_TRASH == m_imageType);
//    dialog->setObjectName("deteledialog");
//    if (dialog->exec() > 0) {
//        clearSelection();

//        //清理界面
//        if (COMMON_STR_VIEW_TIMELINE == m_imageType || COMMON_STR_RECENT_IMPORTED == m_imageType) {
//            emit sigMoveToTrash();
//        }

//        //执行删除
//        (COMMON_STR_TRASH == m_imageType) ? ImageEngineApi::instance()->moveImagesToTrash(paths, true, false)
//        : ImageEngineApi::instance()->moveImagesToTrash(paths);
//    }
}
//更新时间线界面内各个按钮的text状态，单选/框选
void ThumbnailListView::updatetimeLimeBtnText()
{
    qCDebug(albumLog) << "ThumbnailListView::updatetimeLimeBtnText - Entry";
   QModelIndexList indexs = selectionModel()->selectedIndexes();
   emit sigShowCheckBox(!indexs.isEmpty());
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
               if (w != nullptr) {
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
               if (iw != nullptr) { //说明是第一个时间线
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
       }
   }
    //刷新顶部栏
    flushTopTimeLine(8);
    qCDebug(albumLog) << "ThumbnailListView::updatetimeLimeBtnText - Exit";
}

void ThumbnailListView::showAppointTypeItem(ItemType type)
{
    qCDebug(albumLog) << "ThumbnailListView::showAppointTypeItem - Entry: type=" << type;
    //记录当前显示类型
    m_currentShowItemType = type;
    //切换显示之前先清除选中
    this->clearSelection();
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
    qCDebug(albumLog) << "ThumbnailListView::showAppointTypeItem - Exit: type filtering completed";
}

//显示类型数量
int ThumbnailListView::getAppointTypeItemCount(ItemType type)
{
    qCDebug(albumLog) << "ThumbnailListView::getAppointTypeItemCount - Entry: type=" << type;
    int count = 0;
    for (int i = 0;  i < m_model->rowCount(); i++) {
        if (isRowHidden(i)) {
            continue;
        }
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
    qCDebug(albumLog) << "ThumbnailListView::getAppointTypeItemCount - Exit: returning count=" << count;
    return count;
}
//显示指定类型选中项数量
int ThumbnailListView::getAppointTypeSelectItemCount(ItemType type)
{
    qCDebug(albumLog) << "ThumbnailListView::getAppointTypeSelectItemCount - Entry: type=" << type;
    int count = 0;
    QModelIndexList list = selectedIndexes();
    int size = list.size();
    for (int i = 0;  i < size; i++) {
        QModelIndex index = list.at(i);
        if (isRowHidden(index.row())) {
            continue;
        }
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
    qCDebug(albumLog) << "ThumbnailListView::getAppointTypeSelectItemCount - Exit: returning count=" << count;
    return count;
}
//按类型选择
void ThumbnailListView::selectAllByItemType(ItemType type)
{
    qCDebug(albumLog) << __FUNCTION__ << "---type = " << type;
    this->selectAll();
    emit sigSelectAll();
    updateMenuContents();
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
    qCDebug(albumLog) << "ThumbnailListView::TimeLineSelectAllBtn - Entry";
//    if (m_delegatetype == ThumbnailDelegate::TimeLineViewType || m_delegatetype == ThumbnailDelegate:: AlbumViewImportTimeLineViewType) {
//        for (int i = 0; i < m_model->rowCount(); i++) {
//            QModelIndex idx = m_model->index(i, 0);
//            DBImgInfo data = idx.data(Qt::DisplayRole).value<DBImgInfo>();
//            if (data.itemType == ItemTypeTimeLineTitle) {
//                TimeLineDateWidget *w = static_cast<TimeLineDateWidget *>(this->indexWidget(idx));
//                if (w) {
//                    w->onTimeLinePicSelectAll(true);
//                }
//            } else if (data.itemType == ItemTypeImportTimeLineTitle) {
//                importTimeLineDateWidget *w = static_cast<importTimeLineDateWidget *>(this->indexWidget(idx));
//                if (w) {
//                    w->onTimeLinePicSelectAll(true);
//                }
//            }
//        }
//    }
}

void ThumbnailListView::setBatchOperateWidget(BatchOperateWidget *widget)
{
    qCDebug(albumLog) << "ThumbnailListView::setBatchOperateWidget - Entry";
    m_batchOperateWidget = widget;
}

void ThumbnailListView::navigateToMonth(const QString &month)
{
    qCDebug(albumLog) << "Navigating to month:" << month;
    QString tmpdate = month;
    for (int i = 0; i < m_model->rowCount(); i++) {
        QModelIndex index = m_model->index(i, 0);
        DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
        //循环找到时间
        if (data.itemType == ItemTypePic || data.itemType == ItemTypeVideo || data.itemType == ItemTypeMountImg) {
            continue;
        }
        //时间线
        if (m_delegatetype == ThumbnailDelegate::TimeLineViewType && (data.itemType == ItemTypeTimeLineTitle || data.itemType == ItemTypeBlank)
            && data.date.contains(tmpdate)) {
            qCDebug(albumLog) << "Found timeline view match at row:" << i;
            scrollTo(index, QAbstractItemView::PositionAtTop);
            break;
        } else if (m_delegatetype == ThumbnailDelegate::AlbumViewImportTimeLineViewType && (data.itemType == ItemTypeImportTimeLineTitle || data.itemType == ItemTypeBlank)
                   && data.date == tmpdate) { //已导入时间线
            qCDebug(albumLog) << "Found import timeline view match at row:" << i;
            scrollTo(index, QAbstractItemView::PositionAtTop);
        }
    }
    qCDebug(albumLog) << "ThumbnailListView::navigateToMonth - Exit";
}

void ThumbnailListView::slotLoadFirstPageThumbnailsFinish()
{
    // 将缩略图信息插入到listview中
    //通知所有照片界面刷新，如果没有图片则显示导入界面
    qCDebug(albumLog) << "First page thumbnails loaded";
    emit sigDBImageLoaded();
}

void ThumbnailListView::slotOneImgReady(const QString &path, QPixmap pix)
{
    qCDebug(albumLog) << "Image ready for path:" << path;
    for (int i = 0; i < m_model->rowCount(); i++) {
        QModelIndex index = m_model->index(i, 0);
        DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
        if (data.filePath == path) {
            data.image = pix;
            cutPixmap(data);
            QVariant meta;
            meta.setValue(data);
            ImageDataService::instance()->addImage(data.filePath, data.image.toImage());
            m_model->setData(index, meta, Qt::DisplayRole);
            qCDebug(albumLog) << "Updated image at row:" << i;
            break;
        }
    }
}

void ThumbnailListView::onScrollTimerOut()
{
    qCDebug(albumLog) << "Scroll timer timeout";
    disconnect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, &ThumbnailListView::onScrollbarValueChanged);
    m_animation->start();
    connect(m_animation, &QPropertyAnimation::finished, this, [ = ]() {
        connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, &ThumbnailListView::onScrollbarValueChanged);
    });
}

void ThumbnailListView::onScrollbarValueChanged(int value)
{
    qCDebug(albumLog) << "ThumbnailListView::onScrollbarValueChanged - Entry";
    Q_UNUSED(value)
    //滚动条向下滑动
    qCDebug(albumLog) << "Scrollbar value changed to:" << value;
    if (m_animation && m_animation->state() == QPropertyAnimation::Running) {
        m_animation->stop();
    }
    if (m_scrollTimer->isActive()) {
        m_scrollTimer->stop();
    }

    flushTopTimeLine(8); //滑完了要刷新顶部
    qCDebug(albumLog) << "ThumbnailListView::onScrollbarValueChanged - Exit";
}

void ThumbnailListView::flushTopTimeLine(int offset)
{
    qCDebug(albumLog) << "Flushing top timeline with offset:" << offset;
    QModelIndex index = this->indexAt(QPoint(80, m_blankItemHeight + offset)); //额外下移x个像素点，避免在筛选状态的时候选择错误
    DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();

    //无选中照片，直接返回
    QModelIndexList list = selectionModel()->selectedIndexes();
    if (list.isEmpty()) {
        qCDebug(albumLog) << "No items selected, emitting default timeline data";
        emit sigTimeLineDataAndNum(data.date, data.num, QObject::tr("Select"));
        return;
    }

    //先判断当前的类型，是标题则获取是否全选；是图片，则找到它所在时间线的数量和时间以及是否全选
    if (data.itemType == ItemTypeBlank || data.itemType == ItemTypeTimeLineTitle
            || data.itemType == ItemTypeImportTimeLineTitle) {
        bool isSelect = getCurrentIndexSelectStatus(index, false);
        qCDebug(albumLog) << "Emitting timeline data for title item - date:" << data.date << "num:" << data.num << "isSelect:" << isSelect;
        emit sigTimeLineDataAndNum(data.date, data.num, isSelect ? QObject::tr("Unselect") : QObject::tr("Select"));
    } else {
        bool isSelect = getCurrentIndexSelectStatus(index, true);
        QStringList currentIndexTimeList = getCurrentIndexTime(index);
        if (currentIndexTimeList.size() == 2) {
            qCDebug(albumLog) << "Emitting timeline data for image item - date:" << currentIndexTimeList.at(0) << "num:" << currentIndexTimeList.at(1) << "isSelect:" << isSelect;
            emit sigTimeLineDataAndNum(currentIndexTimeList.at(0), currentIndexTimeList.at(1), isSelect ? QObject::tr("Unselect") : QObject::tr("Select"));
        }
    }
    qCDebug(albumLog) << "ThumbnailListView::flushTopTimeLine - Exit";
}

void ThumbnailListView::onDoubleClicked(const QModelIndex &index)
{
    qCDebug(albumLog) << "Double clicked on row:" << index.row();
    if (ALBUM_PATHTYPE_BY_PHONE != m_imageType && m_imageType.compare(COMMON_STR_TRASH) != 0) {
        DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
        if (data.itemType == ItemTypeBlank
            || data.itemType == ItemTypeTimeLineTitle
            || data.itemType == ItemTypeImportTimeLineTitle) {
            qCDebug(albumLog) << "Ignoring double click on blank/title item";
            return;
        }

        qCDebug(albumLog) << "Emitting double click signal for path:" << data.filePath;
        GlobalStatus::instance()->sigDoubleClickedFromQWidget(QUrl::fromLocalFile(data.filePath).toString());
    }
}

void ThumbnailListView::onClicked(const QModelIndex &index)
{
    if (m_isSelectAllBtn) {
        qCDebug(albumLog) << "Ignoring click while select all button is active";
        return;
    }
    DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
    qCDebug(albumLog) << __FUNCTION__ << "---" << data.filePath;
    qCDebug(albumLog) << __FUNCTION__ << "---" << ImageDataService::instance()->imageIsLoaded(data.filePath, m_imageType == COMMON_STR_TRASH);
    qCDebug(albumLog) << __FUNCTION__ << "---" << ImageDataService::instance()->getMovieDurationStrByPath(data.filePath);
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
    qCDebug(albumLog) << "Cutting pixmap for path:" << DBImgInfo.filePath;
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
            qCDebug(albumLog) << "Cropped image to square from width:" << width << "to height:" << height;
        } else {
            y = y - width / 2;
            x = 0;
            DBImgInfo.image = DBImgInfo.image.copy(x, y, width, width);
            qCDebug(albumLog) << "Cropped image to square from height:" << height << "to width:" << width;
        }
    }
}

void ThumbnailListView::timeLimeFloatBtnClicked(const QString &date, bool isSelect)
{
    qCDebug(albumLog) << "ThumbnailListView::timeLimeFloatBtnClicked - Entry";
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
               if (pdata.itemType  == ItemTypePic || pdata.itemType == ItemTypeVideo) {
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
               } else if (pdata.itemType  == ItemTypePic || pdata.itemType == ItemTypeVideo) {
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
   qCDebug(albumLog) << "ThumbnailListView::timeLimeFloatBtnClicked - Exit";
}

void ThumbnailListView::resizeEventF()
{
    qCDebug(albumLog) << "Resizing view - width:" << width();
    int i_totalwidth = width() - RIGHT_MARGIN;
    //计算一行的个数
    m_rowSizeHint = i_totalwidth / m_iBaseHeight;
    int currentwidth = (i_totalwidth - ITEM_SPACING * (m_rowSizeHint - 1)) / m_rowSizeHint;//一张图的宽度
    m_onePicWidth = currentwidth;

    if (m_delegate) {
        m_delegate->setItemSize(QSize(m_onePicWidth, m_onePicWidth));
    }

    this->setSpacing(ITEM_SPACING);
    qCDebug(albumLog) << "New dimensions - rowSizeHint:" << m_rowSizeHint << "onePicWidth:" << m_onePicWidth;
}

