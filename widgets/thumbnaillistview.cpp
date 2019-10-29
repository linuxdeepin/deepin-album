#include "thumbnaillistview.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "controller/signalmanager.h"
#include "utils/snifferimageformat.h"

#include <QDebug>
#include <QImageReader>
#include <QFileInfo>

namespace {
const int ITEM_SPACING = 5;
const int BASE_HEIGHT = 100;
const int LEFT_MARGIN = 12;
const int RIGHT_MARGIN = 8;

const QString IMAGE_DEFAULTTYPE = "All pics";
const QString SHORTCUTVIEW_GROUP = "SHORTCUTVIEW";

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

//    setViewportMargins(LEFT_MARGIN, 0, RIGHT_MARGIN, 0);
    DPalette pa;
    pa.setColor(DPalette::Background,QColor(248, 248, 248));
    setAutoFillBackground(true);
    setPalette(pa);

    setIconSize(QSize(400, 400));
    setResizeMode(QListView::Adjust);
    setViewMode(QListView::IconMode);
    setFlow(QListView::LeftToRight);
    setSpacing(ITEM_SPACING);
    setDragEnabled(false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setStyleSheet("background-color:rgb(248, 248, 248, 230)");

    m_delegate = new ThumbnailDelegate();
    m_delegate->m_imageTypeStr = m_imageType;

    setItemDelegate(m_delegate);
    setModel(m_model);

    m_pMenu = new DMenu();

    initConnections();
}

ThumbnailListView::~ThumbnailListView()
{

}

void ThumbnailListView::initConnections()
{
    connect(this, &QListView::customContextMenuRequested, this, &ThumbnailListView::onShowMenu);
    connect(m_pMenu, &DMenu::triggered, this, &ThumbnailListView::onMenuItemClicked);
	connect(this,&ThumbnailListView::doubleClicked,this,[=](const QModelIndex &index){
        qDebug()<<"index is "<<index.row();
        emit openImage(index.row());
    });
    connect(this,&ThumbnailListView::clicked,this,[=](){
            emit hideExtensionPanel();
    });
    connect(dApp->signalM, &SignalManager::sigMainwindowSliderValueChg, this, &ThumbnailListView::onPixMapScale);
    connect(m_delegate, &ThumbnailDelegate::sigCancelFavorite, this, &ThumbnailListView::onCancelFavorite);
}

void ThumbnailListView::calBasePixMapWandH()
{
    for(int i = 0; i < m_ItemList.length(); i++)
    {
        if (0 == m_ItemList[i].height)
        {
            m_ItemList[i].width = m_iBaseHeight;
        }
        else
        {
            m_ItemList[i].width = m_ItemList[i].width * m_iBaseHeight / m_ItemList[i].height;
        }

        m_ItemList[i].height = m_iBaseHeight;
    }
}

void ThumbnailListView::calWidgetItemWandH()
{
    int i_baseWidth = 0;
    int i_totalwidth = width() - 36;

    QList<int> rowWidthList;
    QList<ItemInfo> itemInfoList;

    m_gridItem.clear();

    for(int i = 0; i< m_ItemList.length(); i++)
    {
        if ((i_baseWidth + m_ItemList[i].width) <= i_totalwidth)
        {
            i_baseWidth = i_baseWidth + m_ItemList[i].width + ITEM_SPACING;
            itemInfoList<<m_ItemList[i];

            if (i == m_ItemList.length() -1 )
            {
                i_baseWidth -= ITEM_SPACING;
                rowWidthList<<i_baseWidth;
                m_gridItem<<itemInfoList;
            }
        }
        else
        {
            i_baseWidth -= ITEM_SPACING;
            rowWidthList<<i_baseWidth;
            i_baseWidth = m_ItemList[i].width + ITEM_SPACING;

            m_gridItem<<itemInfoList;
            itemInfoList.clear();
            itemInfoList<<m_ItemList[i];

            if (i == m_ItemList.length() -1 )
            {
                i_baseWidth -= ITEM_SPACING;
                rowWidthList<<i_baseWidth;
                m_gridItem<<itemInfoList;
            }
        }
    }

    for(int i = 0; i < rowWidthList.length(); i++)
    {
        if (i == rowWidthList.length() - 1)
        {
            break;
        }

        int rowWidth = 0;
        for(int j = 0; j < m_gridItem[i].length(); j++)
        {
            m_gridItem[i][j].width = m_gridItem[i][j].width * i_totalwidth / rowWidthList[i];
            m_gridItem[i][j].height = m_gridItem[i][j].height * i_totalwidth / rowWidthList[i];

            rowWidth = rowWidth + m_gridItem[i][j].width + ITEM_SPACING;
        }

        rowWidthList[i] = rowWidth - ITEM_SPACING;
    }


    if (1 < rowWidthList.length() && rowWidthList[0] < i_totalwidth)
    {
        m_gridItem[0][0].width = m_gridItem[0][0].width + i_totalwidth - rowWidthList[0];
    }

    if (0 < m_gridItem.length())
    {
        m_height = m_gridItem[0][0].height;
    }
}

void ThumbnailListView::addThumbnailView()
{
    m_model->clear();
    for(int i = 0; i < m_gridItem.length(); i++)
    {
        for(int j = 0; j < m_gridItem[i].length(); j++)
        {
            QStandardItem *item = new QStandardItem;

            QVariantList datas;
            datas.append(QVariant(m_gridItem[i][j].name));
            datas.append(QVariant(m_gridItem[i][j].path));
            datas.append(QVariant(m_gridItem[i][j].width));
            datas.append(QVariant(m_gridItem[i][j].height));
            datas.append(QVariant(m_gridItem[i][j].remainDays));
            datas.append(QVariant(m_gridItem[i][j].image));

            item->setData(QVariant(datas), Qt::DisplayRole);
            item->setData(QVariant(QSize(m_gridItem[i][j].width, m_gridItem[i][j].height)), Qt::SizeHintRole);
            m_model->appendRow(item);
        }
    }
}

void ThumbnailListView::updateThumbnailView()
{
    int index = 0;
    for(int i = 0; i < m_gridItem.length(); i++)
    {
        for(int j = 0; j < m_gridItem[i].length(); j++)
        {
            QSize picSize(m_gridItem[i][j].width, m_gridItem[i][j].height);

            m_model->item(index, 0)->setSizeHint(picSize);
            index++;
        }
    }
}

void ThumbnailListView::insertThumbnails(const QList<ItemInfo> &itemList)
{
    m_ItemList = itemList;

    for(int i = 0; i < m_ItemList.length(); i++)
    {
        QImage tImg;

        m_ItemList[i].width = m_ItemList[i].image.width();
        m_ItemList[i].height = m_ItemList[i].image.height();
    }

    calBasePixMapWandH();

    if (0 != m_iDefaultWidth)
    {
        calWidgetItemWandH();
        addThumbnailView();
    }
}

void ThumbnailListView::onShowMenu(const QPoint &pos)
{
    if (!this->indexAt(pos).isValid())
    {
        return;
    }

    updateMenuContents();
    m_pMenu->popup(QCursor::pos());
}

void ThumbnailListView::updateMenuContents()
{
    QStringList paths = selectedPaths();
    paths.removeAll(QString(""));

    m_pMenu->clear();
    if (1 == paths.length())
    {
        appendAction(IdView, tr("查看"), ss("View"));
        appendAction(IdFullScreen, tr("全屏"), ss("Fullscreen"));
    }
    appendAction(IdStartSlideShow, tr("幻灯片放映"), ss("Slide show"));
    if (COMMON_STR_TRASH != m_imageType)
    {
        QMenu *am = createAlbumMenu();
        if (am) {
            m_pMenu->addMenu(am);
        }
    }
    m_pMenu->addSeparator();
    appendAction(IdExport, tr("导出"), ss("export"));
    appendAction(IdCopyToClipboard, tr("复制"), ss("Copy to clipboard"));
    if (COMMON_STR_TRASH == m_imageType)
    {
        appendAction(IdMoveToTrash, tr("删除"), ss("Throw to trash"));
    }
    else
    {
        appendAction(IdMoveToTrash, tr("移动到回收站"), ss("Throw to trash"));
    }

    if (IMAGE_DEFAULTTYPE != m_imageType
        && COMMON_STR_RECENT_IMPORTED != m_imageType
        && COMMON_STR_TRASH != m_imageType
        && COMMON_STR_FAVORITES != m_imageType)
    {
        appendAction(IdRemoveFromAlbum, tr("从相册内删除"), ss("Remove from album"));
    }

    m_pMenu->addSeparator();

    if (1 == paths.length() && COMMON_STR_TRASH != m_imageType)
    {
        if (DBManager::instance()->isImgExistInAlbum(COMMON_STR_FAVORITES, paths[0]))
        {
            appendAction(IdRemoveFromFavorites, tr("取消收藏"), ss("Unfavorite"));
        }
        else
        {
            appendAction(IdAddToFavorites, tr("收藏"), ss("Favorite"));
        }

        m_pMenu->addSeparator();
    }

    int a = 0;
    for(auto path: paths)
    {
        if(!utils::image::imageSupportSave(path))
        {
            a = 1;
            break;
        }
    }
    if(0 == a)
    {
        int flag = 0;
        for(auto path: paths){
            if (QFileInfo(path).isReadable() && !QFileInfo(path).isWritable())
            {
                flag = 1;
                break;
            }
        }
        if(flag == 1)
        {
            appendAction_darkmenu(IdRotateClockwise, tr("顺时针旋转"), ss("Rotate clockwise"));
            appendAction_darkmenu(IdRotateCounterclockwise, tr("逆时针旋转"), ss("Rotate counterclockwise"));
        }
        else
        {
            appendAction(IdRotateClockwise, tr("顺时针旋转"), ss("Rotate clockwise"));
            appendAction(IdRotateCounterclockwise, tr("逆时针旋转"), ss("Rotate counterclockwise"));
        }
    }

    if (1 == paths.length())
    {
        m_pMenu->addSeparator();

        if (utils::image::imageSupportSave(paths[0]))
        {
            appendAction(IdSetAsWallpaper, tr("设为壁纸"), ss("Set as wallpaper"));
        }

        appendAction(IdDisplayInFileManager, tr("在文件管理器中显示"), ss("Display in file manager"));
        appendAction(IdImageInfo, tr("图片信息"), ss("Image info"));
    }
}

void ThumbnailListView::appendAction(int id, const QString &text, const QString &shortcut)
{
    QAction *ac = new QAction(m_pMenu);
    addAction(ac);
    ac->setText(text);
    ac->setProperty("MenuID", id);
    ac->setShortcut(QKeySequence(shortcut));
    m_pMenu->addAction(ac);
}

void ThumbnailListView::appendAction_darkmenu(int id, const QString &text, const QString &shortcut)
{
    QAction *ac = new QAction(m_pMenu);
    addAction(ac);
    ac->setText(text);
    ac->setProperty("MenuID", id);
    ac->setShortcut(QKeySequence(shortcut));
    ac->setDisabled(true);
    m_pMenu->addAction(ac);
}

QMenu *ThumbnailListView::createAlbumMenu()
{
    QMenu *am = new QMenu(tr("添加到相册"));

    QStringList albums = DBManager::instance()->getAllAlbumNames();
    albums.removeAll(COMMON_STR_FAVORITES);

    QAction *ac = new QAction(am);
    ac->setProperty("MenuID", IdAddToAlbum);
    ac->setText(tr("新建相册"));
    ac->setData(QString("Add to new album"));
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
    QStringList paths = selectedPaths();
    paths.removeAll(QString(""));
    if (paths.isEmpty()) {
        return;
    }

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
           DBManager::instance()->insertIntoAlbum(album, paths);
        }
        else {
            emit dApp->signalM->createAlbum(paths);
        }
        break;
    }
    case IdCopyToClipboard:
        utils::base::copyImageToClipboard(paths);
        break;
    case IdMoveToTrash:
    {
        if (COMMON_STR_TRASH == m_imageType)
        {
            for(auto path : paths)
            {
                dApp->m_imagetrashmap.remove(path);
            }

            DBManager::instance()->removeTrashImgInfos(paths);
        }
        else
        {
            DBImgInfoList infos;
            for(auto path : paths)
            {
                DBImgInfo info;
                info = DBManager::instance()->getInfoByPath(path);
                info.time = QDateTime::currentDateTime();
                infos<<info;

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
    case IdRemoveFromAlbum:
        DBManager::instance()->removeFromAlbum(m_imageType, paths);
        break;
    case IdRotateClockwise:
    {
        for(QString path : paths)
        {
            utils::image::rotate(path, 90);
        }

        emit dApp->signalM->sigPixMapRotate(paths);
    }
        break;
    case IdRotateCounterclockwise:
    {
        for(QString path : paths)
        {
            utils::image::rotate(path, -90);
        }

        emit dApp->signalM->sigPixMapRotate(paths);
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
    default:
        break;
    }

//    updateMenuContents();
}

QStringList ThumbnailListView::selectedPaths()
{
    QStringList paths;
    for (QModelIndex index : selectionModel()->selectedIndexes()) {
        const QVariantList datas =
                index.model()->data(index, Qt::DisplayRole).toList();
        if (datas.length() == 6) {
            paths << datas[1].toString();
        }
    }

    return paths;
}

void ThumbnailListView::onPixMapScale(int value)
{
    switch(value)
    {
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
}

void ThumbnailListView::onCancelFavorite(const QModelIndex &index)
{
    QStringList str;
    QVariantList datas = index.model()->data(index, Qt::DisplayRole).toList();

    if (datas.length() >= 2) {
        str<<datas[1].toString();
    }

    DBManager::instance()->removeFromAlbum(COMMON_STR_FAVORITES, str);

    m_model->removeRow(index.row());
}



void ThumbnailListView::resizeEvent(QResizeEvent *e)
{
    if (0 == m_iDefaultWidth)
    {
        calWidgetItemWandH();
        addThumbnailView();
    }
    else
    {
        calWidgetItemWandH();
        updateThumbnailView();
    }
    emit loadend((m_height)*m_gridItem.size()+15);

    m_iDefaultWidth = width();

    QListView::resizeEvent(e);
}
