#include "leftlistwidget.h"

#include <QMouseEvent>
#include <QDebug>
#include "widgets/albumlefttabitem.h"

LeftListWidget::LeftListWidget()
{
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
