#include "thumbnaillistview.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "controller/signalmanager.h"
#include "utils/snifferimageformat.h"
#include "dialogs/imgdeletedialog.h"
#include <QDebug>
#include <QImageReader>
#include <QFileInfo>
#include "timelinelist.h"
#include <QScrollBar>

namespace {
const int ITEM_SPACING = 5;
const int BASE_HEIGHT = 100;
const int LEFT_MARGIN = 12;
const int RIGHT_MARGIN = 8;

//const QString IMAGE_DEFAULTTYPE = "All pics";
const QString IMAGE_DEFAULTTYPE = "All Photos";
const QString SHORTCUTVIEW_GROUP = "SHORTCUTVIEW";

using namespace utils::common;

QString ss(const QString &text)
{
    QString str = dApp->setter->value(SHORTCUTVIEW_GROUP, text).toString();
    str.replace(" ", "");

    return str;
}
}  //namespace

ThumbnailListView::ThumbnailListView(QString imgtype)
    : m_model(new QStandardItemModel(this))
{
    m_imageType = imgtype;

    m_iDefaultWidth = 0;
    m_iBaseHeight = BASE_HEIGHT;
    m_albumMenu = nullptr;

//    setViewportMargins(LEFT_MARGIN, 0, RIGHT_MARGIN, 0);
    setIconSize(QSize(400, 400));
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
    setMinimumWidth(800);

    m_delegate = new ThumbnailDelegate();
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

void ThumbnailListView::mousePressEvent(QMouseEvent *event)
{
    qDebug() << "m_imageType: " << m_imageType << " ;dragDropMode(): " << dragDropMode();
    if ((m_imageType != COMMON_STR_VIEW_TIMELINE) &&
            (m_imageType != "All Photos") &&
            (m_imageType != COMMON_STR_TRASH) &&
            (m_imageType != ALBUM_PATHTYPE_BY_PHONE)) {
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

    DListView::mousePressEvent(event);
}

void ThumbnailListView::mouseMoveEvent(QMouseEvent *event)
{
    DListView::mouseMoveEvent(event);
}

void ThumbnailListView::mouseReleaseEvent(QMouseEvent *event)
{
    DListView::mouseReleaseEvent(event);

    emit sigMouseRelease();
}

void ThumbnailListView::dragEnterEvent(QDragEnterEvent *event)
{
    qDebug() << "dragEnter";

    DListView::dragEnterEvent(event);
}

void ThumbnailListView::dragMoveEvent(QDragMoveEvent *event)
{
    DListView::dragMoveEvent(event);
}

void ThumbnailListView::dragLeaveEvent(QDragLeaveEvent *event)
{
    m_dragItemPath = selectedPaths();
    qDebug() << m_dragItemPath;

    DListView::dragLeaveEvent(event);
}

void ThumbnailListView::dropEvent(QDropEvent *event)
{
    qDebug() << "drop";

//    DListView::dropEvent(event);
}

void ThumbnailListView::initConnections()
{
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
    connect(dApp->signalM, &SignalManager::sigMainwindowSliderValueChg, this, &ThumbnailListView::onPixMapScale);
    connect(m_delegate, &ThumbnailDelegate::sigCancelFavorite, this, &ThumbnailListView::onCancelFavorite);
}

void ThumbnailListView::calBasePixMapWandH()
{
    int i_totalwidth = width() - 36;      //same as i_totalwidth in calWidgetItemWandH()

    for (int i = 0; i < m_ItemList.length(); i++) {
        if (0 == m_ItemList[i].height || 0 == m_ItemList[i].width) {
            m_ItemList[i].width = m_iBaseHeight;
            m_ItemList[i].height = m_iBaseHeight;
        } else {
            m_ItemList[i].width = m_ItemList[i].width * m_iBaseHeight / m_ItemList[i].height;
            if (m_ItemList[i].width > i_totalwidth) {
                m_ItemList[i].height = m_ItemList[i].height * i_totalwidth / m_ItemList[i].width;
                m_ItemList[i].width = i_totalwidth;
            } else {
                m_ItemList[i].height = m_iBaseHeight;
            }
        }

        m_ItemList[i].imgHeight = m_ItemList[i].height;
        m_ItemList[i].imgWidth = m_ItemList[i].width;
        //Prevents height or width less than 2 after scaling
        m_ItemList[i].imgHeight = (1 > m_ItemList[i].imgHeight) ? 1 : m_ItemList[i].imgHeight;
        m_ItemList[i].imgWidth = (1 > m_ItemList[i].imgWidth) ? 1 : m_ItemList[i].imgWidth;
        m_ItemList[i].height = (m_iBaseHeight > m_ItemList[i].height) ? m_iBaseHeight : m_ItemList[i].height;
        m_ItemList[i].width = (28 > m_ItemList[i].width) ? 28 : m_ItemList[i].width;
    }
}

void ThumbnailListView::calWidgetItemWandH()
{
    int i_baseWidth = 0;
    int i_totalwidth = width() - 36;      //same as i_totalwidth in calBasePixMapWandH()

    QList<int> rowWidthList;
    QList<ItemInfo> itemInfoList;

    rowWidthList.clear();
    itemInfoList.clear();
    m_gridItem.clear();

    //set rows for list
    for (int i = 0; i < m_ItemList.length(); i++) {
        if ((i_baseWidth + m_ItemList[i].width) <= i_totalwidth) {
            i_baseWidth = i_baseWidth + m_ItemList[i].width + ITEM_SPACING;
            itemInfoList << m_ItemList[i];

            if (i == m_ItemList.length() - 1 ) {
                i_baseWidth -= ITEM_SPACING;
                rowWidthList << i_baseWidth;
                m_gridItem << itemInfoList;
            }
        } else if (i_totalwidth - i_baseWidth > 200) {
            m_ItemList[i].imgHeight = m_ItemList[i].imgHeight * (i_totalwidth - i_baseWidth) / m_ItemList[i].imgWidth;
            m_ItemList[i].imgHeight = (1 > m_ItemList[i].imgHeight) ? 1 : m_ItemList[i].imgHeight;
            m_ItemList[i].imgWidth = i_totalwidth - i_baseWidth;
            m_ItemList[i].width = i_totalwidth - i_baseWidth;

            i_baseWidth = i_totalwidth;
            rowWidthList << i_baseWidth;

            itemInfoList << m_ItemList[i];
            m_gridItem << itemInfoList;

            i_baseWidth = 0;
            itemInfoList.clear();
        } else {
            i_baseWidth -= ITEM_SPACING;
            rowWidthList << i_baseWidth;
            i_baseWidth = m_ItemList[i].width + ITEM_SPACING;

            m_gridItem << itemInfoList;
            itemInfoList.clear();
            itemInfoList << m_ItemList[i];

            if (i == m_ItemList.length() - 1 ) {
                i_baseWidth -= ITEM_SPACING;
                rowWidthList << i_baseWidth;
                m_gridItem << itemInfoList;
            }
        }
    }

    //scaling for each row adapting list width except last one
    for (int i = 0; i < rowWidthList.length() - 1; i++) {
        if (rowWidthList[i] < i_totalwidth) {
            int i_totalwidthExSpace = i_totalwidth - ITEM_SPACING * m_gridItem[i].length();
            int rowWidthListExSpace = rowWidthList[i] - ITEM_SPACING * m_gridItem[i].length();
            int rowWidth = 0;
            for (int j = 0; j < m_gridItem[i].length(); j++) {
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

    if (0 < m_gridItem.length()) {
        m_height = 0;
        for (int i = 0; i < rowWidthList.length(); i++) {
            m_height = m_height + m_gridItem[i][0].height + ITEM_SPACING;
        }
        m_height -= ITEM_SPACING;
    }
}

void ThumbnailListView::addThumbnailView()
{
    m_model->clear();
    for (int i = 0; i < m_gridItem.length(); i++) {
        for (int j = 0; j < m_gridItem[i].length(); j++) {
            QStandardItem *item = new QStandardItem;

            QVariantList datas;
            datas.append(QVariant(m_gridItem[i][j].name));
            datas.append(QVariant(m_gridItem[i][j].path));
            datas.append(QVariant(m_gridItem[i][j].width));
            datas.append(QVariant(m_gridItem[i][j].height));
            datas.append(QVariant(m_gridItem[i][j].remainDays));
            datas.append(QVariant(m_gridItem[i][j].image));
            datas.append(QVariant(m_gridItem[i][j].imgWidth));
            datas.append(QVariant(m_gridItem[i][j].imgHeight));

            item->setData(QVariant(datas), Qt::DisplayRole);
            item->setData(QVariant(QSize(m_gridItem[i][j].width, m_gridItem[i][j].height)), Qt::SizeHintRole);
            m_model->appendRow(item);
        }
    }
}

void ThumbnailListView::updateThumbnailView()
{
    int index = 0;
    for (int i = 0; i < m_gridItem.length(); i++) {
        for (int j = 0; j < m_gridItem[i].length(); j++) {
            QSize picSize(m_gridItem[i][j].width, m_gridItem[i][j].height);

            m_model->item(index, 0)->setSizeHint(picSize);
            index++;
        }
    }
}

void ThumbnailListView::insertThumbnails(const QList<ItemInfo> &itemList)
{
    m_delegate->m_imageTypeStr = m_imageType;
    m_ItemList = itemList;

    for (int i = 0; i < m_ItemList.length(); i++) {
        QImage tImg;

//        m_ItemList[i].width = m_ItemList[i].image.width();
//        m_ItemList[i].height = m_ItemList[i].image.height();
        m_ItemList[i].width = m_ItemList[i].width;
        m_ItemList[i].height = m_ItemList[i].height;
    }

    calBasePixMapWandH();

    if (0 != m_iDefaultWidth) {
        calWidgetItemWandH();
        addThumbnailView();
    }
}

void ThumbnailListView::onShowMenu(const QPoint &pos)
{
    //外接设备显示照片时，禁用鼠标右键菜单
    if (!this->indexAt(pos).isValid() || ALBUM_PATHTYPE_BY_PHONE == m_imageType) {
        return;
    }

    updateMenuContents();
    m_pMenu->popup(QCursor::pos());
}

void ThumbnailListView::updateMenuContents()
{
    if (m_imageType.compare(COMMON_STR_TRASH) == 0) {
        return;
    }
    QStringList paths;
    if (m_imageType == COMMON_STR_VIEW_TIMELINE) {
        emit sigGetSelectedPaths(&paths);
    } else {
        paths = selectedPaths();
    }
    paths.removeAll(QString(""));

    foreach (QAction *action, m_MenuActionMap.values()) {
        action->setVisible(true);
    }

    if (1 != paths.length()) {

        m_MenuActionMap.value(tr("View"))->setVisible(false);
        m_MenuActionMap.value(tr("Fullscreen"))->setVisible(false);
        m_MenuActionMap.value(tr("Export"))->setEnabled(true);
    } else {
        bool ret = true;
        QString strSuffix = QFileInfo(paths.at(0)).completeSuffix();
        if (strSuffix.compare("jpeg") && strSuffix.compare("jpg") && strSuffix.compare("bmp")
                && strSuffix.compare("png") && strSuffix.compare("ppm") && strSuffix.compare("xbm")
                && strSuffix.compare("xpm")) {
            ret = false;
        }

        m_MenuActionMap.value(tr("Export"))->setEnabled(ret);
    }
    if (COMMON_STR_TRASH == m_imageType) {

        m_MenuActionMap.value(tr("Move to trash"))->setVisible(false);
    } else {
        m_albumMenu->deleteLater();
        m_albumMenu = createAlbumMenu();
        if (m_albumMenu) {

            QAction *action = m_MenuActionMap.value(tr("Export"));
            m_pMenu->insertMenu(action, m_albumMenu);
        }
    }


    if (1 == paths.length() && COMMON_STR_TRASH != m_imageType) {
        if (COMMON_STR_RECENT_IMPORTED == m_imageType
                || IMAGE_DEFAULTTYPE == m_imageType
                || COMMON_STR_VIEW_TIMELINE == m_imageType) {
            m_MenuActionMap.value(tr("Remove from album"))->setVisible(false);
        }

        if (DBManager::instance()->isImgExistInAlbum(COMMON_STR_FAVORITES, paths[0])) {
            m_MenuActionMap.value(tr("Favorite"))->setVisible(false);
        } else {
            m_MenuActionMap.value(tr("Unfavorite"))->setVisible(false);
        }

        m_pMenu->addSeparator();
    } else {
        if (COMMON_STR_RECENT_IMPORTED == m_imageType
                || IMAGE_DEFAULTTYPE == m_imageType
                || COMMON_STR_VIEW_TIMELINE == m_imageType) {
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
        appendAction(IdMoveToTrash, tr("Delete"), ss(THROWTOTRASH_CONTEXT_MENU));
        appendAction(IdTrashRecovery, tr("Recovery"), ss(BUTTON_RECOVERY));
        return;
    }
    m_MenuActionMap.clear();

    appendAction(IdView, tr("View"), ss(VIEW_CONTEXT_MENU));
    appendAction(IdFullScreen, tr("Fullscreen"), ss(FULLSCREEN_CONTEXT_MENU));
    appendAction(IdStartSlideShow, tr("Slide show"), ss(SLIDESHOW_CONTEXT_MENU));

    m_pMenu->addSeparator();
    appendAction(IdExport, tr("Export"), ss(EXPORT_CONTEXT_MENU));
    appendAction(IdCopyToClipboard, tr("Copy"), ss(COPYTOCLIPBOARD_CONTEXT_MENU));
    appendAction(IdMoveToTrash, tr("Delete"), ss(THROWTOTRASH_CONTEXT_MENU));
//    appendAction(IdRemoveFromAlbum, tr("Remove from album"), ss(THROWTOTRASH_CONTEXT_MENU));
    m_pMenu->addSeparator();
    appendAction(IdRemoveFromFavorites, tr("Unfavorite"), ss(UNFAVORITE_CONTEXT_MENU));
    appendAction(IdAddToFavorites, tr("Favorite"), ss(FAVORITE_CONTEXT_MENU));
    m_pMenu->addSeparator();
    appendAction(IdRotateClockwise, tr("Rotate clockwise"),
                 ss(ROTATECLOCKWISE_CONTEXT_MENU));
    appendAction(IdRotateCounterclockwise, tr("Rotate counterclockwise"),
                 ss(ROTATECOUNTERCLOCKWISE_CONTEXT_MENU));
    m_pMenu->addSeparator();
    appendAction(IdSetAsWallpaper, tr("Set as wallpaper"), ss(SETASWALLPAPER_CONTEXT_MENU));
    appendAction(IdDisplayInFileManager, tr("Display in file manager"),
                 ss(DISPLAYINFILEMANAGER_CONTEXT_MENU));
    appendAction(IdImageInfo, tr("Photo info"), ss(ImageInfo_CONTEXT_MENU));
}


QMenu *ThumbnailListView::createAlbumMenu()
{
    QMenu *am = new QMenu(tr("Add to album"));

    QStringList albums = DBManager::instance()->getAllAlbumNames();
    albums.removeAll(COMMON_STR_FAVORITES);
    albums.removeAll(COMMON_STR_TRASH);
    albums.removeAll(COMMON_STR_RECENT_IMPORTED);

    QAction *ac = new QAction(am);
    ac->setProperty("MenuID", IdAddToAlbum);
    ac->setText(tr("New Album"));
    ac->setData("Add to new album");
    am->addAction(ac);
    am->addSeparator();
    for (QString album : albums) {
        QAction *ac = new QAction(am);
        ac->setProperty("MenuID", IdAddToAlbum);
        ac->setText(fontMetrics().elidedText(QString(album).replace("&", "&&"), Qt::ElideMiddle, 200));
        ac->setData(album);
        am->addAction(ac);
    }

    return am;
}

void ThumbnailListView::onMenuItemClicked(QAction *action)
{

    if (m_imageType == COMMON_STR_VIEW_TIMELINE) {
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
        const QVariantList datas =
            index.model()->data(index, Qt::DisplayRole).toList();
        if (datas.length() == 8) {
            paths << datas[1].toString();
        }
    }

    return paths;
}

QStringList ThumbnailListView::getDagItemPath()
{
    return m_dragItemPath;
}

QList<ThumbnailListView::ItemInfo> ThumbnailListView::getAllPaths()
{
    return m_ItemList;
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
    case IdStartSlideShow:
        emit menuOpenImage(path, paths, true, true);
        break;
    case IdAddToAlbum: {
        const QString album = action->data().toString();
        if (album != "Add to new album") {
            if (1 == paths.count()) {
                if (! DBManager::instance()->isImgExistInAlbum(album, paths[0])) {
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
                for (auto path : paths)
                {
                    dApp->m_imagetrashmap.remove(path);
                }

                DBManager::instance()->removeTrashImgInfos(paths);
                emit trashDelete();
            });
        }


        else {
            DBImgInfoList infos;
            for (auto path : paths) {
                DBImgInfo info;
                info = DBManager::instance()->getInfoByPath(path);
                info.time = QDateTime::currentDateTime();
                infos << info;

                dApp->m_imagemap.remove(path);
            }

            dApp->m_imageloader->addTrashImageLoader(paths);
            DBManager::instance()->insertTrashImgInfos(infos);
            DBManager::instance()->removeImgInfos(paths);
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
        if (IMAGE_DEFAULTTYPE != m_imageType
                && COMMON_STR_VIEW_TIMELINE != m_imageType
                && COMMON_STR_RECENT_IMPORTED != m_imageType
                && COMMON_STR_TRASH != m_imageType
                && COMMON_STR_FAVORITES != m_imageType) {
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

        if (COMMON_STR_TRASH == m_imageType) {
            dApp->m_imageloader->updateTrashImageLoader(paths);
        } else {
            dApp->m_imageloader->updateImageLoader(paths);
        }
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

        if (COMMON_STR_TRASH == m_imageType) {
            dApp->m_imageloader->updateTrashImageLoader(paths);
        } else {
            dApp->m_imageloader->updateImageLoader(paths);
        }
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
    emit loadend(m_height + 15);
}

void ThumbnailListView::onCancelFavorite(const QModelIndex &index)
{
    QStringList str;
    QVariantList datas = index.model()->data(index, Qt::DisplayRole).toList();

    if (datas.length() >= 2) {
        str << datas[1].toString();
    }

    DBManager::instance()->removeFromAlbumNoSignal(COMMON_STR_FAVORITES, str);

    m_model->removeRow(index.row());
}

void ThumbnailListView::resizeEvent(QResizeEvent *e)
{
    if (0 == m_iDefaultWidth) {
        calWidgetItemWandH();
        addThumbnailView();
    } else {
        calWidgetItemWandH();
        updateThumbnailView();
    }
//    emit loadend((m_height)*m_gridItem.size()+15);
    emit loadend(m_height + 15);

    m_iDefaultWidth = width();

    QListView::resizeEvent(e);
}

bool ThumbnailListView::eventFilter(QObject *obj, QEvent *e)
{
    Q_UNUSED(obj)
    if (e->type() == QEvent::Wheel && QApplication::keyboardModifiers () == Qt::ControlModifier) {
        return true;
    }
    //add for pageup pagedown for time line view.
    else if (e->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
        if (m_imageType == COMMON_STR_VIEW_TIMELINE) {
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
