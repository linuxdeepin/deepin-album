#include "thumbnaillistview.h"

namespace {
const int ITEM_SPACING = 5;
const int BASE_HEIGHT = 100;
const int LEFT_MARGIN = 12;
const int RIGHT_MARGIN = 8;

const QString SHORTCUTVIEW_GROUP = "SHORTCUTVIEW";

QString ss(const QString &text)
{
    QString str = dApp->setter->value(SHORTCUTVIEW_GROUP, text).toString();
    str.replace(" ", "");
    return str;
}
}  //namespace

ThumbnailListView::ThumbnailListView(QWidget *parent)
    : QListView(parent), m_model(new QStandardItemModel(this))
{
    setViewportMargins(LEFT_MARGIN, 0, RIGHT_MARGIN, 0);
    setIconSize(QSize(400, 400));
    setResizeMode(QListView::Adjust);
    setViewMode(QListView::IconMode);
    setFlow(QListView::LeftToRight);
    setSpacing(ITEM_SPACING);
    setDragEnabled(false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setContextMenuPolicy(Qt::CustomContextMenu);

    m_delegate = new ThumbnailDelegate();

    setItemDelegate(m_delegate);
    setModel(m_model);

    m_pMenu = new QMenu();

    m_iDefaultWidth = 0;


}

ThumbnailListView::~ThumbnailListView()
{

}

void ThumbnailListView::initConnections()
{
//    connect(this, &QListView::customContextMenuRequested,
//            this, &ThumbnailListView::showMenu);
    connect(this, &QListView::customContextMenuRequested,
            this, [=] (const QPoint &pos) {
        if (this->indexAt(pos).isValid()) {
//            showMenuCon();
            m_pMenu->popup(QCursor::pos());
        }
    });
}

void ThumbnailListView::calBasePixMapWandH()
{
    for(int i = 0; i < m_ItemList.length(); i++)
    {
        m_ItemList[i].width = m_ItemList[i].width * BASE_HEIGHT / m_ItemList[i].height;
        m_ItemList[i].height = BASE_HEIGHT;
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
            i_baseWidth = m_ItemList[i].width;

            m_gridItem<<itemInfoList;
            itemInfoList.clear();
            itemInfoList<<m_ItemList[i];

            if (i == m_ItemList.length() -1 )
            {
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
    m_height = m_gridItem[0][0].height;
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
        QPixmap pixmap;
        pixmap.load(m_ItemList[i].path);
        m_ItemList[i].width = pixmap.width();
        m_ItemList[i].height = pixmap.height();
    }

    calBasePixMapWandH();

    if (0 != m_iDefaultWidth)
    {
        calWidgetItemWandH();
        addThumbnailView();
    }
}

void ThumbnailListView::showMenu(const QPoint &pos)
{
    appendAction(IdView, tr("View"), ss("View"));
    appendAction(IdFullScreen, tr("Fullscreen"), ss("Fullscreen"));

    m_pMenu->popup(QCursor::pos());
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
