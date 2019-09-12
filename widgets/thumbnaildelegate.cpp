/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
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
#include "thumbnaildelegate.h"
#include "application.h"
#include "utils/imageutils.h"
#include "utils/baseutils.h"
#include <QDateTime>

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPainter>
#include <QPixmapCache>
#include <QStandardItemModel>
#include <QThread>
#include <QTimer>
#include <QMouseEvent>

namespace
{
const QString IMAGE_DEFAULTTYPE = "All pics";
const QString TRASH_ALBUM = "Trash";
const QString FAVORITES_ALBUM = "My favorite";
}

ThumbnailDelegate::ThumbnailDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
    m_imageTypeStr = IMAGE_DEFAULTTYPE;
}

void ThumbnailDelegate::paint(QPainter *painter,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
{
    const ItemData data = itemData(index);
    if (data.path.isEmpty()) return;

    bool selected = false;

    if (/*(option.state & QStyle::State_MouseOver) &&*/
            (option.state & QStyle::State_Selected) != 0) {
        selected = true;
    }

    painter->setRenderHints(QPainter::HighQualityAntialiasing |
                            QPainter::SmoothPixmapTransform |
                            QPainter::Antialiasing);
    QRect rect = option.rect;

    QPainterPath bp;
    bp.addRoundedRect(rect, utils::common::BORDER_RADIUS, utils::common::BORDER_RADIUS);
    painter->setClipPath(bp);

    painter->fillRect(rect, QBrush(utils::common::LIGHT_CHECKER_COLOR));

    QPixmap pixmapItem;
    pixmapItem.load(data.path);
    painter->drawPixmap(rect, pixmapItem);
    if (TRASH_ALBUM == m_imageTypeStr)
    {
        painter->drawText(rect, Qt::AlignRight|Qt::AlignBottom, "30天");
    }

    if (FAVORITES_ALBUM == m_imageTypeStr)
    {
        QPixmap pixmap1;
        pixmap1 = utils::base::renderSVG(":/resources/images/other/fav_icon .svg", QSize(24, 24));

        QRect rect1(rect.x()+rect.width()-24,rect.y()+rect.height()-24,24,24);

        painter->drawPixmap(rect1, pixmap1);
    }

    // Draw inside border
    QPen p(selected ? utils::common::BORDER_COLOR_SELECTED : m_borderColor,
           selected ? utils::common::BORDER_WIDTH_SELECTED : utils::common::BORDER_WIDTH);
    painter->setPen(p);
    QPainterPathStroker stroker;
    stroker.setWidth(selected ? utils::common::BORDER_WIDTH_SELECTED : utils::common::BORDER_WIDTH);
    stroker.setJoinStyle(Qt::RoundJoin);
    QPainterPath borderPath = stroker.createStroke(bp);
    painter->drawPath(borderPath);
}

QSize ThumbnailDelegate::sizeHint(const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    Q_UNUSED(option)
    return index.model()->data(index, Qt::SizeHintRole).toSize();
}

ThumbnailDelegate::ItemData ThumbnailDelegate::itemData(const QModelIndex &index) const
{
    QVariantList datas = index.model()->data(index, Qt::DisplayRole).toList();
    ItemData data;
    if (datas.length() >= 1) {
        data.name = datas[0].toString();
    }
    if (datas.length() >= 2) {
        data.path = datas[1].toString();
    }
    if (datas.length() >= 3) {
        data.width = datas[2].toInt();
    }
    if (datas.length() >= 4) {
        data.height = datas[3].toInt();
    }

    return data;
}

bool ThumbnailDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    QRect rect = QRect(option.rect.x()+option.rect.width()-24,option.rect.y()+option.rect.height()-24,24,24);

    QMouseEvent *pMouseEvent = static_cast<QMouseEvent*>(event);

    if (event->type() == QEvent::MouseButtonPress && rect.contains(pMouseEvent->pos()) && FAVORITES_ALBUM == m_imageTypeStr)
    {
        emit sigCancelFavorite(index);
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
