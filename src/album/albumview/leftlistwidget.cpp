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

void LeftListWidget::mouseMoveEvent(QMouseEvent *e)
{
    emit sigMouseMoveEvent();
    return DListWidget::mouseMoveEvent(e);
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

void LeftListWidget::mouseReleaseEvent(QMouseEvent *e)
{
    QModelIndex index = indexAt(e->pos());
    if (index.isValid()) {
        emit sigMouseReleaseEvent(index);
    } else {
        QModelIndex currentSelect = this->currentIndex();
        if (currentSelect.isValid())
            emit sigMouseReleaseEvent(currentSelect);
    }
    DListWidget::mouseReleaseEvent(e);
}

void LeftListWidget::SaveRename(QPoint p)
{
    QModelIndex index = indexAt(p);
    if (!index.isValid() || index.row() == 0 || index.row() == 4 || index.row() == 6) {
        //保存当前的修改状态，发送不存在的index点击,0 4 6为标题索引，为有效index
        QModelIndexList indexs = this->selectedIndexes();
        for (auto idx : indexs) {
            AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(this->itemWidget(this->item(idx.row())));
            if (item->m_pLineEdit->isVisible()) { //如果lineedit不可见则表示是在未触发编辑的情况下点击lift list下方的空白处，此时不需要发射这个信号，否则会导致点几下新建几个相册
                emit item->m_pLineEdit->editingFinished();
            }
        }
    }
}
