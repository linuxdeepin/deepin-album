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

#include "controller/signalmanager.h"
#include "controller/wallpapersetter.h"
#include "widgets/dialogs/imgdeletedialog.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "utils/unionimage.h"
#include "imageengine/imageengineapi.h"
#include "imageengine/imageenginethread.h"
#include "ac-desktop-define.h"

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
        m_scrollbarbottomdistance = 0;
    }// else if (ThumbnailDelegate::AlbumViewType == m_delegatetype) {
//        m_scrollbarbottomdistance = 27;
//    } else if (ThumbnailDelegate::SearchViewType == m_delegatetype || ThumbnailDelegate::AlbumViewPhoneType == m_delegatetype) {
//        m_scrollbartopdistance = 134;
//        m_scrollbarbottomdistance = 27;
//    }
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
    m_delegate = new ThumbnailDelegate(type, this); //绑在this上面，用于自动销毁
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
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &ThumbnailListView::sltChangeDamagedPixOnThemeChanged);
    touchTapDistance = 15;

    updateEnableSelectionByMouseTimer = new QTimer(this);
    updateEnableSelectionByMouseTimer->setInterval(500);
    connect(updateEnableSelectionByMouseTimer, &QTimer::timeout, [ = ]() {
        if (touchStatus == 0) { //时间到了还在等待模式,则进入框选模式
            touchStatus = 2;
        }
        updateEnableSelectionByMouseTimer->stop();
    });
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

    if ((QApplication::keyboardModifiers() == Qt::ShiftModifier && event->button() == Qt::LeftButton)
            && (m_imageType == COMMON_STR_VIEW_TIMELINE || m_imageType == COMMON_STR_RECENT_IMPORTED));
    else {
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
        else {
            DListView::mousePressEvent(event);
        }
    }
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
            emit sigNeedMoveScorll(-(event->pos() - lastTouchBeginPos).y());
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
    QTimer::singleShot(100, this, SLOT(resizeEventF()));
}

void ThumbnailListView::mouseReleaseEvent(QMouseEvent *event)
{
    DListView::mouseReleaseEvent(event);

    //触屏状态复位
    touchStatus = 0;
    updateEnableSelectionByMouseTimer->stop();

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
    connect(dApp->signalM, &SignalManager::sigSyncListviewModelData, this, &ThumbnailListView::onSyncListviewModelData);
    connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, &ThumbnailListView::onScrollbarValueChanged);
    connect(this->verticalScrollBar(), &QScrollBar::rangeChanged, this, &ThumbnailListView::onScrollBarRangeChanged);
    connect(this, &QListView::customContextMenuRequested, this, &ThumbnailListView::onShowMenu);
    connect(m_pMenu, &DMenu::triggered, this, &ThumbnailListView::onMenuItemClicked);
    connect(this, &ThumbnailListView::doubleClicked, this, &ThumbnailListView::onDoubleClicked);
    connect(this, &ThumbnailListView::clicked, this, &ThumbnailListView::onClicked);
    connect(dApp->signalM, &SignalManager::sigMainwindowSliderValueChg, this, &ThumbnailListView::onPixMapScale);
    connect(m_delegate, &ThumbnailDelegate::sigCancelFavorite, this, &ThumbnailListView::onCancelFavorite);
}

void ThumbnailListView::addThumbnailViewNew(QList<QList<ItemInfo>> gridItem)
{
    for (int i = 0; i < gridItem.length(); i++) {
        for (int j = 0; j < gridItem[i].length(); j++) {
            QStandardItem *item = new QStandardItem;
            QString qsfirstorlast = "NotFirstOrLast";
            //针对第一行做处理
            int height = gridItem[i][j].imgHeight;
            if (ThumbnailDelegate::AllPicViewType == m_delegatetype) {
                if (m_model->rowCount() < m_rowSizeHint) {
                    height += 50;
                    qsfirstorlast = "First";
                }
            }

            QVariantList datas;
            datas.append(QVariant(gridItem[i][j].name));
            datas.append(QVariant(gridItem[i][j].path));
            datas.append(QVariant(gridItem[i][j].remainDays));
            datas.append(QVariant(gridItem[i][j].image));
            datas.append(QVariant(gridItem[i][j].imgWidth));
            datas.append(QVariant(gridItem[i][j].imgHeight));
            datas.append(QVariant(gridItem[i][j].baseWidth));
            datas.append(QVariant(gridItem[i][j].baseHeight));
            datas.append(QVariant(qsfirstorlast));
            datas.append(QVariant(gridItem[i][j].bNotSupportedOrDamaged));
            item->setData(QVariant(datas), Qt::DisplayRole);
            QStringList albumNames = ImageEngineApi::instance()->getImgPathAndAlbumNames().values(gridItem[i][j].path);
            item->setData(QVariant(albumNames), Qt::UserRole + 2);

            item->setData(QVariant(QSize(gridItem[i][j].imgWidth, /*gridItem[i][j].height*/height)),
                          Qt::SizeHintRole);
            m_model->appendRow(item);
        }
    }
    m_gridItem << gridItem;
    int hightlast = m_height;
    if (0 < m_gridItem.size()) {
        m_height = 0;
        int index = 0;//当前一个list所有照片数量
        for (int i = 0; i < m_gridItem.size(); i++) {
            for (int j = 0; j < m_gridItem.at(i).size(); j++) {
                index ++;
            }
        }
        int m_row = 0;//当前一个list的行数
        if (index % m_rowSizeHint == 0)
            m_row = index / m_rowSizeHint;
        else
            m_row = index / m_rowSizeHint + 1;

        m_height = (m_gridItem.at(0).at(0).imgHeight + ITEM_SPACING) * m_row;
        m_height -= ITEM_SPACING;
    }
    if (hightlast != m_height) {
        sendNeedResize();
    }

    if (m_selectPrePath.length() > 0) {
        this->clearSelection();
        QModelIndex lastIndex;
        int rowcount = m_model->rowCount();
        for (int i = 0; i < rowcount; i++) {
            for (int j = 0 ; j < m_model->columnCount(); j++) {
                QVariantList datas = m_model->item(i, j)->data(Qt::DisplayRole).toList();
                QString path;
                if (datas.length() >= 2) {
                    path = datas[1].toString();
                }
                if (m_selectPrePath == path) {
                    if (this->objectName() == "RightTrashThumbnail" || this->objectName() == "RightFavoriteThumbnail") {
                        if (i >= m_rowSizeHint) {
                            lastIndex = m_model->item(i - m_rowSizeHint, j)->index();
                        } else {
                            lastIndex = m_model->item(i, j)->index();
                        }
                    } else {
                        lastIndex = m_model->item(i, j)->index();
                    }
                    if (lastIndex.isValid()) {
                        m_Row = m_model->item(i, j)->row() / m_model->rowCount();
                        this->scrollTo(lastIndex, ScrollHint::PositionAtCenter);
                        break;
                    }
                }
            }
        }
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
        for (int j = 0; j < m_gridItem.at(i).length(); j++) {
            QStandardItem *item = new QStandardItem;
            QString qsfirstorlast = "NotFirstOrLast";
            int height = m_gridItem.at(i).at(j).imgHeight;
            //针对第一行做处理
            if (ThumbnailDelegate::AllPicViewType == m_delegatetype) {
                if (m_model->rowCount() < m_rowSizeHint) {
                    height += 50;
                    qsfirstorlast = "First";
                } else { //刷新，只取第一行添加
                    if (i < m_rowSizeHint) {
                        height += 50;
                        qsfirstorlast = "First";
                    }
                }
            }

            QVariantList datas;
            datas.append(QVariant(m_gridItem.at(i).at(j).name));
            datas.append(QVariant(m_gridItem.at(i).at(j).path));
            datas.append(QVariant(m_gridItem.at(i).at(j).remainDays));
            datas.append(QVariant(m_gridItem.at(i).at(j).image));
            datas.append(QVariant(m_gridItem.at(i).at(j).imgWidth));
            datas.append(QVariant(m_gridItem.at(i).at(j).imgHeight));
            datas.append(QVariant(m_gridItem.at(i).at(j).baseWidth));
            datas.append(QVariant(m_gridItem.at(i).at(j).baseHeight));
            datas.append(QVariant(qsfirstorlast));
            datas.append(QVariant(m_gridItem.at(i).at(j).bNotSupportedOrDamaged));
            item->setData(QVariant(datas), Qt::DisplayRole);
            item->setData(QVariant(QSize(m_gridItem.at(i).at(j).imgHeight, height)),
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
        for (int j = 0; j < m_gridItem.at(i).length(); j++) {
            if (m_gridItem.at(i).at(j).path == updatePath) {  //需要旋转的图片
                ImageDataSt data;
                ImageEngineApi::instance()->getImageData(updatePath, data);
                ItemInfo info;
                if (data.imgpixmap.isNull()) {
                    info.bNotSupportedOrDamaged = true;
                    data.imgpixmap = getDamagedPixmap();
                }
                info.name = data.dbi.fileName;
                info.path = data.dbi.filePath;
                info.image = data.imgpixmap;
                info.remainDays = data.remainDays;
                info.baseWidth = data.imgpixmap.width();
                info.baseHeight = data.imgpixmap.height();
                modifyAllPic(info);
                for (auto &item : m_ItemList) {     //替换
                    if (item == m_gridItem.at(i).at(j))
                        item = info;
                }
                QList<ItemInfo>::iterator item = m_ItemList.begin();
                while (item != m_ItemList.end()) {
                    if (*item == m_gridItem.at(i).at(j))
                        *item = info;
                    ++item;
                }
                m_gridItem[i][j] = info;
                QVariantList newdatas;
                QString qsfirstorlast = "NotFirstOrLast";
                int height = m_gridItem.at(i).at(j).imgHeight;
                //针对第一行做处理
                if (ThumbnailDelegate::AllPicViewType == m_delegatetype) {
                    if (m_model->rowCount() < m_rowSizeHint) {
                        height += 50;
                        qsfirstorlast = "First";
                    } else { //刷新，只取第一行添加
                        if (i < m_rowSizeHint) {
                            height += 50;
                            qsfirstorlast = "First";
                        }
                    }
                }

                newdatas.append(QVariant(m_gridItem.at(i).at(j).name));
                newdatas.append(QVariant(m_gridItem.at(i).at(j).path));
                newdatas.append(QVariant(m_gridItem.at(i).at(j).remainDays));
                newdatas.append(QVariant(m_gridItem.at(i).at(j).image));
                newdatas.append(QVariant(m_gridItem.at(i).at(j).imgWidth));
                newdatas.append(QVariant(m_gridItem.at(i).at(j).imgHeight));
                newdatas.append(QVariant(m_gridItem.at(i).at(j).baseWidth));
                newdatas.append(QVariant(m_gridItem.at(i).at(j).baseHeight));
                newdatas.append(QVariant(qsfirstorlast));
                newdatas.append(QVariant(m_gridItem.at(i).at(j).bNotSupportedOrDamaged));
                m_model->item(index, 0)->setData(QVariant(newdatas), Qt::DisplayRole);
                m_model->item(index, 0)->setData(QVariant(QSize(m_gridItem.at(i).at(j).imgWidth, height)),
                                                 Qt::SizeHintRole);
                QStringList albumNames = ImageEngineApi::instance()->getImgPathAndAlbumNames().values(m_gridItem.at(i).at(j).path);
                m_model->item(index, 0)->setData(QVariant(albumNames), Qt::UserRole + 2);
            }
            index++;
        }
    }
    //更新布局
    calBasePixMapWidth();
    calgridItemsWidth();
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
    calgridItemsWidth();
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
    calgridItemsWidth();
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
//            blastload = true;
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
        info.image = data.imgpixmap;
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
    cutPixmap(info);
    modifyAllPic(info);
    m_allItemLeft << info; //所有待处理的图片
    calgridItems();
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
}

QStringList ThumbnailListView::getAllFileList()
{
    return m_allfileslist;
}

void ThumbnailListView::setIBaseHeight(int iBaseHeight)
{
    m_iBaseHeight = iBaseHeight;
}

void ThumbnailListView::onShowMenu(const QPoint &pos)
{
    //外接设备显示照片时，禁用鼠标右键菜单
    if (!this->indexAt(pos).isValid() || ALBUM_PATHTYPE_BY_PHONE == m_imageType) {
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
    if ((1 == paths.length()) && (!QFileInfo(paths[0]).exists()) && (COMMON_STR_TRASH != m_imageType)) {
        //源文件不存在时的菜单
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
#ifndef tablet_PC
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
    bool first = true;
    for (QModelIndex index : selectionModel()->selectedIndexes()) {
        const QVariantList datas = index.model()->data(index, Qt::DisplayRole).toList();
        if (datas.length() >= 8) {
            paths << datas[1].toString();
        }
        if (first) {
            m_timeLineSelectPrePic = index.row() - 1;
            if (m_timeLineSelectPrePic < 0)
                m_timeLineSelectPrePic = 0;
        }
    }
    m_currentDeletePath = paths;
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
        PrintHelper::getIntance()->showPrintDialog(paths, this);
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
        ImgDeleteDialog *dialog = new ImgDeleteDialog(this, paths.length());
        dialog->setObjectName("deteledialog");
        if (dialog->exec() > 0) {
            // 只更新部分，删除
            updateModelRoleData("", IdMoveToTrash);
            if (COMMON_STR_VIEW_TIMELINE == m_imageType || COMMON_STR_RECENT_IMPORTED == m_imageType) {
                emit sigMoveToTrash();
            }
            (COMMON_STR_TRASH == m_imageType) ? ImageEngineApi::instance()->moveImagesToTrash(paths, true, false) : ImageEngineApi::instance()->moveImagesToTrash(paths);
        }
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
    calBasePixMapWidth();
    calgridItemsWidth();
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
    calgridItemsWidth();
    updateThumbnailView();
    sendNeedResize();
}

void ThumbnailListView::resizeEvent(QResizeEvent *e)
{
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
        for (int j = 0; j < m_gridItem.at(i).length(); j++) {
            QString qsfirstorlast = "NotFirstOrLast";
            int height = m_gridItem.at(i).at(j).imgHeight;
            //针对第一行做处理
            if (ThumbnailDelegate::AllPicViewType == m_delegatetype) {
                if (m_model->rowCount() < m_rowSizeHint) {
                    height += 50;
                    qsfirstorlast = "First";
                } else { //刷新，只取第一行添加
                    if (i < m_rowSizeHint) {
                        height += 50;
                        qsfirstorlast = "First";
                    }
                }
            }

            QVariantList datas;
            datas.append(QVariant(m_gridItem.at(i).at(j).name));
            datas.append(QVariant(m_gridItem.at(i).at(j).path));
            datas.append(QVariant(m_gridItem.at(i).at(j).remainDays));
            if (m_gridItem.at(i).at(j).bNotSupportedOrDamaged) {
                m_gridItem[i][j].image = getDamagedPixmap();
            }
            datas.append(QVariant(m_gridItem.at(i).at(j).image));
            datas.append(QVariant(m_gridItem.at(i).at(j).imgWidth));
            datas.append(QVariant(m_gridItem.at(i).at(j).imgHeight));
            datas.append(QVariant(m_gridItem.at(i).at(j).baseWidth));
            datas.append(QVariant(m_gridItem.at(i).at(j).baseHeight));
            datas.append(QVariant(qsfirstorlast));
            datas.append(QVariant(m_gridItem.at(i).at(j).bNotSupportedOrDamaged));

            //更新值
            QStandardItem *newItem = m_model->item(index);
            if (newItem) {
                newItem->setData(QVariant(datas), Qt::DisplayRole);
                newItem->setData(QVariant(QSize(m_gridItem.at(i).at(j).imgWidth, height)),
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
                for (int j = 0; j < paths.count(); j++) {
                    if (lst.at(1).toString() == paths.at(j)) {
                        // 选中
                        selectionModel()->select(idx, QItemSelectionModel::Select);
                        if (!firstIndex.isValid()) {
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
            if (firstIndex.isValid())
            {
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

void ThumbnailListView::updateModelRoleData(QString albumName, int actionType)
{
    // listview存在多个，状态model不同，所以先处理已知的，再通知其他model
    // 本listview对象不再处理其他listviw对象model的同步问题！
    if (actionType == IdRemoveFromAlbum) {
        // remove from album
        for (QModelIndex index : selectionModel()->selectedIndexes()) {
            QStringList datas = index.model()->data(index, Qt::UserRole + 2).toStringList();
            datas.removeAll(albumName);
            QMap<int, QVariant> tempData;
            tempData.insert(Qt::UserRole + 2, datas);
            m_model->setItemData(index, tempData);
        }
    } else if (actionType == IdAddToAlbum) {
        // add to album
        for (QModelIndex index : selectionModel()->selectedIndexes()) {
            QStringList datas = index.model()->data(index, Qt::UserRole + 2).toStringList();
            datas.append(albumName);
            QMap<int, QVariant> tempData;
            tempData.insert(Qt::UserRole + 2, datas);
            m_model->setItemData(index, tempData);
        }
    } else if (actionType == IdMoveToTrash) {
        // delete photos
        for (QModelIndex index : selectionModel()->selectedIndexes()) {
            QStringList datas;
            datas.clear();
            QMap<int, QVariant> tempData;
            tempData.insert(Qt::UserRole + 2, datas);
            m_model->setItemData(index, tempData);
        }
    }
    setCurrentSelectPath();
    QStringList paths;
    paths.clear();
    for (QModelIndex index : selectionModel()->selectedIndexes()) {
        paths.append(index.model()->data(index, Qt::DisplayRole).toList().at(1).toString());
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

void ThumbnailListView::onSyncListviewModelData(QStringList paths, QString albumName, int actionType)
{
    if (paths.count() < 1)
        return ;
    if (sender() != this) {
        // listview存在多个，状态model不同，所以先处理已知的，再通知其他model
        // 本listview对象不再处理其他listviw对象model的同步问题！
        if (actionType == IdRemoveFromAlbum) {
            // remove from album
            if (paths.count() == 1 && !this->getAllPaths().contains(paths.at(0)))
                return;
            for (int i = 0; i < m_model->rowCount(); i++) {
                QModelIndex idx = m_model->index(i, 0);
                QVariantList lst = idx.model()->data(idx, Qt::DisplayRole).toList();
                if (lst.count() >= 12) {
                    for (int j = 0; j < paths.count(); j++) {
                        if (lst.at(1).toString() == paths.at(j)) {
                            QStringList datas = idx.model()->data(idx, Qt::UserRole + 2).toStringList();
                            datas.removeAll(albumName);
                            QMap<int, QVariant> tempData;
                            tempData.insert(Qt::UserRole + 2, datas);
                            m_model->setItemData(idx, tempData);
                        }
                    }
                }
            }
        } else if (actionType == IdAddToAlbum) {
            // add to album
            if (paths.count() == 1 && !this->getAllPaths().contains(paths.at(0)))
                return;
            for (int i = 0; i < m_model->rowCount(); i++) {
                QModelIndex idx = m_model->index(i, 0);
                QVariantList lst = idx.model()->data(idx, Qt::DisplayRole).toList();
                if (lst.count() >= 12) {
                    for (int j = 0; j < paths.count(); j++) {
                        if (lst.at(1).toString() == paths.at(j)) {
                            QStringList datas = idx.model()->data(idx, Qt::UserRole + 2).toStringList();
                            datas.append(albumName);
                            QMap<int, QVariant> tempData;
                            tempData.insert(Qt::UserRole + 2, datas);
                            m_model->setItemData(idx, tempData);
                        }
                    }
                }
            }
        } else if (actionType == IdMoveToTrash) {
            // delete photos
            if (paths.count() == 1 && !this->getAllPaths().contains(paths.at(0)))
                return;
            for (int i = 0; i < m_model->rowCount(); i++) {
                QModelIndex idx = m_model->index(i, 0);
                QVariantList lst = idx.model()->data(idx, Qt::DisplayRole).toList();
                if (lst.count() >= 12) {
                    for (int j = 0; j < paths.count(); j++) {
                        if (lst.at(1).toString() == paths.at(j)) {
                            QStringList datas;
                            datas.clear();
                            QMap<int, QVariant> tempData;
                            tempData.insert(Qt::UserRole + 2, datas);
                            m_model->setItemData(idx, tempData);
                        }
                    }
                }
            }
        }
    }
}

void ThumbnailListView::onScrollbarValueChanged(int value)
{
    if (value && value >= (this->verticalScrollBar()->maximum())) {
        if (m_requestCount > 0) {
            bneedloadimage = true;
        } else {
            requestSomeImages();
        }
    }
}

void ThumbnailListView::onScrollBarRangeChanged(int min, int max)
{
    Q_UNUSED(max);
    Q_UNUSED(min);
    if (nullptr == m_item) {
        QScrollBar *bar = this->verticalScrollBar();
        bar->setGeometry(bar->x(), /*bar->y() + */m_scrollbartopdistance, bar->width(), this->height() - m_scrollbartopdistance - m_scrollbarbottomdistance);
    }
}

void ThumbnailListView::onDoubleClicked(const QModelIndex &index)
{
    if (m_isSelectAllBtn) {
        return;
    }
    if (ALBUM_PATHTYPE_BY_PHONE != m_imageType) {
        if (m_imageType.compare(COMMON_STR_TRASH) != 0) {
            emit openImage(index.row());
        }
    }
}

void ThumbnailListView::onClicked(const QModelIndex &index)
{
    emit hideExtensionPanel();
    if (m_isSelectAllBtn) {
        return;
    }
#ifdef tablet_PC
    if (activeClick && ALBUM_PATHTYPE_BY_PHONE != m_imageType) {
        if (m_imageType.compare(COMMON_STR_TRASH) != 0) {
            emit openImage(index.row());
        }
    }
#endif
    Q_UNUSED(index)
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

//调整图片尺寸
void ThumbnailListView::modifyAllPic(ThumbnailListView::ItemInfo &info)
{
    int i_totalwidth = width() - 30;

    info.imgWidth = m_iBaseHeight;
    info.imgHeight = m_iBaseHeight;
    if (info.imgWidth > i_totalwidth) {
        info.imgHeight = i_totalwidth / 4;
        info.imgWidth = i_totalwidth / 4;
    }
    // 异常判断
    info.imgHeight = (1 > info.imgHeight) ? 1 : info.imgHeight;
    info.imgWidth = (1 > info.imgWidth) ? 1 : info.imgWidth;
    info.imgHeight = (m_iBaseHeight > info.imgHeight) ? m_iBaseHeight : info.imgHeight;
    info.imgWidth = (28 > info.imgWidth) ? 28 : info.imgWidth;
}

//裁剪图片尺寸
void ThumbnailListView::cutPixmap(ThumbnailListView::ItemInfo &iteminfo)
{
    int width = iteminfo.image.width();
    if (width == 0)
        width = m_iBaseHeight;
    int height = iteminfo.image.height();
    if (abs((width - height) * 10 / width) >= 1) {
        QRect rect = iteminfo.image.rect();
        int x = rect.x() + width / 2;
        int y = rect.y() + height / 2;
        if (width > height) {
            x = x - height / 2;
            y = 0;
            iteminfo.image = iteminfo.image.copy(x, y, height, height);
        } else {
            y = y - width / 2;
            x = 0;
            iteminfo.image = iteminfo.image.copy(x, y, width, width);
        }
    }
}

void ThumbnailListView::calgridItems()
{
    int i_totalwidth = width() - 30;
    m_rowSizeHint = i_totalwidth / (m_iBaseHeight + ITEM_SPACING);//计算一行能放几个
    int currentwidth = (i_totalwidth - ITEM_SPACING * (m_rowSizeHint - 1)) / m_rowSizeHint;//一张图的宽度
    m_onePicWidth = currentwidth;
    if (currentwidth < 1)
        return;

    QList<QList<ItemInfo>> gridItem;//所有
    QList<ItemInfo> rowList;//一行
    gridItem.clear();
    rowList.clear();
    int index = 0;
    int totlelength = m_allItemLeft.size();//待处理
    for (int i = 0; i < totlelength; i++) {
        m_allItemLeft[i].imgHeight = currentwidth;
        m_allItemLeft[i].imgWidth = currentwidth;
        if (index < m_rowSizeHint) {
            rowList << m_allItemLeft[i];
            index ++;
        } else {
            index = 1;
            gridItem << rowList;
            rowList.clear();
            rowList << m_allItemLeft[i];
        }
    }
    m_allItemLeft.clear();
    if (rowList.size() > 0) {
        gridItem << rowList;
        rowList.clear();
    }
    addThumbnailViewNew(gridItem);
}

void ThumbnailListView::calBasePixMapWidth()
{
    int i_totalwidth = width() - 30;
    for (int i = 0; i < m_ItemList.length(); i++) {
        m_ItemList[i].imgWidth = m_iBaseHeight;
        m_ItemList[i].imgHeight = m_iBaseHeight;
        if (m_ItemList[i].imgWidth > i_totalwidth) {
            m_ItemList[i].imgHeight = i_totalwidth / 4;
            m_ItemList[i].imgWidth = i_totalwidth / 4;
        }
    }
}

void ThumbnailListView::calgridItemsWidth()
{
    int i_totalwidth = width() - 30;
    QList<ItemInfo> oneRowList;
    //计算一行的个数
    m_rowSizeHint = i_totalwidth / (m_iBaseHeight + ITEM_SPACING);
    int currentwidth = (i_totalwidth - ITEM_SPACING * (m_rowSizeHint - 1)) / m_rowSizeHint;//一张图的宽度
    m_onePicWidth = currentwidth;
    if (currentwidth < 80)
        currentwidth = 80;

    for (int i = 0; i < m_gridItem.length(); i++) {
        for (int j = 0 ; j < m_gridItem.at(i).length(); j++) {
            m_gridItem[i][j].imgHeight = currentwidth;
            m_gridItem[i][j].imgWidth = currentwidth;
        }
    }
    if (0 < m_gridItem.size()) {
        m_height = 0;
        int index = 0;//当前一个list所有照片数量
        for (int i = 0; i < m_gridItem.size(); i++) {
            for (int j = 0; j < m_gridItem.at(i).size(); j++) {
                index ++;
            }
        }
        int m_row = 0;//当前一个list的行数
        if (index % m_rowSizeHint == 0)
            m_row = index / m_rowSizeHint;
        else
            m_row = index / m_rowSizeHint + 1;

        m_height = (m_gridItem.at(0).at(0).imgHeight + ITEM_SPACING) * m_row;
        m_height -= ITEM_SPACING;
    }
}

void ThumbnailListView::setCurrentSelectPath()
{
    m_currentDeletePath.clear();
    m_selectPrePath = "";

    int row;//最后选中的索引
    for (int i = 0; i < selectionModel()->selectedIndexes().size(); i++) {
        QModelIndex index = selectionModel()->selectedIndexes().at(i);
        m_currentDeletePath.append(index.model()->data(index, Qt::DisplayRole).toList().at(1).toString());
        if (i == selectionModel()->selectedIndexes().size() - 1) {
            row = index.row();
            while (m_currentDeletePath.contains(index.model()->data(index, Qt::DisplayRole).toList().at(1).toString())) {
                if (row > 0) {
                    row -= 1;
                    QModelIndex indextemp = index.model()->index(row, 0);
                    if (indextemp.isValid()) {
                        m_selectPrePath = indextemp.data(Qt::DisplayRole).toList().at(1).toString();
                    }
                } else {
                    break;
                }
                if (!m_currentDeletePath.contains(m_selectPrePath))
                    break;
            }
        }
    }
}

void ThumbnailListView::resizeEventF()
{
    if (nullptr == m_item) {
        DScrollBar *bar = this->verticalScrollBar();
        bar->setGeometry(bar->x(), /*bar->y() + */m_scrollbartopdistance, bar->width(), this->height() - m_scrollbartopdistance - m_scrollbarbottomdistance);
    }
    calgridItemsWidth();
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
