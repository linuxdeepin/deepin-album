#include "leftlistwidget.h"

#include <QMouseEvent>
#include <QDebug>
#include "widgets/albumlefttabitem.h"

LeftListWidget::LeftListWidget()
{
    setViewportMargins(8,0,8,0);
}

void LeftListWidget::mousePressEvent(QMouseEvent* e)
{
    QModelIndex index = indexAt(e->pos());

//    qDebug()<<this->currentRow();
//    if (!index.isValid())
//    {
//        qDebug()<<"111111111"<<index;
//    }
//    else {
//        qDebug()<<index.row();
//    }

//    AlbumLeftTabItem *item = (AlbumLeftTabItem*)this->itemWidget(this->currentItem());
//    item->onCheckNameValid();

    DListWidget::mousePressEvent(e);

}

QStyleOptionViewItem LeftListWidget::viewOptions() const
{
    QStyleOptionViewItem option = QAbstractItemView::viewOptions();

    if (this->viewMode() == QListView::IconMode) {
        option.showDecorationSelected = false;
        option.decorationPosition = QStyleOptionViewItem::Top;
        option.displayAlignment = Qt::AlignCenter;
    } else {
        option.decorationPosition = QStyleOptionViewItem::Left;
    }

    if (this->gridSize().isValid()) {
        option.rect.setSize(this->gridSize());
    }

    return option;
}

QModelIndex LeftListWidget::getModelIndex(QListWidgetItem * pItem)
{
    return indexFromItem(pItem);
}
