#include "thumbnaillistview.h"
#include "application.h"
#include <QDebug>
#include <QDrag>
#include <QFileInfo>
#include <QImageReader>
#include <QMimeData>
#include <QScrollBar>
#include <QMutex>
#include "controller/signalmanager.h"
#include "controller/wallpapersetter.h"
#include "widgets/dialogs/imgdeletedialog.h"
//#include "timelinelist.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "utils/snifferimageformat.h"
#include "imageengine/imageengineapi.h"

namespace {
const int ITEM_SPACING = 4;
const int BASE_HEIGHT = 100;
const int LEFT_MARGIN = 12;
const int RIGHT_MARGIN = 8;

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
    :  DListView(parent), m_delegatetype(type), m_allfileslist()
{

    if (ThumbnailDelegate::AllPicViewType == m_delegatetype) {
        m_scrollbartopdistance = 50;
        m_scrollbarbottomdistance = 27;
    } else if (ThumbnailDelegate::AlbumViewType == m_delegatetype) {
        m_scrollbarbottomdistance = 27;
    } else if (ThumbnailDelegate::SearchViewType == m_delegatetype || ThumbnailDelegate::AlbumViewPhoneType == m_delegatetype) {
        m_scrollbartopdistance = 130;
        m_scrollbarbottomdistance = 27;
    }
    m_model = new QStandardItemModel(this);
//    setStyleSheet("Background:green");
    m_imageType = imgtype;

    m_iDefaultWidth = 0;
    m_iBaseHeight = BASE_HEIGHT;
    m_albumMenu = nullptr;

//        setViewportMargins(0, 0, -10, 0);
//    setIconSize(QSize(400, 400));
    setResizeMode(QListView::Adjust);
    setViewMode(QListView::IconMode);
    //    setFlow(QListView::LeftToRight);
    setSpacing(ITEM_SPACING);
    setDragEnabled(false);
    if (COMMON_STR_VIEW_TIMELINE == m_imageType) {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
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
    if ((QApplication::keyboardModifiers() == Qt::ShiftModifier && event->button() == Qt::LeftButton)
            && (m_imageType == COMMON_STR_VIEW_TIMELINE || m_imageType == COMMON_STR_RECENT_IMPORTED));
    else
        DListView::mousePressEvent(event);

    qDebug() << "m_imageType: " << m_imageType << " ;dragDropMode(): " << dragDropMode();
    if ((m_imageType != COMMON_STR_VIEW_TIMELINE) && (m_imageType != "All Photos") &&
            (m_imageType != COMMON_STR_TRASH) && (m_imageType != ALBUM_PATHTYPE_BY_PHONE)) {
        if (dragDropMode() != NoDragDrop) {
            setDragDropMode(DragDrop);
        }
    } else {
        setDragEnabled(false);
    }

    bool isListArea = this->indexAt(event->pos()).isValid();
    qDebug() << "isListArea:" << isListArea << ", keyboard:" << QApplication::keyboardModifiers();
    if (!isListArea) {
        if (QApplication::keyboardModifiers() != Qt::ControlModifier) {
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
    qDebug() << "ThumbnailListView::mouseMoveEvent()";
    emit sigMouseMove();
    DListView::mouseMoveEvent(event);
}

void ThumbnailListView::startDrag(Qt::DropActions supportedActions)
{
    qDebug() << "ThumbnailListView::startDrag()";
//        m_dragItemPath = selectedPaths();
    //    qDebug() << m_dragItemPath;

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
    //    pDrag->setHotSpot(QPoint(0, 0));
    pDrag->exec(Qt::MoveAction);

//    DListView::startDrag(supportedActions);
}

void ThumbnailListView::mouseReleaseEvent(QMouseEvent *event)
{
    qDebug() << "ThumbnailListView::mouseReleaseEvent()";
    DListView::mouseReleaseEvent(event);
    if (COMMON_STR_RECENT_IMPORTED  == m_imageType) {
        if (QApplication::keyboardModifiers() == Qt::NoModifier) {
            emit sigMouseRelease();
        }
    } else {
        emit sigMouseRelease();
    }
}

void ThumbnailListView::keyPressEvent(QKeyEvent *event)
{
    qDebug() << "ThumbnailListView::keyPressEvent()";
    DListView::keyPressEvent(event);
//    if (m_imageType == COMMON_STR_RECENT_IMPORTED || m_imageType == COMMON_STR_VIEW_TIMELINE) {
    if ((event->modifiers() == Qt::ControlModifier) && (event->key() == Qt::Key_A)) {
        emit sigSelectAll();
    }
//    }

    m_dragItemPath = selectedPaths();
    qDebug() << m_dragItemPath;
}

void ThumbnailListView::dragEnterEvent(QDragEnterEvent *event)
{
    qDebug() << "ThumbnailListView::dragEnterEvent()";

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

    //    DListView::dragEnterEvent(event);
}

void ThumbnailListView::dragMoveEvent(QDragMoveEvent *event)
{
    qDebug() << "ThumbnailListView::dragMoveEvent()";

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

    //    DListView::dragMoveEvent(event);
}

void ThumbnailListView::dragLeaveEvent(QDragLeaveEvent *event)
{
    qDebug() << "ThumbnailListView::dragLeaveEvent()";
    m_dragItemPath = selectedPaths();
    qDebug() << m_dragItemPath;
    DListView::dragLeaveEvent(event);
}

void ThumbnailListView::dropEvent(QDropEvent *event)
{
//    emit sigDrop();
    qDebug() << "ThumbnailListView::dropEvent()";
    if (event->mimeData()->hasFormat("TestListView/text-icon-icon_hover"))
        return;
    DListView::dropEvent(event);
}

void ThumbnailListView::initConnections()
{
    connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, [ = ](int value) {
        if (value >= (this->verticalScrollBar()->maximum())) {
            if (m_requestCount > 0) {
                bneedloadimage = true;
            } else {
                requestSomeImages();
            }
        }
    });
    connect(this->verticalScrollBar(), &QScrollBar::rangeChanged, this, [ = ](int min, int max) {
        QScrollBar *bar = this->verticalScrollBar();
        bar->setGeometry(bar->x(), /*bar->y() + */m_scrollbartopdistance, bar->width(), this->height() - m_scrollbartopdistance - m_scrollbarbottomdistance);
    });
    connect(this, &QListView::customContextMenuRequested, this, &ThumbnailListView::onShowMenu);
    connect(m_pMenu, &DMenu::triggered, this, &ThumbnailListView::onMenuItemClicked);
    connect(this, &ThumbnailListView::doubleClicked, this, [ = ](const QModelIndex & index) {
        if (ALBUM_PATHTYPE_BY_PHONE != m_imageType) {
            qDebug() << "index is " << index.row();
            if (m_imageType.compare(COMMON_STR_TRASH) != 0) {
                emit openImage(index.row());
            }
        }
    });
    connect(this, &ThumbnailListView::clicked, this, [ = ]() {
        emit hideExtensionPanel();
    });
    connect(dApp->signalM, &SignalManager::sigMainwindowSliderValueChg, this,
            &ThumbnailListView::onPixMapScale);
    connect(m_delegate, &ThumbnailDelegate::sigCancelFavorite, this,
            &ThumbnailListView::onCancelFavorite);
//    connect(m_delegate, &ThumbnailDelegate::sigPageNeedResize, this,
//            &ThumbnailListView::slotPageNeedResize);
}

void ThumbnailListView::calBasePixMap(ItemInfo &info)
{
    //    int i_totalwidth = width() - 36;  // same as i_totalwidth in calWidgetItemWandH()
    int i_totalwidth = window()->width() - 30;  // same as i_totalwidth in calWidgetItemWandH()
    if (0 == info.height || 0 == info.width || ((float)info.height) / ((float)info.width) > 3) {
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
//    int i_totalwidth = width() - 36;  // same as i_totalwidth in calWidgetItemWandH()
    int i_totalwidth = window()->width() - 30;  // same as i_totalwidth in calWidgetItemWandH()

    for (int i = 0; i < m_ItemList.length(); i++) {
        if (0 == m_ItemList[i].height || 0 == m_ItemList[i].width) {
            m_ItemList[i].width = m_iBaseHeight;
            m_ItemList[i].height = m_iBaseHeight;
        } else {
//            m_ItemList[i].width = m_ItemList[i].baseWidth * m_iBaseHeight / m_ItemList[i].height;
            m_ItemList[i].width = m_ItemList[i].width * m_iBaseHeight / m_ItemList[i].height;
            if (m_ItemList[i].width > i_totalwidth) {
//                m_ItemList[i].height = m_ItemList[i].height * i_totalwidth / 4 / m_ItemList[i].width;
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
                gridItem[i][j].width =
                    gridItem[i][j].width * i_totalwidthExSpace / rowWidthListExSpace;
                gridItem[i][j].height =
                    gridItem[i][j].height * i_totalwidthExSpace / rowWidthListExSpace;
                gridItem[i][j].imgWidth =
                    gridItem[i][j].imgWidth * i_totalwidthExSpace / rowWidthListExSpace;
                gridItem[i][j].imgHeight =
                    gridItem[i][j].imgHeight * i_totalwidthExSpace / rowWidthListExSpace;

                rowWidth = rowWidth + gridItem[i][j].width + ITEM_SPACING;
            }

            rowWidthList[i] = rowWidth - ITEM_SPACING;

            if (rowWidthList[i] < i_totalwidth) {
                gridItem[i][0].width = gridItem[i][0].width + i_totalwidth - rowWidthList[i];
            }
        }
    }

//    if (0 < gridItem.size()) {
//        m_height = 0;
//        for (int i = 0; i < rowWidthList.length(); i++) {
//            m_height = m_height + gridItem[i][0].height + ITEM_SPACING;
//        }
//        m_height -= ITEM_SPACING;
//    }
    addThumbnailViewNew(gridItem);
    if (gridItem.size() > 0) {
        bfirstload = false;
    }
}

void ThumbnailListView::calWidgetItemWandH()
{
    int i_baseWidth = 0;
//    int i_totalwidth = width() - 36;  // same as i_totalwidth in calBasePixMapWandH()
    int i_totalwidth = width() - 30;  // same as i_totalwidth in calBasePixMapWandH()


    QList<int> rowWidthList;
    QList<ItemInfo> itemInfoList;

    QList<ItemInfo> m_ItemListAll;
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
        } else if (i_totalwidth - i_baseWidth > 200) {
            m_ItemListAll[corrent].imgHeight =
                m_ItemListAll[corrent].imgHeight * (i_totalwidth - i_baseWidth) / m_ItemListAll[corrent].imgWidth;
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
        if (rowWidthList[i] < i_totalwidth) {
            int i_totalwidthExSpace = i_totalwidth - ITEM_SPACING * m_gridItem[i].size();
            int rowWidthListExSpace = rowWidthList[i] - ITEM_SPACING * m_gridItem[i].size();
            int rowWidth = 0;
            for (int j = 0; j < m_gridItem[i].size(); j++) {
                m_gridItem[i][j].width =
                    m_gridItem[i][j].width * i_totalwidthExSpace / rowWidthListExSpace;
                m_gridItem[i][j].height =
                    m_gridItem[i][j].height * i_totalwidthExSpace / rowWidthListExSpace;
                m_gridItem[i][j].imgWidth =
                    m_gridItem[i][j].imgWidth * i_totalwidthExSpace / rowWidthListExSpace;
                m_gridItem[i][j].imgHeight =
                    m_gridItem[i][j].imgHeight * i_totalwidthExSpace / rowWidthListExSpace;

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

            item->setData(QVariant(datas), Qt::DisplayRole);
            item->setData(QVariant(QSize(gridItem[i][j].width, /*m_gridItem[i][j].height*/height)),
                          Qt::SizeHintRole);
            m_model->appendRow(item);
        }
    }
    m_gridItem << gridItem;

    if (0 < m_gridItem.size()) {
        m_height = 0;
        for (int i = 0; i < m_gridItem.size(); i++) {
            m_height = m_height + m_gridItem[i][0].height + ITEM_SPACING;
        }
        m_height -= ITEM_SPACING;
    }
}

void ThumbnailListView::addThumbnailView()
{
    QModelIndexList mlist = getSelectedIndexes();
    struct Listolditem {
        int row;
        int column;
    };
    QList<Listolditem> items;
    for (QModelIndex i : mlist) {
        Listolditem item;
        item.row = i.row();
        item.column = i.column();
        items.append(item);
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

            item->setData(QVariant(datas), Qt::DisplayRole);
            item->setData(QVariant(QSize(m_gridItem[i][j].width, /*m_gridItem[i][j].height*/height)),
                          Qt::SizeHintRole);
            m_model->appendRow(item);
        }
    }

    //设置更新之前的选择状态
    for (Listolditem i : items) {
        if (i.row < m_model->rowCount()
                && i.column < m_model->columnCount()) {
            QModelIndex qindex = m_model->index(i.row, i.column);
            selectionModel()->select(qindex, QItemSelectionModel::Select);
        }
    }
}

void ThumbnailListView::updateThumbnailView()
{
    int index = 0;
    for (int i = 0; i < m_gridItem.length(); i++) {
        for (int j = 0; j < m_gridItem[i].length(); j++) {

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

            m_model->item(index, 0)->setData(QVariant(newdatas), Qt::DisplayRole);
            m_model->item(index, 0)->setData(QVariant(QSize(m_gridItem[i][j].width, /*m_gridItem[i][j].height*/height)),
                                             Qt::SizeHintRole);
            QSize picSize(m_gridItem[i][j].width, /*m_gridItem[i][j].height*/height);
            m_model->item(index, 0)->setSizeHint(picSize);
            index++;
        }
    }
}

void ThumbnailListView::loadFilesFromDB(QString name)
{
    ImageEngineApi::instance()->loadImagesFromDB(m_delegatetype, this, name);
//    switch (m_delegatetype) {
//    case ThumbnailDelegate::AllPicViewType:
//        ImageEngineApi::instance()->importImagesFromDB(DelegateType, this);
//        break;
//    case ThumbnailDelegate::SearchViewType:
//        ImageEngineApi::instance()->importImagesFromDB(DBAllPaths, this);
//        break;
//    default:
//        return;
//    }
}

bool ThumbnailListView::imageFromDBLoaded(QStringList &filelist)
{
//    qDebug() << "threadID : " << QThread::currentThreadId();
    m_allfileslist << filelist;
    m_filesbeleft << filelist;
    m_allNeedRequestFilesCount += filelist.size();
    calWidgetItemWandH();
    addThumbnailView();
    if (bneedloadimage) {
        requestSomeImages();
    }
    return true;
}

void ThumbnailListView::loadFilesFromLocal(QStringList files, bool needcache, bool needcheck)
{
    ImageEngineApi::instance()->loadImagesFromLocal(files, this);
    bneedcache = needcache;
}

void ThumbnailListView::loadFilesFromLocal(DBImgInfoList files, bool needcache, bool needcheck)
{
    ImageEngineApi::instance()->loadImagesFromLocal(files, this);
    bneedcache = needcache;
}

void ThumbnailListView::loadFilesFromTrash(DBImgInfoList files)
{
    ImageEngineApi::instance()->loadImagesFromTrash(files, this);
}

bool ThumbnailListView::imageLocalLoaded(QStringList &filelist)
{
//    qDebug() << "threadID : " << QThread::currentThreadId();
    stopLoadAndClear();
    m_allfileslist << filelist;
    m_filesbeleft << filelist;
    m_allNeedRequestFilesCount += filelist.size();
    calWidgetItemWandH();
    addThumbnailView();
    if (bneedloadimage) {
        requestSomeImages();
    }
    return true;
}

void ThumbnailListView::requestSomeImages()
{
//    QMutexLocker mutex(&m_mutex);
    bneedloadimage = false;

    if (m_filesbeleft.size() < Number_Of_Displays_Per_Time) {
        m_requestCount += m_filesbeleft.size();
    } else {
        m_requestCount += Number_Of_Displays_Per_Time;
    }
//    m_requestCount = 0;
    for (int i = 0; i < Number_Of_Displays_Per_Time; i++) {
        if (m_filesbeleft.size() <= 1) {
            brequestallfiles = true;
        }
        if (m_filesbeleft.size() <= 0) {
//            brequestallfiles = true;
            return;
        }
//        m_requestCount++;
        QString firstfilesbeleft = m_filesbeleft.first();
        m_filesbeleft.removeFirst();
        ImageEngineApi::instance()->reQuestImageData(firstfilesbeleft, this, bneedcache);
    }
//    m_requestCount = Number_Of_Displays_Per_Time;
}

bool ThumbnailListView::imageLoaded(QString filepath)
{
//    qDebug() << "threadID : " << QThread::currentThreadId();
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
        info.name = data.dbi.fileName;
        info.path = data.dbi.filePath;
        info.width = data.imgpixmap.width();
        info.height = data.imgpixmap.height();
        info.image = data.imgpixmap;
        info.remainDays = data.remainDays;
        info.baseWidth = data.imgpixmap.width();
        info.baseHeight = data.imgpixmap.height();
        insertThumbnail(info);
//        return false;
        reb = false;
    }
    if (m_requestCount < 1
//            && (bneedloadimage || !verticalScrollBar()->isVisible())  //注释以后，一次性全部加载
       ) {
        requestSomeImages();
    }
//    return true;
    return reb;
}

void ThumbnailListView::insertThumbnail(const ItemInfo &iteminfo)
{
    ItemInfo info = iteminfo;
    calBasePixMap(info);
    m_ItemListLeft << info;
//    m_ItemList << info;
//    if (0 != m_iDefaultWidth) {
    calWidgetItem();
    emit needResize(m_height + 15);

    if (nullptr != m_item) {
        m_item->setSizeHint(QSize(this->width(), m_height + 27 + 8)/*this->size()*/);
        this->setFixedSize(QSize(this->width(), m_height + 27 + 8)/*this->size()*/);
    }
//}
}

void ThumbnailListView::stopLoadAndClear()
{

//    qDebug() << "threadID : " << QThread::currentThreadId();
    clearAndStopThread();

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


void ThumbnailListView::setListWidgetItem(QListWidgetItem *item)
{
    m_item = item;
}

void ThumbnailListView::setVScrollbarDistance(int topdistance, int bottomdistance)
{
    m_scrollbartopdistance = topdistance;
    m_scrollbarbottomdistance = bottomdistance;
}

//void ThumbnailListView::insertThumbnails(const QList<ItemInfo> &itemList)
//{
//    m_delegate->m_imageTypeStr = m_imageType;
//    m_ItemList = itemList;

//    for (int i = 0; i < m_ItemList.length(); i++) {
//        QImage tImg;

//        //        m_ItemList[i].width = m_ItemList[i].image.width();
//        //        m_ItemList[i].height = m_ItemList[i].image.height();
//        m_ItemList[i].baseWidth = m_ItemList[i].width;
//        m_ItemList[i].baseHeight = m_ItemList[i].height;
//    }

//    if (0 != m_iDefaultWidth) {
//        calBasePixMapWandH();
//        calWidgetItemWandH();
//        addThumbnailView();
//    }
//}

void ThumbnailListView::onShowMenu(const QPoint &pos)
{
    //外接设备显示照片时，禁用鼠标右键菜单
    if (!this->indexAt(pos).isValid() || ALBUM_PATHTYPE_BY_PHONE == m_imageType) {
        return;
    }

    updateMenuContents();
    m_pMenu->popup(QCursor::pos());
    emit sigMouseRelease();
}

void ThumbnailListView::updateMenuContents()
{
    if (m_imageType.compare(COMMON_STR_TRASH) == 0) {
        return;
    }
    QStringList paths;
    if (m_imageType == COMMON_STR_VIEW_TIMELINE || m_imageType == COMMON_STR_RECENT_IMPORTED) {
        emit sigGetSelectedPaths(&paths);
    } else {
        paths = selectedPaths();
    }
    paths.removeAll(QString(""));

    foreach (QAction *action, m_MenuActionMap.values()) {
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
        if (DBManager::instance()->isImgExistInAlbum(COMMON_STR_FAVORITES, paths[0])) {
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
        m_MenuActionMap.value(tr("Export"))->setEnabled(true);
    } else {
        bool ret = true;
        QString strSuffix = QFileInfo(paths.at(0)).completeSuffix();
        if (strSuffix.compare("jpeg") && strSuffix.compare("jpg") && strSuffix.compare("bmp") &&
                strSuffix.compare("png") && strSuffix.compare("ppm") && strSuffix.compare("xbm") &&
                strSuffix.compare("xpm") && strSuffix.compare("gif") && strSuffix.compare("JPEG") &&
                strSuffix.compare("JPG") && strSuffix.compare("BMP") && strSuffix.compare("PNG") &&
                strSuffix.compare("PPM") && strSuffix.compare("XBM") && strSuffix.compare("XPM") &&
                strSuffix.compare("GIF")) {
            ret = false;
        }

        m_MenuActionMap.value(tr("Export"))->setEnabled(ret);
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

        if (DBManager::instance()->isImgExistInAlbum(COMMON_STR_FAVORITES, paths[0])) {
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
        m_MenuActionMap.value(tr("Favorite"))->setVisible(false);
        m_MenuActionMap.value(tr("Unfavorite"))->setVisible(false);
    }

    int flag_imageSupportSave = 0;
    for (auto path : paths) {
        if (!utils::image::imageSupportSave(path)) {
            flag_imageSupportSave = 1;
            break;
        }
    }

    if (0 == flag_imageSupportSave) {
        int flag_isRW = 0;
        for (auto path : paths) {
            if (QFileInfo(path).isReadable() && !QFileInfo(path).isWritable()) {
                flag_isRW = 1;
                break;
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
    if (!(1 == paths.length() && utils::image::imageSupportSave(paths[0]))) {
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
        appendAction(IdImageInfo, tr("Photo info"), ss(ImageInfo_CONTEXT_MENU));
        appendAction(IdMoveToTrash, tr("Delete"), ss(""));
        appendAction(IdTrashRecovery, tr("Restore"), ss(BUTTON_RECOVERY));
        return;
    }
    m_MenuActionMap.clear();

    appendAction(IdView, tr("View"), ss(VIEW_CONTEXT_MENU));
    appendAction(IdFullScreen, tr("Fullscreen"), ss(FULLSCREEN_CONTEXT_MENU));
    appendAction(Idprint, tr("Print"), ss(PRINT_CONTEXT_MENU));
    appendAction(IdStartSlideShow, tr("Slide show"), ss(SLIDESHOW_CONTEXT_MENU));

    m_pMenu->addSeparator();
    appendAction(IdExport, tr("Export"), ss(EXPORT_CONTEXT_MENU));
    appendAction(IdCopyToClipboard, tr("Copy"), ss(COPYTOCLIPBOARD_CONTEXT_MENU));
    appendAction(IdMoveToTrash, tr("Delete"), ss(""));
    appendAction(IdRemoveFromAlbum, tr("Remove from album"), ss(""));
    m_pMenu->addSeparator();
    appendAction(IdRemoveFromFavorites, tr("Unfavorite"), ss(UNFAVORITE_CONTEXT_MENU));
    appendAction(IdAddToFavorites, tr("Favorite"), ss(FAVORITE_CONTEXT_MENU));
    m_pMenu->addSeparator();
    appendAction(IdRotateClockwise, tr("Rotate clockwise"), ss(ROTATECLOCKWISE_CONTEXT_MENU));
    appendAction(IdRotateCounterclockwise, tr("Rotate counterclockwise"),
                 ss(ROTATECOUNTERCLOCKWISE_CONTEXT_MENU));
    m_pMenu->addSeparator();
    appendAction(IdSetAsWallpaper, tr("Set as wallpaper"), ss(SETASWALLPAPER_CONTEXT_MENU));
    appendAction(IdDisplayInFileManager, tr("Display in file manager"),
                 ss(DISPLAYINFILEMANAGER_CONTEXT_MENU));
    appendAction(IdImageInfo, tr("Photo info"), ss(ImageInfo_CONTEXT_MENU));
}

DMenu *ThumbnailListView::createAlbumMenu()
{
    DMenu *am = new DMenu(tr("Add to album"));

    QStringList albums = DBManager::instance()->getAllAlbumNames();
    albums.removeAll(COMMON_STR_FAVORITES);
    albums.removeAll(COMMON_STR_TRASH);
    albums.removeAll(COMMON_STR_RECENT_IMPORTED);

    QAction *ac = new QAction(am);
    ac->setProperty("MenuID", IdAddToAlbum);
    ac->setText(tr("New album"));
    ac->setData("Add to new album");
    am->addAction(ac);
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

//QList<ThumbnailListView::ItemInfo> ThumbnailListView::getAllPaths()
QStringList ThumbnailListView::getAllPaths()
{
//    return m_ItemList;
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
    //    const QStringList viewPaths = (paths.length() == 1) ? albumPaths() : paths;
    const QString path = paths.first();
    const int id = action->property("MenuID").toInt();
    switch (MenuItemId(id)) {
    case IdView:
        emit menuOpenImage(path, paths, false, false);
        break;
    case IdFullScreen:
        emit menuOpenImage(path, paths, true, false);
        break;
    case Idprint:
        PrintHelper::showPrintDialog(QStringList(path), this);
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
        } else {
            emit dApp->signalM->createAlbum(paths);
        }

        break;
    }
    case IdCopyToClipboard:
        utils::base::copyImageToClipboard(paths);
        break;
    case IdMoveToTrash: {
        if (COMMON_STR_TRASH == m_imageType) {
            ImgDeleteDialog *dialog = new ImgDeleteDialog(this, paths.length());
            dialog->show();
            connect(dialog, &ImgDeleteDialog::imgdelete, this, [ = ] {
                //                for (auto path : paths)
                //                {
                //                    dApp->m_imagetrashmap.remove(path);
                //                }
                emit dApp->signalM->sigDeletePhotos(paths.length());
                DBManager::instance()->removeTrashImgInfos(paths);
                emit trashDelete();
            });
        }

        else {
            ImageEngineApi::instance()->moveImagesToTrash(paths);
        }
    }
    break;
    case IdAddToFavorites:
        DBManager::instance()->insertIntoAlbum(COMMON_STR_FAVORITES, paths);
        break;
    case IdRemoveFromFavorites:
        DBManager::instance()->removeFromAlbum(COMMON_STR_FAVORITES, paths);
        break;
    case IdRemoveFromAlbum: {
        if (IMAGE_DEFAULTTYPE != m_imageType && COMMON_STR_VIEW_TIMELINE != m_imageType &&
                COMMON_STR_RECENT_IMPORTED != m_imageType && COMMON_STR_TRASH != m_imageType) {
            DBManager::instance()->removeFromAlbum(m_imageType, paths);
        }
    }
    break;
    case IdRotateClockwise: {
        QModelIndexList mlist = selectedIndexes();
        QModelIndexList::iterator i;
        QList<Listolditem> items;
        for (i = mlist.begin(); i != mlist.end(); ++i) {
            Listolditem item;
            item.row = (*i).row();
            item.column = (*i).column();
            items.append(item);
        }
        for (QString path : paths) {
            utils::image::rotate(path, 90);
        }

//        if (COMMON_STR_TRASH == m_imageType) {
//            dApp->m_imageloader->updateTrashImageLoader(paths);
//        } else {
        dApp->m_imageloader->updateImageLoader(paths);
//    }
        QList<Listolditem>::iterator j;
        for (j = items.begin(); j != items.end(); ++j) {
            if ((*j).row < m_model->rowCount() && (*j).column < m_model->columnCount()) {
                QModelIndex qindex = m_model->index((*j).row, (*j).column);
                selectionModel()->select(qindex, QItemSelectionModel::Select);
            }
        }
    }
    break;
    case IdRotateCounterclockwise: {
        QModelIndexList mlist = selectedIndexes();
        QModelIndexList::iterator i;
        struct Listolditem {
            int row;
            int column;
        };
        QList<Listolditem> items;
        for (i = mlist.begin(); i != mlist.end(); ++i) {
            Listolditem item;
            item.row = (*i).row();
            item.column = (*i).column();
            items.append(item);
        }
        for (QString path : paths) {
            utils::image::rotate(path, -90);
        }

//        if (COMMON_STR_TRASH == m_imageType) {
//            dApp->m_imageloader->updateTrashImageLoader(paths);
//        } else {
        dApp->m_imageloader->updateImageLoader(paths);
//        }
        QList<Listolditem>::iterator j;
        for (j = items.begin(); j != items.end(); ++j) {
            if ((*j).row < m_model->rowCount() && (*j).column < m_model->columnCount()) {
                QModelIndex qindex = m_model->index((*j).row, (*j).column);
                selectionModel()->select(qindex, QItemSelectionModel::Select);
            }
        }
    }
    break;
    case IdSetAsWallpaper:
        dApp->wpSetter->setWallpaper(path);
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

//    updateMenuContents();
}

void ThumbnailListView::onPixMapScale(int value)
{
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
    }

    calBasePixMapWandH();
    calWidgetItemWandH();
    addThumbnailView();

    emit SignalManager::instance()->updateThumbnailViewSize();
//    emit loadend(m_height + 15);
    emit needResize(m_height + 15);
}

//void ThumbnailListView::slotPageNeedResize(int index)
//{
//    qDebug() << index;
//    m_ItemList[index].baseWidth = dApp->m_imagemap.value(m_ItemList[index].path).width();
//    m_ItemList[index].baseHeight = dApp->m_imagemap.value(m_ItemList[index].path).height();

//    for (int i = 0; i < m_ItemList.length(); i++) {
//        m_ItemList[i].width = m_ItemList[i].baseWidth;
//        m_ItemList[i].height = m_ItemList[i].baseHeight;
//        m_ItemList[i].imgWidth = 0;
//        m_ItemList[i].imgHeight = 0;
//    }
//    calBasePixMapWandH();
////    calWidgetItemWandH();
////    updateThumbnailView();

//    setFixedSize(QSize(size().width() + 1, size().height()));
//    setFixedSize(QSize(size().width() - 1, size().height())); //触发resizeevent
//    setMinimumSize(0, 0);
//    setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));  //触发后还原状态
////    resize(QSize(size().width() + 1, size().height()));
////    resize(QSize(size().width() - 1, size().height())); //触发resizeevent
////    resize(size() - QSize(1, 1));
////    resize(size() + QSize(1, 1)); //触发resizeevent

//}

void ThumbnailListView::onCancelFavorite(const QModelIndex &index)
{
    QStringList str;
    QVariantList datas = index.model()->data(index, Qt::DisplayRole).toList();

    if (datas.length() >= 2) {
        str << datas[1].toString();
    }

    DBManager::instance()->removeFromAlbumNoSignal(COMMON_STR_FAVORITES, str);

    emit dApp->signalM->updateFavoriteNum();
    m_model->removeRow(index.row());
    m_ItemList.removeAt(index.row());
    calWidgetItemWandH();
    updateThumbnailView();
//    emit dApp->signalM->sigUpdataAlbumRightTitle(m_albumNameStr);

}

void ThumbnailListView::resizeEvent(QResizeEvent *e)
{
//    QMutexLocker mutex(&m_mutex);
//    if (COMMON_STR_RECENT_IMPORTED == m_imageType) {
//        int a = 0;
//    }
//    if (0 == m_iDefaultWidth) {
//        calBasePixMapWandH();
//        calWidgetItemWandH();
//        addThumbnailView();
//    } else {
    QScrollBar *bar = this->verticalScrollBar();
    bar->setGeometry(bar->x(), /*bar->y() + */m_scrollbartopdistance, bar->width(), this->height() - m_scrollbartopdistance - m_scrollbarbottomdistance);

    calWidgetItemWandH();
    addThumbnailView();
//        updateThumbnailView();
//}
//    emit loadend((m_height)*m_gridItem.size()+15);
//    emit loadend(m_height + 15);
    emit needResize(m_height + 15);


    m_iDefaultWidth = width();

    QListView::resizeEvent(e);
    if (nullptr != m_item) {
        m_item->setSizeHint(QSize(this->width(), getListViewHeight() + 8 + 27)/*this->size()*/);
    }
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
                qDebug() << "sigKeyEvent" << keyEvent->key();
                //处理上下翻页
                emit sigKeyEvent(keyEvent->key());

                return true;
            }
        } else {
            if (keyEvent->key() == Qt::Key_PageDown) {
                QScrollBar *vb = this->verticalScrollBar();
                int posValue = vb->value();
                posValue += this->height();
                vb->setValue(posValue);

                return true;
            } else if (keyEvent->key() == Qt::Key_PageUp) {
                QScrollBar *vb = this->verticalScrollBar();
                int posValue = vb->value();
                posValue -= this->height();
                vb->setValue(posValue);

                return true;
            }
        }
    }

    return false;
}
#if 1
QModelIndexList ThumbnailListView::getSelectedIndexes()
{
    return selectedIndexes();
}

void ThumbnailListView::selectCurrent(int row)
{
    QModelIndex qindex = m_model->index(row, 0);
    selectionModel()->select(qindex, QItemSelectionModel::Select);
}

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
//    QModelIndex qindex = m_model->index(row,0);
//    selectionModel()->select(qindex,QItemSelectionModel::Clear);
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
#endif
//add start 3975
int ThumbnailListView::getListViewHeight()
{
    return m_height;
}
//add end 3975
