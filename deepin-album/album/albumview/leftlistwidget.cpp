#include "leftlistwidget.h"

#include <QDebug>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QAbstractItemView>
#include "widgets/albumlefttabitem.h"

LeftListWidget::LeftListWidget()
{
    setViewportMargins(8, 0, 8, 0);
    setAcceptDrops(true);
}

void LeftListWidget::dragMoveEvent(QDragMoveEvent *event)
{
    QModelIndex index = this->indexAt(event->pos());
    if (index.isValid()) {
        AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(this->itemWidget(this->item(index.row())));
        QString leftTabListName = item->m_albumNameStr;
        QString leftTabListType = item->m_albumTypeStr;
        //只支持拖拽到自定义相册
        if (leftTabListType == COMMON_STR_CREATEALBUM) {
            return event->accept();
        } else {
            qDebug() << "Can not drop!";
            return event->ignore();
        }
    }
}

void LeftListWidget::dropEvent(QDropEvent *event)
{
    QModelIndex index = this->indexAt(event->pos());
    if (index.isValid()) {
        qDebug() << "emit signalDropEvent:" << index;
        emit signalDropEvent(index);
    } else {
        DListWidget::dropEvent(event);
    }

}

void LeftListWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("TestListView/text-icon-icon_hover")) {
        event->acceptProposedAction();
    } else {
        QListWidget::dragEnterEvent(event);
    }
}

void LeftListWidget::mousePressEvent(QMouseEvent *e)
{
    QModelIndex index = indexAt(e->pos());
    if (!index.isValid()) {
        emit sigMousePressIsNoValid();
    }
    DListWidget::mousePressEvent(e);
}

void LeftListWidget::keyPressEvent(QKeyEvent *event)
{
    DListWidget::keyPressEvent(event);
}

void LeftListWidget::keyReleaseEvent(QKeyEvent *event)
{
    DListWidget::keyReleaseEvent(event);
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

//QModelIndex LeftListWidget::getModelIndex(QListWidgetItem *pItem)
//{
//    return indexFromItem(pItem);
//}
