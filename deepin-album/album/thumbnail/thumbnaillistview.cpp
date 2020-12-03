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

#include "controller/signalmanager.h"
#include "controller/wallpapersetter.h"
#include "widgets/dialogs/imgdeletedialog.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "utils/unionimage.h"
#include "imageengine/imageengineapi.h"
#include "imageengine/imageenginethread.h"

namespace {
const int ITEM_SPACING = 4;
const int BASE_HEIGHT = 100;

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

ThumbnailListView::ThumbnailListView(ThumbnailDelegate::DelegateType type, QString imgtype, QWidget *parent)
    :  DListView(parent), m_delegatetype(type), m_allfileslist(), updateEnableSelectionByMouseTimer(nullptr)
{
    if (ThumbnailDelegate::AllPicViewType == m_delegatetype) {
        m_scrollbartopdistance = 50;
        m_scrollbarbottomdistance = 27;
    } else if (ThumbnailDelegate::AlbumViewType == m_delegatetype) {
        m_scrollbarbottomdistance = 27;
    } else if (ThumbnailDelegate::SearchViewType == m_delegatetype || ThumbnailDelegate::AlbumViewPhoneType == m_delegatetype) {
        m_scrollbartopdistance = 134;
        m_scrollbarbottomdistance = 27;
    }
    m_model = new QStandardItemModel(this);
    m_imageType = imgtype;
    m_iDefaultWidth = 0;
    m_iBaseHeight = BASE_HEIGHT;
    m_albumMenu = nullptr;
    setResizeMode(QListView::Adjust);
    setViewMode(QListView::IconMode);
    setSpacing(ITEM_SPACING);
    setDragEnabled(false);
    if (COMMON_STR_VIEW_TIMELINE == m_imageType ||
            COMMON_STR_RECENT_IMPORTED == m_imageType) {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
    //按照像素滚动，步进20
    setVerticalScrollMode(QListView::ScrollPerPixel);
    verticalScrollBar()->setSingleStep(20);

    setContextMenuPolicy(Qt::CustomContextMenu);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setMinimumWidth(500);
    m_delegate = new ThumbnailDelegate(type);
    m_delegate->m_imageTypeStr = m_imageType;
    setItemDelegate(m_delegate);
    setModel(m_model);
    m_pMenu = new DMenu();
    initMenuAction();
    initConnections();
    installEventFilter(this);
    m_dt = new QTimer(this);
    m_dt->setSingleShot(true);
    m_dt->setInterval(20);
    connect(m_dt, SIGNAL(timeout()), this, SLOT(onTimerOut()));
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
            this, &ThumbnailListView::sltChangeDamagedPixOnThemeChanged);
    touchTapDistance = 15;
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
    // 当事件source为MouseEventSynthesizedByQt，认为此事件为TouchBegin转换而来
    if (event->source() == Qt::MouseEventSynthesizedByQt) {
        lastTouchBeginPos = event->pos();

        // 清空触屏滚动操作，因为在鼠标按下时还不知道即将进行的是触屏滚动还是文件框选
        if (QScroller::hasScroller(this)) {
            // 不可使用 ungrab，会导致应用崩溃，或许是Qt的bug
            QScroller::scroller(this)->deleteLater();
        }

        if (updateEnableSelectionByMouseTimer && updateEnableSelectionByMouseTimer->isActive()) {
            updateEnableSelectionByMouseTimer->stop();
        }
        updateEnableSelectionByMouseTimer = new QTimer(this);
        updateEnableSelectionByMouseTimer->setSingleShot(true);
        updateEnableSelectionByMouseTimer->setInterval(50);
        connect(updateEnableSelectionByMouseTimer, &QTimer::timeout, updateEnableSelectionByMouseTimer, &QTimer::deleteLater);
        updateEnableSelectionByMouseTimer->start();
    }

    if ((QApplication::keyboardModifiers() == Qt::ShiftModifier && event->button() == Qt::LeftButton)
            && (m_imageType == COMMON_STR_VIEW_TIMELINE || m_imageType == COMMON_STR_RECENT_IMPORTED));
    else
        DListView::mousePressEvent(event);
    if ((m_imageType != COMMON_STR_VIEW_TIMELINE) && (m_imageType != "All Photos") &&
            (m_imageType != COMMON_STR_TRASH) && (m_imageType != ALBUM_PATHTYPE_BY_PHONE)) {
        if (dragDropMode() != NoDragDrop) {
            setDragDropMode(DragDrop);
        }
    } else {
        setDragEnabled(false);
    }

    bool isListArea = this->indexAt(event->pos()).isValid();
    if (!isListArea) {
        if (QApplication::keyboardModifiers() != Qt::ControlModifier) {
            clearSelection();
            update();
        }
        if (event->source() != Qt::MouseEventSynthesizedByQt) {
            clearSelection();
            update();
        }
    }
    if (m_imageType == COMMON_STR_VIEW_TIMELINE || m_imageType == COMMON_STR_RECENT_IMPORTED) {
        if (QApplication::keyboardModifiers() == Qt::NoModifier /*&& event->button() == Qt::LeftButton*/) {
            emit sigMousePress(event);
        } else if (QApplication::keyboardModifiers() == Qt::ShiftModifier && event->button() == Qt::LeftButton)
            emit sigShiftMousePress(event);
        else if (QApplication::keyboardModifiers() == Qt::ControlModifier && event->button() == Qt::LeftButton) {
            emit sigCtrlMousePress(event);
        }
    }
}

void ThumbnailListView::mouseMoveEvent(QMouseEvent *event)
{
    QRectF rect(QPointF((lastTouchBeginPos.x() - 30), (lastTouchBeginPos.y() - 30)),
                QPointF((lastTouchBeginPos.x() + 30), (lastTouchBeginPos.y() + 30)));
    if (rect.contains(event->pos())) {
        return;
    }
    emit sigMouseMove();
    if (event->source() == Qt::MouseEventSynthesizedByQt) {
        if (QScroller::hasScroller(this))
            return;
        // 在定时器期间收到鼠标move事件且距离大于一定值则认为触发视图滚动
        if (updateEnableSelectionByMouseTimer && updateEnableSelectionByMouseTimer->isActive()) {
            const QPoint difference_pos = event->pos() - lastTouchBeginPos;
            if (qAbs(difference_pos.x()) > touchTapDistance || qAbs(difference_pos.y()) > touchTapDistance) {
                QScroller::grabGesture(this);
                QScroller *scroller = QScroller::scroller(this);
                scroller->handleInput(QScroller::InputPress, event->localPos(), static_cast<qint64>(event->timestamp()));
                scroller->handleInput(QScroller::InputMove, event->localPos(), static_cast<qint64>(event->timestamp()));
            }
            return;
        }
    }
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
    QTimer::singleShot(100, this, SLOT(resizeEventF()));
}

void ThumbnailListView::mouseReleaseEvent(QMouseEvent *event)
{
    DListView::mouseReleaseEvent(event);
    if (COMMON_STR_RECENT_IMPORTED  == m_imageType) {
        if (QApplication::keyboardModifiers() == Qt::NoModifier) {
            emit sigMouseRelease();
        }
    } else {
        emit sigMouseRelease();
    }
    // 避免滚动视图导致文件选中状态被取消
    if (!QScroller::hasScroller(this))
        return DListView::mouseReleaseEvent(event);
}

void ThumbnailListView::keyPressEvent(QKeyEvent *event)
{
    DListView::keyPressEvent(event);
    if ((event->modifiers() == Qt::ControlModifier) && (event->key() == Qt::Key_A)) {
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
    connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, [ = ](int value) {
        if (value && value >= (this->verticalScrollBar()->maximum())) {
            if (m_requestCount > 0) {
                bneedloadimage = true;
            } else {
                requestSomeImages();
            }
        }
    });
    connect(this->verticalScrollBar(), &QScrollBar::rangeChanged, this, [ = ](int min, int max) {
        Q_UNUSED(max);
        Q_UNUSED(min);
        if (nullptr == m_item) {
            QScrollBar *bar = this->verticalScrollBar();
            bar->setGeometry(bar->x(), /*bar->y() + */m_scrollbartopdistance, bar->width(), this->height() - m_scrollbartopdistance - m_scrollbarbottomdistance);
        }
    });
    connect(this, &QListView::customContextMenuRequested, this, &ThumbnailListView::onShowMenu);
    connect(m_pMenu, &DMenu::triggered, this, &ThumbnailListView::onMenuItemClicked);
    connect(this, &ThumbnailListView::doubleClicked, this, [ = ](const QModelIndex & index) {
        if (ALBUM_PATHTYPE_BY_PHONE != m_imageType) {
            if (m_imageType.compare(COMMON_STR_TRASH) != 0) {
                emit openImage(index.row());
            }
        }
    });
    connect(this, &ThumbnailListView::clicked, this, [ = ]() {
        emit hideExtensionPanel();
    });
    connect(dApp->signalM, &SignalManager::sigMainwindowSliderValueChg, this, &ThumbnailListView::onPixMapScale);
    connect(m_delegate, &ThumbnailDelegate::sigCancelFavorite, this,
            &ThumbnailListView::onCancelFavorite);
}

void ThumbnailListView::calBasePixMap(ItemInfo &info)
{
    int i_totalwidth = window()->width() - 30;
    bool bcalBase = false;
    if (info.width != 0) {
        bcalBase = (info.height / info.width) > 3;
    }
    if (0 == info.height || 0 == info.width || bcalBase) {
        info.width = m_iBaseHeight;
        info.height = m_iBaseHeight;
    } else {
        info.width = info.width * m_iBaseHeight / info.height;
        if (info.width > i_totalwidth) {
            info.height = m_iBaseHeight / 4;
            info.width = i_totalwidth / 4;
        } else {
            info.height = m_iBaseHeight;
        }
    }
    info.imgHeight = info.height;
    info.imgWidth = info.width;
    info.imgHeight = (1 > info.imgHeight) ? 1 : info.imgHeight;
    info.imgWidth = (1 > info.imgWidth) ? 1 : info.imgWidth;
    info.height =
        (m_iBaseHeight > info.height) ? m_iBaseHeight : info.height;
    info.width = (28 > info.width) ? 28 : info.width;
}

void ThumbnailListView::calBasePixMapWandH()
{
    int i_totalwidth = window()->width() - 30;

    for (int i = 0; i < m_ItemList.length(); i++) {
        if (0 == m_ItemList[i].height || 0 == m_ItemList[i].width) {
            m_ItemList[i].width = m_iBaseHeight;
            m_ItemList[i].height = m_iBaseHeight;
        } else {
            m_ItemList[i].width = m_ItemList[i].width * m_iBaseHeight / m_ItemList[i].height;
            if (m_ItemList[i].width > i_totalwidth) {
                m_ItemList[i].height = m_iBaseHeight / 4;
                m_ItemList[i].width = i_totalwidth / 4;
            } else {
                m_ItemList[i].height = m_iBaseHeight;
            }
        }

        m_ItemList[i].imgHeight = m_ItemList[i].height;
        m_ItemList[i].imgWidth = m_ItemList[i].width;
        // Prevents height or width less than 2 after scaling
        m_ItemList[i].imgHeight = (1 > m_ItemList[i].imgHeight) ? 1 : m_ItemList[i].imgHeight;
        m_ItemList[i].imgWidth = (1 > m_ItemList[i].imgWidth) ? 1 : m_ItemList[i].imgWidth;
        m_ItemList[i].height =
            (m_iBaseHeight > m_ItemList[i].height) ? m_iBaseHeight : m_ItemList[i].height;
        m_ItemList[i].width = (28 > m_ItemList[i].width) ? 28 : m_ItemList[i].width;
    }
}

void ThumbnailListView::calWidgetItem()
{
    int m_gbaseWidth = 0;
    int i_totalwidth = width() - 30;  // same as i_totalwidth in calBasePixMapWandH()
    QList<int> rowWidthList;
    QList<ItemInfo> itemInfoList;
    rowWidthList.clear();
    itemInfoList.clear();
    QList<QList<ItemInfo>> gridItem;
    gridItem.clear();
    int totlelength = m_ItemListLeft.size();
    int corrent = 0;
    // set rows for list
    for (int i = 0; i < totlelength; i++) {
        if ((m_gbaseWidth + m_ItemListLeft[corrent].width) <= i_totalwidth) {
            m_gbaseWidth = m_gbaseWidth + m_ItemListLeft[corrent].width + ITEM_SPACING;
            itemInfoList << m_ItemListLeft[corrent];
            if (m_allNeedRequestFilesCount < 1 && corrent == m_ItemListLeft.size() - 1) {
                m_gbaseWidth -= ITEM_SPACING;
                rowWidthList << m_gbaseWidth;
                gridItem << itemInfoList;
                for (int j = 0; j <= corrent; j++) {
                    m_ItemList << m_ItemListLeft.first();
                    m_ItemListLeft.removeFirst();
                }
            }
        } else if (i_totalwidth - m_gbaseWidth > 200) {
            m_ItemListLeft[corrent].imgHeight =
                m_ItemListLeft[corrent].imgHeight * (i_totalwidth - m_gbaseWidth) / m_ItemListLeft[corrent].imgWidth;
            m_ItemListLeft[corrent].imgHeight = (1 > m_ItemListLeft[corrent].imgHeight) ? 1 : m_ItemListLeft[corrent].imgHeight;
            m_ItemListLeft[corrent].imgWidth = i_totalwidth - m_gbaseWidth;
            m_ItemListLeft[corrent].width = i_totalwidth - m_gbaseWidth;
            m_gbaseWidth = i_totalwidth;
            rowWidthList << m_gbaseWidth;
            itemInfoList << m_ItemListLeft[corrent];
            gridItem << itemInfoList;
            for (int j = 0; j <= corrent; j++) {
                m_ItemList << m_ItemListLeft.first();
                m_ItemListLeft.removeFirst();
            }
            corrent = -1;
            m_gbaseWidth = 0;
            itemInfoList.clear();
        } else {
            m_gbaseWidth -= ITEM_SPACING;
            rowWidthList << m_gbaseWidth;
            m_gbaseWidth = m_ItemListLeft[corrent].width + ITEM_SPACING;
            gridItem << itemInfoList;
            for (int j = 0; j < corrent; j++) {
                m_ItemList << m_ItemListLeft.first();
                m_ItemListLeft.removeFirst();
            }
            itemInfoList.clear();
            corrent = 0;
            itemInfoList << m_ItemListLeft[corrent];
            if (m_allNeedRequestFilesCount < 1 && corrent == m_ItemListLeft.size() - 1) {
                m_gbaseWidth -= ITEM_SPACING;
                rowWidthList << m_gbaseWidth;
                gridItem << itemInfoList;
                for (int j = 0; j <= corrent; j++) {
                    m_ItemList << m_ItemListLeft.first();
                    m_ItemListLeft.removeFirst();
                }
            }
        }
        corrent++;
    }

    // scaling for each row adapting list width except last one
    totlelength = rowWidthList.size();
    if (m_allNeedRequestFilesCount < 1)
        totlelength -= 1;
    for (int i = 0; i < totlelength; i++) {
        if (rowWidthList[i] < i_totalwidth) {
            int i_totalwidthExSpace = i_totalwidth - ITEM_SPACING * gridItem[i].length();
            int rowWidthListExSpace = rowWidthList[i] - ITEM_SPACING * gridItem[i].length();
            int rowWidth = 0;
            for (int j = 0; j < gridItem[i].length(); j++) {
                gridItem[i][j].width = gridItem[i][j].width * i_totalwidthExSpace / rowWidthListExSpace;
                gridItem[i][j].height = gridItem[i][j].height * i_totalwidthExSpace / rowWidthListExSpace;
                gridItem[i][j].imgWidth = gridItem[i][j].imgWidth * i_totalwidthExSpace / rowWidthListExSpace;
                gridItem[i][j].imgHeight = gridItem[i][j].imgHeight * i_totalwidthExSpace / rowWidthListExSpace;
                rowWidth = rowWidth + gridItem[i][j].width + ITEM_SPACING;
            }
            rowWidthList[i] = rowWidth - ITEM_SPACING;
            if (rowWidthList[i] < i_totalwidth) {
                gridItem[i][0].width = gridItem[i][0].width + i_totalwidth - rowWidthList[i];
            }
        }
    }
    addThumbnailViewNew(gridItem);
    if (gridItem.size() > 0) {
        bfirstload = false;
    }
}

void ThumbnailListView::calWidgetItemWandH()
{
    int i_baseWidth = 0;
    int i_totalwidth = width() - 30;  // same as i_totalwidth in calBasePixMapWandH()
    QList<int> rowWidthList;          //一行的宽度
    QList<ItemInfo> itemInfoList;     //一行的item项
    QList<ItemInfo> m_ItemListAll;    //所有图片项
    m_ItemListAll << m_ItemList << m_ItemListLeft;
    rowWidthList.clear();
    itemInfoList.clear();
    m_gridItem.clear();
    m_ItemList.clear();
    m_ItemListLeft.clear();
    int totlelength = m_ItemListAll.size();
    int corrent = 0;
    // set rows for list
    for (int i = 0; i < totlelength; i++) {
        if ((i_baseWidth + m_ItemListAll[corrent].width) <= i_totalwidth) {
            i_baseWidth = i_baseWidth + m_ItemListAll[corrent].width + ITEM_SPACING;
            itemInfoList << m_ItemListAll[corrent];
            if (m_allNeedRequestFilesCount < 1 && corrent == m_ItemListAll.size() - 1) {
                i_baseWidth -= ITEM_SPACING;
                rowWidthList << i_baseWidth;
                m_gridItem << itemInfoList;
                for (int j = 0; j <= corrent; j++) {
                    m_ItemList << m_ItemListAll.first();
                    m_ItemListAll.removeFirst();
                }
            }
        } else if (i_totalwidth - i_baseWidth > 200) {  //一行最后剩余宽度大于200   对当前图片进行缩放
            m_ItemListAll[corrent].imgHeight = m_ItemListAll[corrent].imgHeight * (i_totalwidth - i_baseWidth) / m_ItemListAll[corrent].imgWidth;
            m_ItemListAll[corrent].imgHeight = (1 > m_ItemListAll[corrent].imgHeight) ? 1 : m_ItemListAll[corrent].imgHeight;
            m_ItemListAll[corrent].imgWidth = i_totalwidth - i_baseWidth;
            m_ItemListAll[corrent].width = i_totalwidth - i_baseWidth;
            i_baseWidth = i_totalwidth;
            rowWidthList << i_baseWidth;
            itemInfoList << m_ItemListAll[corrent];
            m_gridItem << itemInfoList;
            for (int j = 0; j <= corrent; j++) {
                m_ItemList << m_ItemListAll.first();
                m_ItemListAll.removeFirst();
            }
            corrent = -1;
            i_baseWidth = 0;
            itemInfoList.clear();
        } else {
            i_baseWidth -= ITEM_SPACING;
            rowWidthList << i_baseWidth;
            i_baseWidth = m_ItemListAll[corrent].width + ITEM_SPACING;
            m_gridItem << itemInfoList;
            for (int j = 0; j < corrent; j++) {
                m_ItemList << m_ItemListAll.first();
                m_ItemListAll.removeFirst();
            }
            itemInfoList.clear();
            corrent = 0;
            itemInfoList << m_ItemListAll[corrent];
            if (m_allNeedRequestFilesCount < 1 && corrent == m_ItemListAll.size() - 1) {
                i_baseWidth -= ITEM_SPACING;
                rowWidthList << i_baseWidth;
                m_gridItem << itemInfoList;
                for (int j = 0; j <= corrent; j++) {
                    m_ItemList << m_ItemListAll.first();
                    m_ItemListAll.removeFirst();
                }
            }
        }
        corrent++;
    }
    m_ItemListLeft << m_ItemListAll;
    totlelength = rowWidthList.size();
    if (m_allNeedRequestFilesCount < 1)
        totlelength -= 1;
    // scaling for each row adapting list width except last one
    for (int i = 0; i < totlelength; i++) {
        if (i < (rowWidthList.size() - 1) && rowWidthList[i] < i_totalwidth && i < m_gridItem.size()) {
            int i_totalwidthExSpace = i_totalwidth - ITEM_SPACING * m_gridItem[i].size();
            int rowWidthListExSpace = rowWidthList[i] - ITEM_SPACING * m_gridItem[i].size();
            int rowWidth = 0;
            for (int j = 0; j < m_gridItem[i].size(); j++) {
                m_gridItem[i][j].width = m_gridItem[i][j].width * i_totalwidthExSpace / rowWidthListExSpace;
                m_gridItem[i][j].height = m_gridItem[i][j].height * i_totalwidthExSpace / rowWidthListExSpace;
                m_gridItem[i][j].imgWidth = m_gridItem[i][j].imgWidth * i_totalwidthExSpace / rowWidthListExSpace;
                m_gridItem[i][j].imgHeight = m_gridItem[i][j].imgHeight * i_totalwidthExSpace / rowWidthListExSpace;
                rowWidth = rowWidth + m_gridItem[i][j].width + ITEM_SPACING;
            }
            rowWidthList[i] = rowWidth - ITEM_SPACING;
            if (rowWidthList[i] < i_totalwidth) {
                m_gridItem[i][0].width = m_gridItem[i][0].width + i_totalwidth - rowWidthList[i];
            }
        }
    }
    if (0 < m_gridItem.size()) {
        m_height = 0;
        for (int i = 0; i < rowWidthList.size(); i++) {
            m_height = m_height + m_gridItem[i][0].height + ITEM_SPACING;
        }
        m_height -= ITEM_SPACING;
    }
}

void ThumbnailListView::addThumbnailViewNew(QList<QList<ItemInfo>> gridItem)
{
    for (int i = 0; i < gridItem.length(); i++) {
        for (int j = 0; j < gridItem[i].length(); j++) {
            QStandardItem *item = new QStandardItem;
            QString qsfirstorlast = "NotFirstOrLast";
            int height = gridItem[i][j].height;
            if (ThumbnailDelegate::AllPicViewType == m_delegatetype) {
                if (i == 0 && bfirstload) {
                    height += 50;
                    qsfirstorlast = "First";
                } else if (i == gridItem.length() - 1 && blastload) {
                    height += 27;
                    qsfirstorlast = "Last";
                }
            } else if (ThumbnailDelegate::AlbumViewType == m_delegatetype) {
                if (i == gridItem.length() - 1 && blastload) {
                    height += 27;
                    qsfirstorlast = "Last";
                }
            } else if (ThumbnailDelegate::SearchViewType == m_delegatetype || ThumbnailDelegate::AlbumViewPhoneType == m_delegatetype) {
                if (i == 0 && bfirstload) {
                    height += 130;
                    qsfirstorlast = "First";
                } else if (i == gridItem.length() - 1 && blastload) {
                    height += 27;
                    qsfirstorlast = "Last";
                }
            }
            QVariantList datas;
            datas.append(QVariant(gridItem[i][j].name));
            datas.append(QVariant(gridItem[i][j].path));
            datas.append(QVariant(gridItem[i][j].width));
            datas.append(QVariant(gridItem[i][j].height));
            datas.append(QVariant(gridItem[i][j].remainDays));
            datas.append(QVariant(gridItem[i][j].image));
            datas.append(QVariant(gridItem[i][j].imgWidth));
            datas.append(QVariant(gridItem[i][j].imgHeight));
            datas.append(QVariant(gridItem[i][j].baseWidth));
            datas.append(QVariant(gridItem[i][j].baseHeight));
            datas.append(QVariant(qsfirstorlast));
            datas.append(QVariant(gridItem[i][j].bNotSupportedOrDamaged));
            item->setData(QVariant(datas), Qt::DisplayRole);
            item->setData(QVariant(QSize(gridItem[i][j].width, height)),
                          Qt::SizeHintRole);
            m_model->appendRow(item);
        }
    }
    m_gridItem << gridItem;
    int hightlast = m_height;
    if (0 < m_gridItem.size()) {
        m_height = 0;
        for (int i = 0; i < m_gridItem.size(); i++) {
            m_height = m_height + m_gridItem[i][0].height + ITEM_SPACING;
        }
        m_height -= ITEM_SPACING;
    }
    if (hightlast != m_height) {
        sendNeedResize();
    }
}

void ThumbnailListView::addThumbnailView()
{
    QModelIndexList mlist = getSelectedIndexes();
    QMap<int, QPair<int, int>> items;
    int key = 0;
    for (QModelIndex i : mlist) {
        items.insert(key, QPair<int, int>(i.row(), i.column()));
        key++;
    }
    m_model->clear();
    for (int i = 0; i < m_gridItem.length(); i++) {
        for (int j = 0; j < m_gridItem[i].length(); j++) {
            QStandardItem *item = new QStandardItem;
            QString qsfirstorlast = "NotFirstOrLast";
            int height = m_gridItem[i][j].height;
            if (ThumbnailDelegate::AllPicViewType == m_delegatetype) {
                if (i == 0) {
                    height += 50;
                    qsfirstorlast = "First";
                } else if (i == m_gridItem.length() - 1) {
                    height += 27;
                    qsfirstorlast = "Last";
                }
            } else if (ThumbnailDelegate::AlbumViewType == m_delegatetype) {
                if (i == m_gridItem.length() - 1) {
                    height += 27;
                    qsfirstorlast = "Last";
                }
            } else if (ThumbnailDelegate::SearchViewType == m_delegatetype || ThumbnailDelegate::AlbumViewPhoneType == m_delegatetype) {
                if (i == 0) {
                    height += 130;
                    qsfirstorlast = "First";
                } else if (i == m_gridItem.length() - 1) {
                    height += 27;
                    qsfirstorlast = "Last";
                }
            }
            QVariantList datas;
            datas.append(QVariant(m_gridItem[i][j].name));
            datas.append(QVariant(m_gridItem[i][j].path));
            datas.append(QVariant(m_gridItem[i][j].width));
            datas.append(QVariant(m_gridItem[i][j].height));
            datas.append(QVariant(m_gridItem[i][j].remainDays));
            datas.append(QVariant(m_gridItem[i][j].image));
            datas.append(QVariant(m_gridItem[i][j].imgWidth));
            datas.append(QVariant(m_gridItem[i][j].imgHeight));
            datas.append(QVariant(m_gridItem[i][j].baseWidth));
            datas.append(QVariant(m_gridItem[i][j].baseHeight));
            datas.append(QVariant(qsfirstorlast));
            datas.append(QVariant(m_gridItem[i][j].bNotSupportedOrDamaged));
            item->setData(QVariant(datas), Qt::DisplayRole);
            //item->setData(1, Qt::UserRole);
            item->setData(QVariant(QSize(m_gridItem[i][j].width, height)),
                          Qt::SizeHintRole);
            m_model->appendRow(item);
        }
    }

//  设置更新之前的选择状态
    for (auto it : items) {
        if (it.first < m_model->rowCount()
                && it.second < m_model->columnCount()) {
            QModelIndex qindex = m_model->index(it.first, it.second);
            selectionModel()->select(qindex, QItemSelectionModel::Select);
        }
    }
}

void ThumbnailListView::updateThumbnailView(QString updatePath)
{
    int index = 0;
    for (int i = 0; i < m_gridItem.length(); i++) {
        for (int j = 0; j < m_gridItem[i].length(); j++) {
            if (m_gridItem[i][j].path == updatePath) {  //需要旋转的图片
                ImageDataSt data;
                ImageEngineApi::instance()->getImageData(updatePath, data);
                ItemInfo info;
                if (data.imgpixmap.isNull()) {
                    info.bNotSupportedOrDamaged = true;
                    data.imgpixmap = getDamagedPixmap();
                }
                info.name = data.dbi.fileName;
                info.path = data.dbi.filePath;
                info.width = data.imgpixmap.width();
                info.height = data.imgpixmap.height();
                info.image = data.imgpixmap;
                info.remainDays = data.remainDays;
                info.baseWidth = data.imgpixmap.width();
                info.baseHeight = data.imgpixmap.height();
                calBasePixMap(info);
                for (auto &item : m_ItemList) {     //替换
                    if (item == m_gridItem[i][j])
                        item = info;
                }
                m_gridItem[i][j] = info;

                int height = m_gridItem[i][j].height;
                QVariantList newdatas;
                QString qsfirstorlast = "NotFirstOrLast";
                if (ThumbnailDelegate::AllPicViewType == m_delegatetype) {
                    if (i == 0) {
                        height += 50;
                        qsfirstorlast = "First";
                    } else if (i == m_gridItem.length() - 1) {
                        height += 27;
                        qsfirstorlast = "Last";
                    }
                } else if (ThumbnailDelegate::AlbumViewType == m_delegatetype) {
                    if (i == m_gridItem.length() - 1) {
                        height += 27;
                        qsfirstorlast = "Last";
                    }
                } else if (ThumbnailDelegate::SearchViewType == m_delegatetype || ThumbnailDelegate::AlbumViewPhoneType == m_delegatetype) {
                    if (i == 0) {
                        height += 130;
                        qsfirstorlast = "First";
                    } else if (i == m_gridItem.length() - 1) {
                        height += 27;
                        qsfirstorlast = "Last";
                    }
                }
                newdatas.append(QVariant(m_gridItem[i][j].name));
                newdatas.append(QVariant(m_gridItem[i][j].path));
                newdatas.append(QVariant(m_gridItem[i][j].width));
                newdatas.append(QVariant(m_gridItem[i][j].height));
                newdatas.append(QVariant(m_gridItem[i][j].remainDays));
                newdatas.append(QVariant(m_gridItem[i][j].image));
                newdatas.append(QVariant(m_gridItem[i][j].imgWidth));
                newdatas.append(QVariant(m_gridItem[i][j].imgHeight));
                newdatas.append(QVariant(m_gridItem[i][j].baseWidth));
                newdatas.append(QVariant(m_gridItem[i][j].baseHeight));
                newdatas.append(QVariant(qsfirstorlast));
                newdatas.append(QVariant(m_gridItem[i][j].bNotSupportedOrDamaged));
                m_model->item(index, 0)->setData(QVariant(newdatas), Qt::DisplayRole);
                m_model->item(index, 0)->setData(QVariant(QSize(m_gridItem[i][j].width, /*m_gridItem[i][j].height*/height)),
                                                 Qt::SizeHintRole);
                QSize picSize(m_gridItem[i][j].width, /*m_gridItem[i][j].height*/height);
                m_model->item(index, 0)->setSizeHint(picSize);
            }
            index++;
        }
    }
    //更新布局
    calBasePixMapWandH();
    calWidgetItemWandH();
    updateThumbnaillistview();
//    addThumbnailView();
    emit needResize(m_height + 15);     //调整整体大小
}

void ThumbnailListView::loadFilesFromDB(QString name, int loadCount)
{
    ImageEngineApi::instance()->loadImagesFromDB(m_delegatetype, this, name, loadCount);
}

bool ThumbnailListView::imageFromDBLoaded(QStringList &filelist)
{
    emit sigDBImageLoaded();
    stopLoadAndClear();
    m_allfileslist << filelist;
    m_filesbeleft << filelist;
    m_allNeedRequestFilesCount += filelist.size();
    calWidgetItemWandH();
    addThumbnailView();
    if (bneedloadimage) {
        requestSomeImages();
    }
    sendNeedResize();
    return true;
}

void ThumbnailListView::loadFilesFromLocal(QStringList files, bool needcache, bool needcheck)
{
    qDebug() << "zy------ThumbnailListView::loadFilesFromLocal";
    ImageEngineApi::instance()->loadImagesFromLocal(files, this, needcheck);
    bneedcache = needcache;
}

void ThumbnailListView::loadFilesFromLocal(DBImgInfoList files, bool needcache, bool needcheck)
{
    ImageEngineApi::instance()->loadImagesFromLocal(files, this, needcheck);
    bneedcache = needcache;
}

void ThumbnailListView::loadFilesFromTrash(DBImgInfoList files)
{
    ImageEngineApi::instance()->loadImagesFromTrash(files, this);
}

bool ThumbnailListView::imageLocalLoaded(QStringList &filelist)
{
    stopLoadAndClear();
    m_allfileslist << filelist;
    m_filesbeleft << filelist;
    m_allNeedRequestFilesCount += filelist.size();
    calWidgetItemWandH();
    addThumbnailView();
    if (bneedloadimage) {
        requestSomeImages();
    }
    sendNeedResize();
    return true;
}

void ThumbnailListView::requestSomeImages()
{
    //QMutexLocker mutex(&m_mutex);
    bneedloadimage = false;

    if (m_filesbeleft.size() < Number_Of_Displays_Per_Time) {
        m_requestCount += m_filesbeleft.size();
    } else {
        m_requestCount += Number_Of_Displays_Per_Time;
    }
    for (int i = 0; i < Number_Of_Displays_Per_Time; i++) {
        if (m_filesbeleft.size() <= 1) {
            brequestallfiles = true;
        }
        if (m_filesbeleft.size() <= 0) {
            return;
        }
        QString firstfilesbeleft = m_filesbeleft.first();
        m_filesbeleft.removeFirst();
        bool useGlobalThreadPool = true;
        if (m_useFor == Mount) {
            useGlobalThreadPool = false;
        }
        ImageEngineApi::instance()->reQuestImageData(firstfilesbeleft, this, bneedcache, useGlobalThreadPool);
    }
}

bool ThumbnailListView::imageLoaded(QString filepath)
{
    m_requestCount--;
    m_allNeedRequestFilesCount--;
    if (m_requestCount < 1) {
        if (brequestallfiles) {
            blastload = true;
            emit loadEnd();
        }
    }
    ImageDataSt data;
    bool reb = true;
    if (ImageEngineApi::instance()->getImageData(filepath, data)) {
        ItemInfo info;

        if (data.imgpixmap.isNull()) {
            info.bNotSupportedOrDamaged = true;
            data.imgpixmap = getDamagedPixmap();
        }

        info.name = data.dbi.fileName;
        info.path = data.dbi.filePath;
        info.width = data.imgpixmap.width();
        info.height = data.imgpixmap.height();
        info.image = data.imgpixmap;
//        info.bNotSupportedOrDamaged = data.imgpixmap.isNull();
        info.remainDays = data.remainDays;
        info.baseWidth = data.imgpixmap.width();
        info.baseHeight = data.imgpixmap.height();
        insertThumbnail(info);
        reb = false;
    }
    if (m_requestCount < 1) {
        requestSomeImages();
    }
    return reb;
}

void ThumbnailListView::insertThumbnail(const ItemInfo &iteminfo)
{
    ItemInfo info = iteminfo;
    calBasePixMap(info);
    m_ItemListLeft << info;
    calWidgetItem();
    if (nullptr != m_item) {
        m_item->setSizeHint(QSize(this->width(), m_height + 27 + 8)/*this->size()*/);
        this->resize(QSize(this->width(), m_height + 27 + 8)/*this->size()*/);
    }
}

void ThumbnailListView::stopLoadAndClear(bool bClearModel)
{
    clearAndStopThread();
    if (bClearModel)
        m_model->clear();   //清除模型中的数据
    m_allfileslist.clear();
    m_filesbeleft.clear();
    m_allNeedRequestFilesCount = 0;
    bneedloadimage = true;
    brequestallfiles = false;
    m_ItemListLeft.clear();
    m_requestCount = 0;
    m_ItemList.clear();
    m_gridItem.clear();
    blastload = false;
    bfirstload = true;
}

QStringList ThumbnailListView::getAllFileList()
{
    return m_allfileslist;
}

//void ThumbnailListView::setListWidgetItem(QListWidgetItem *item)
//{
//    m_item = item;
//    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//}

void ThumbnailListView::setIBaseHeight(int iBaseHeight)
{
    m_iBaseHeight = iBaseHeight;
}

//void ThumbnailListView::setVScrollbarDistance(int topdistance, int bottomdistance)
//{
//    m_scrollbartopdistance = topdistance;
//    m_scrollbarbottomdistance = bottomdistance;
//}

void ThumbnailListView::onShowMenu(const QPoint &pos)
{
    //外接设备显示照片时，禁用鼠标右键菜单
    if (!this->indexAt(pos).isValid() || ALBUM_PATHTYPE_BY_PHONE == m_imageType) {
        return;
    }
    emit sigMouseRelease();
    updateMenuContents();
    m_pMenu->popup(QCursor::pos());

}

void ThumbnailListView::updateMenuContents()
{
    QStringList paths;
    if (m_imageType == COMMON_STR_VIEW_TIMELINE || m_imageType == COMMON_STR_RECENT_IMPORTED) {
        emit sigGetSelectedPaths(&paths);
    } else {
        paths = selectedPaths();
    }
    paths.removeAll(QString(""));
    if (paths.empty()) {
        return;
    }
    if (m_imageType.compare(COMMON_STR_TRASH) == 0) {
        if (1 == paths.length())
            m_MenuActionMap.value(tr("Photo info"))->setVisible(true);
        else
            m_MenuActionMap.value(tr("Photo info"))->setVisible(false);
        return;
    }
    for (QAction *action : m_MenuActionMap.values()) {
        action->setVisible(true);
        action->setEnabled(true);
    }
    if ((1 == paths.length()) && (!QFileInfo(paths[0]).exists()) && (COMMON_STR_TRASH != m_imageType)) {
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
        m_MenuActionMap.value(tr("Delete"))->setEnabled(false);
        m_MenuActionMap.value(tr("Remove from album"))->setVisible(false);
        m_MenuActionMap.value(tr("Print"))->setVisible(false);
        if (DBManager::instance()->isImgExistInAlbum(COMMON_STR_FAVORITES, paths[0], AlbumDBType::Favourite)) {
            m_MenuActionMap.value(tr("Favorite"))->setVisible(false);
            m_MenuActionMap.value(tr("Unfavorite"))->setEnabled(false);
        } else {
            m_MenuActionMap.value(tr("Unfavorite"))->setVisible(false);
            m_MenuActionMap.value(tr("Favorite"))->setEnabled(false);
        }
        m_MenuActionMap.value(tr("Rotate clockwise"))->setEnabled(false);
        m_MenuActionMap.value(tr("Rotate counterclockwise"))->setEnabled(false);
        m_MenuActionMap.value(tr("Display in file manager"))->setEnabled(false);
        m_MenuActionMap.value(tr("Photo info"))->setEnabled(false);
        m_MenuActionMap.value(tr("Set as wallpaper"))->setEnabled(false);
        return;
    }
    if (1 != paths.length()) {
        m_MenuActionMap.value(tr("View"))->setVisible(false);
        m_MenuActionMap.value(tr("Fullscreen"))->setVisible(false);
    }
    if (COMMON_STR_TRASH == m_imageType) {
        m_MenuActionMap.value(tr("Delete"))->setVisible(false);
    } else {
        m_albumMenu->deleteLater();
        m_albumMenu = createAlbumMenu();
        if (m_albumMenu) {
            QAction *action = m_MenuActionMap.value(tr("Export"));
            m_pMenu->insertMenu(action, m_albumMenu);
        }
    }
    if (1 == paths.length() && COMMON_STR_TRASH != m_imageType) {
        if (COMMON_STR_RECENT_IMPORTED == m_imageType || IMAGE_DEFAULTTYPE == m_imageType ||
                COMMON_STR_VIEW_TIMELINE == m_imageType || COMMON_STR_FAVORITES == m_imageType) {
            m_MenuActionMap.value(tr("Remove from album"))->setVisible(false);
        }
        if (DBManager::instance()->isImgExistInAlbum(COMMON_STR_FAVORITES, paths[0], AlbumDBType::Favourite)) {
            m_MenuActionMap.value(tr("Favorite"))->setVisible(false);
        } else {
            m_MenuActionMap.value(tr("Unfavorite"))->setVisible(false);
        }
        m_pMenu->addSeparator();
    } else {
        if (COMMON_STR_RECENT_IMPORTED == m_imageType || IMAGE_DEFAULTTYPE == m_imageType ||
                COMMON_STR_VIEW_TIMELINE == m_imageType) {
            m_MenuActionMap.value(tr("Remove from album"))->setVisible(false);
        }
        if (COMMON_STR_FAVORITES == m_imageType) {
            m_MenuActionMap.value(tr("Remove from album"))->setVisible(false);
        }
        m_MenuActionMap.value(tr("Unfavorite"))->setVisible(false);
        m_MenuActionMap.value(tr("Favorite"))->setVisible(false);
    }
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
}

void ThumbnailListView::appendAction(int id, const QString &text, const QString &shortcut)
{
    QAction *ac = new QAction();
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
    appendAction(IdView, tr("View"), ss(VIEW_CONTEXT_MENU));
    appendAction(IdFullScreen, tr("Fullscreen"), ss(FULLSCREEN_CONTEXT_MENU));
    appendAction(IdPrint, tr("Print"), ss(PRINT_CONTEXT_MENU));
    appendAction(IdStartSlideShow, tr("Slide show"), ss(SLIDESHOW_CONTEXT_MENU));
    m_pMenu->addSeparator();
    appendAction(IdExport, tr("Export"), ss(EXPORT_CONTEXT_MENU));
    appendAction(IdCopyToClipboard, tr("Copy"), ss(COPYTOCLIPBOARD_CONTEXT_MENU));
    appendAction(IdMoveToTrash, tr("Delete"), ss(""));
    appendAction(IdRemoveFromAlbum, tr("Remove from album"), ss(""));
    m_pMenu->addSeparator();
    appendAction(IdAddToFavorites, tr("Favorite"), "");
    appendAction(IdRemoveFromFavorites, tr("Unfavorite"), "");
    m_pMenu->addSeparator();
    appendAction(IdRotateClockwise, tr("Rotate clockwise"), ss(ROTATECLOCKWISE_CONTEXT_MENU));
    appendAction(IdRotateCounterclockwise, tr("Rotate counterclockwise"),
                 ss(ROTATECOUNTERCLOCKWISE_CONTEXT_MENU));
    m_pMenu->addSeparator();
    appendAction(IdSetAsWallpaper, tr("Set as wallpaper"), ss(SETASWALLPAPER_CONTEXT_MENU));
    appendAction(IdDisplayInFileManager, tr("Display in file manager"), ss(DISPLAYINFILEMANAGER_CONTEXT_MENU));
    appendAction(IdImageInfo, tr("Photo info"), ss(ImageInfo_CONTEXT_MENU));
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
    for (QString album : albums) {
        QAction *ac = new QAction(am);
        ac->setProperty("MenuID", IdAddToAlbum);
        ac->setText(
            fontMetrics().elidedText(QString(album).replace("&", "&&"), Qt::ElideMiddle, 200));
        ac->setData(album);
        am->addAction(ac);
    }
    return am;
}

void ThumbnailListView::onMenuItemClicked(QAction *action)
{
    if (m_imageType == COMMON_STR_VIEW_TIMELINE || m_imageType == COMMON_STR_RECENT_IMPORTED) {
        emit sigMenuItemDeal(action);
    } else {
        QStringList paths = selectedPaths();
        menuItemDeal(paths, action);
    }
}

QStringList ThumbnailListView::selectedPaths()
{
    QStringList paths;
    for (QModelIndex index : selectionModel()->selectedIndexes()) {
        const QVariantList datas = index.model()->data(index, Qt::DisplayRole).toList();
        if (datas.length() >= 8) {
            paths << datas[1].toString();
        }
    }
    return paths;
}

QStringList ThumbnailListView::getDagItemPath()
{
    return m_dragItemPath;
}

QStringList ThumbnailListView::getAllPaths()
{
    return m_allfileslist;
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
        emit menuOpenImage(path, paths, false, false);
        break;
    case IdFullScreen:
        emit menuOpenImage(path, paths, true, false);
        break;
    case IdPrint:
        PrintHelper::showPrintDialog(paths, this);
        break;
    case IdStartSlideShow:
        emit menuOpenImage(path, paths, true, true);
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
        } else {
            emit dApp->signalM->createAlbum(paths);
        }
        break;
    }
    case IdCopyToClipboard:
        utils::base::copyImageToClipboard(paths);
        break;
    case IdMoveToTrash: {
        ImgDeleteDialog *dialog = new ImgDeleteDialog(this, paths.length());
        if (dialog->exec())
            (COMMON_STR_TRASH == m_imageType)? ImageEngineApi::instance()->moveImagesToTrash(paths, true, false) : ImageEngineApi::instance()->moveImagesToTrash(paths);
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
            DBManager::instance()->removeFromAlbum(m_imageType, paths);
        }
    }
    break;
    case IdRotateClockwise: {
        QString errMsg;
        if (!UnionImage_NameSpace::rotateImageFIle(90, path, errMsg)) {
            qDebug() << errMsg;
            return;
        }
        dApp->m_imageloader->updateImageLoader(paths);
    }
    break;
    case IdRotateCounterclockwise: {
        QString errMsg;
        if (!UnionImage_NameSpace::rotateImageFIle(-90, path, errMsg)) {
            qDebug() << errMsg;
            return;
        }
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
    calBasePixMapWandH();
    calWidgetItemWandH();
    updateThumbnaillistview();      //改用新的调整位置--xioalong
//    addThumbnailView();//耗时最长
    sendNeedResize();
}

void ThumbnailListView::onCancelFavorite(const QModelIndex &index)
{
    QStringList str;
    QVariantList datas = index.model()->data(index, Qt::DisplayRole).toList();

    if (datas.length() >= 2) {
        str << datas[1].toString();
    }
    //通知其它界面更新取消收藏
    DBManager::instance()->removeFromAlbum(COMMON_STR_FAVORITES, str, AlbumDBType::Favourite);
    emit dApp->signalM->updateFavoriteNum();
    m_model->removeRow(index.row());
    m_ItemList.removeAt(index.row());
    calWidgetItemWandH();
    updateThumbnailView();
    sendNeedResize();
}

void ThumbnailListView::resizeEvent(QResizeEvent *e)
{
    qDebug() << "zy--------ThumbnailListView::resizeEvent";
    if (e->size().width() == e->oldSize().width()) {
//        qDebug() << "宽度没有改变，证明是导入图片改变了高度";
        return;
    }
    resizeEventF();
}

bool ThumbnailListView::eventFilter(QObject *obj, QEvent *e)
{
    Q_UNUSED(obj)
    if (e->type() == QEvent::Wheel && QApplication::keyboardModifiers() == Qt::ControlModifier) {
        return true;
    }
    // add for pageup pagedown for time line view.
    else if (e->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
        if (COMMON_STR_VIEW_TIMELINE == m_imageType || COMMON_STR_RECENT_IMPORTED == m_imageType) {
            if (keyEvent->key() == Qt::Key_PageDown || keyEvent->key() == Qt::Key_PageUp) {
                //处理上下翻页
                emit sigKeyEvent(keyEvent->key());
                return true;
            }
        } else {
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
    }
    return false;
}

QPixmap ThumbnailListView::getDamagedPixmap()
{
    return utils::image::getDamagePixmap(DApplicationHelper::instance()->themeType() == DApplicationHelper::LightType);
}

void ThumbnailListView::updateThumbnaillistview()
{
    int index = 0;
    for (int i = 0; i < m_gridItem.length(); i++) {
        for (int j = 0; j < m_gridItem[i].length(); j++) {
            QString qsfirstorlast = "NotFirstOrLast";
            int height = m_gridItem[i][j].height;
            if (ThumbnailDelegate::AllPicViewType == m_delegatetype) {
                if (i == 0) {
                    height += 50;
                    qsfirstorlast = "First";
                } else if (i == m_gridItem.length() - 1) {
                    height += 27;
                    qsfirstorlast = "Last";
                }
            } else if (ThumbnailDelegate::AlbumViewType == m_delegatetype) {
                if (i == m_gridItem.length() - 1) {
                    height += 27;
                    qsfirstorlast = "Last";
                }
            } else if (ThumbnailDelegate::SearchViewType == m_delegatetype || ThumbnailDelegate::AlbumViewPhoneType == m_delegatetype) {
                if (i == 0) {
                    height += 130;
                    qsfirstorlast = "First";
                } else if (i == m_gridItem.length() - 1) {
                    height += 27;
                    qsfirstorlast = "Last";
                }
            }

            QVariantList datas;
            datas.append(QVariant(m_gridItem[i][j].name));
            datas.append(QVariant(m_gridItem[i][j].path));
            datas.append(QVariant(m_gridItem[i][j].width));
            datas.append(QVariant(m_gridItem[i][j].height));
            datas.append(QVariant(m_gridItem[i][j].remainDays));
            if (m_gridItem[i][j].bNotSupportedOrDamaged) {
                m_gridItem[i][j].image = getDamagedPixmap();
            }
            datas.append(QVariant(m_gridItem[i][j].image));
            datas.append(QVariant(m_gridItem[i][j].imgWidth));
            datas.append(QVariant(m_gridItem[i][j].imgHeight));
            datas.append(QVariant(m_gridItem[i][j].baseWidth));
            datas.append(QVariant(m_gridItem[i][j].baseHeight));
            datas.append(QVariant(qsfirstorlast));
            datas.append(QVariant(m_gridItem[i][j].bNotSupportedOrDamaged));

            //更新值
            QStandardItem *newItem = m_model->item(index);
            if (newItem) {
                newItem->setData(QVariant(datas), Qt::DisplayRole);
                newItem->setData(QVariant(QSize(m_gridItem[i][j].width, height)),
                                 Qt::SizeHintRole);
                m_model->setItem(index++, newItem);
            }
        }
    }
    this->setSpacing(ITEM_SPACING);     //重新布局
}

#if 1
QModelIndexList ThumbnailListView::getSelectedIndexes()
{
    return selectedIndexes();
}

//void ThumbnailListView::selectCurrent(int row)
//{
//    QModelIndex qindex = m_model->index(row, 0);
//    selectionModel()->select(qindex, QItemSelectionModel::Select);
//}

int ThumbnailListView::getRow(QPoint point)
{
    return indexAt(point).row();
}

void ThumbnailListView::selectRear(int row)
{
    for (int i = row; i < m_model->rowCount(); i++) {
        QModelIndex qindex = m_model->index(i, 0);
        selectionModel()->select(qindex, QItemSelectionModel::Select);
    }
}

void ThumbnailListView::selectFront(int row)
{
    for (int i = row; i >= 0; i--) {
        QModelIndex qindex = m_model->index(i, 0);
        selectionModel()->select(qindex, QItemSelectionModel::Select);
    }
}

void ThumbnailListView::selectExtent(int start, int end)
{
    for (int i = start; i <= end; i++) {
        QModelIndex qindex = m_model->index(i, 0);
        selectionModel()->select(qindex, QItemSelectionModel::Select);
    }
}

void ThumbnailListView::clearSelectionRear(int row)
{
    for (int i = row; i < m_model->rowCount(); i++) {
        QModelIndex qindex = m_model->index(i, 0);
        selectionModel()->select(qindex, QItemSelectionModel::Deselect);
    }
}

void ThumbnailListView::clearSelectionFront(int row)
{
    for (int i = row; i >= 0; i--) {
        QModelIndex qindex = m_model->index(i, 0);
        selectionModel()->select(qindex, QItemSelectionModel::Deselect);
    }
}

void ThumbnailListView::clearSelectionExtent(int start, int end)
{
    for (int i = start; i <= end; i++) {
        QModelIndex qindex = m_model->index(i, 0);
        selectionModel()->select(qindex, QItemSelectionModel::Deselect);
    }
}

void ThumbnailListView::resizeHand()
{
    emit needResize(m_height + 15);
}

void ThumbnailListView::setListViewUseFor(ThumbnailListView::ListViewUseFor usefor)
{
    m_useFor = usefor;
}
#endif
//add start 3975
int ThumbnailListView::getListViewHeight()
{
    return m_height;
}
//add end 3975

void ThumbnailListView::onTimerOut()
{
    if (bneedsendresize && lastresizeheight != m_height) {
        resizenum++;
        emit needResize(m_height + 15);
        lastresizeheight = m_height;
    }
    bneedsendresize = false;
}

void ThumbnailListView::sltChangeDamagedPixOnThemeChanged()
{
    for (int i = 0; i < m_model->rowCount(); i++) {
        QModelIndex idx = m_model->index(i, 0);
        QVariantList lst = idx.model()->data(idx, Qt::DisplayRole).toList();
        if (lst.count() >= 12) {
            const bool &bNotSuppOrDmg = lst[11].toBool();
            if (bNotSuppOrDmg) {
                lst.replace(5, getDamagedPixmap());
                m_model->item(i, 0)->setData(lst, Qt::DisplayRole);
            }
        }
    }
}

void ThumbnailListView::selectDuplicateForOneListView(QStringList paths, QModelIndex &firstIndex)
{
    if (paths.count() > 0) {
        this->clearSelection();
        for (int i = 0; i < m_model->rowCount(); i++) {
            QModelIndex idx = m_model->index(i, 0);
            QVariantList lst = idx.model()->data(idx, Qt::DisplayRole).toList();
            if (lst.count() >= 12) {
                for ( int j = 0; j < paths.count(); j++) {
                    if (lst.at(1).toString() == paths.at(j)) {
                        // 选中
                        selectionModel()->select(idx, QItemSelectionModel::Select);
                        if (!firstIndex.isValid()){
                            firstIndex = idx;
                        }
                    }
                }
            }
        }
    }
}

// 选中重复导入的图片
void ThumbnailListView::selectDuplicatePhotos(QStringList paths, bool bMultiListView)
{
    // 针对单行listview处理
    if (!bMultiListView) {
        QTimer::singleShot(100, this, [ = ] {
            QModelIndex firstIndex;
            selectDuplicateForOneListView(paths, firstIndex);
            // 定位第一个重复导入的照片
            if (firstIndex.isValid()) {
                this->scrollTo(firstIndex);
            }
        });
    }
    // 多行处理在外部，针对时间线和已导入界面
    else {
        if (paths.count() > 0) {
            QModelIndex firstIndex;
            selectDuplicateForOneListView(paths, firstIndex);
        }
    }
}

void ThumbnailListView::slotReCalcTimelineSize()
{
    emit needResize(m_height + 15);
}

void ThumbnailListView::slotLoad80ThumbnailsFinish()
{
    qDebug() << "zy------ThumbnailListView::slotLoad80ThumbnailsFinish";
    for (auto data : ImageEngineApi::instance()->m_AllImageData) {
        ItemInfo info;
        if (data.imgpixmap.isNull()) {
            info.bNotSupportedOrDamaged = true;
            data.imgpixmap = getDamagedPixmap();
        }
        info.name = data.dbi.fileName;
        info.path = data.dbi.filePath;
        info.width = data.imgpixmap.width();
        info.height = data.imgpixmap.height();
        info.image = data.imgpixmap;
//        info.bNotSupportedOrDamaged = data.imgpixmap.isNull();
        info.remainDays = data.remainDays;
        info.baseWidth = data.imgpixmap.width();
        info.baseHeight = data.imgpixmap.height();
        insertThumbnail(info);
    }
    resizeEventF();
    emit sigLoad80ThumbnailsFinish();
}

void ThumbnailListView::sendNeedResize(/*int hight*/)
{
    if (!isVisible()) {
        return;
    }
    if (m_dt->isActive()) {
        bneedsendresize = true;
        return;
    }
    m_dt->start();
    if (lastresizeheight != m_height) {
        resizenum++;
        emit needResize(m_height + 15);
    }
    lastresizeheight = m_height;
    bneedsendresize = false;
}

void ThumbnailListView::resizeEventF()
{
    if (nullptr == m_item) {
        DScrollBar *bar = this->verticalScrollBar();
        bar->setGeometry(bar->x(), /*bar->y() + */m_scrollbartopdistance, bar->width(), this->height() - m_scrollbartopdistance - m_scrollbarbottomdistance);
    }
    calWidgetItemWandH();
//    addThumbnailView();
    updateThumbnaillistview();
    sendNeedResize();
    m_iDefaultWidth = width();
    if (nullptr != m_item) {
        m_item->setSizeHint(QSize(this->width(), getListViewHeight() + 8 + 27)/*this->size()*/);
        this->resize(QSize(this->width(), m_height + 27 + 8)/*this->size()*/);
    }
}

bool ThumbnailListView::checkResizeNum()
{
    resizenum--;
    if (resizenum > 0) {
        return false;
    }
    return true;
}

bool ThumbnailListView::isLoading()
{
    if (m_threads.empty()) {
        return false;
    }
    return true;
}

bool ThumbnailListView::isAllPicSeleted()
{
    return getAllPaths().count() == selectedPaths().count();
}
