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
#include "utils/snifferimageformat.h"
#include <QDateTime>

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPainter>
#include <QPixmapCache>
#include <QStandardItemModel>
#include <QThread>
#include <QTimer>
#include <QMouseEvent>
#include <QImageReader>

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

    QRect backgroundRect = option.rect;

    if (selected)
    {
        QPainterPath backgroundBp;
        backgroundBp.addRoundedRect(backgroundRect, utils::common::BORDER_RADIUS, utils::common::BORDER_RADIUS);
        painter->setClipPath(backgroundBp);

        painter->fillRect(backgroundRect, QBrush(utils::common::LIGHT_CHECKER_COLOR));

        QPixmap selectedPixmap;
        selectedPixmap = utils::base::renderSVG(":/resources/images/other/photo_checked.svg", QSize(data.width, data.height));

        painter->drawPixmap(backgroundRect, selectedPixmap);
    }

    QRect pixmapRect;
    pixmapRect.setX(backgroundRect.x()+7);
    pixmapRect.setY(backgroundRect.y()+7);
    pixmapRect.setWidth(backgroundRect.width()-14);
    pixmapRect.setHeight(backgroundRect.height()-14);

    QPainterPath bp1;
    bp1.addRoundedRect(pixmapRect, utils::common::BORDER_RADIUS, utils::common::BORDER_RADIUS);
    painter->setClipPath(bp1);


    QImage tImg;

    QString format = DetectImageFormat(data.path);
    if (format.isEmpty()) {
        QImageReader reader(data.path);
        reader.setAutoTransform(true);
        if (reader.canRead()) {
            tImg = reader.read();
        }
    } else {
        QImageReader readerF(data.path, format.toLatin1());
        readerF.setAutoTransform(true);
        if (readerF.canRead()) {
            tImg = readerF.read();
        } else {
            qWarning() << "can't read image:" << readerF.errorString()
                       << format;

            tImg = QImage(data.path);
        }
    }

    QPixmap pixmapItem = QPixmap::fromImage(tImg);

    painter->drawPixmap(pixmapRect, pixmapItem);

    if (TRASH_ALBUM == m_imageTypeStr)
    {
        painter->drawText(pixmapRect, Qt::AlignRight|Qt::AlignBottom, "30天");
    }

    if (FAVORITES_ALBUM == m_imageTypeStr)
    {
        QPixmap favPixmap;
        favPixmap = utils::base::renderSVG(":/resources/images/other/fav_icon .svg", QSize(20, 20));

        QRect favRect(pixmapRect.x()+pixmapRect.width()-20-13,pixmapRect.y()+pixmapRect.height()-20-10,20,20);

        painter->drawPixmap(favRect, favPixmap);
    }

    if (selected)
    {
        QPixmap selectedPixmap;
        selectedPixmap = utils::base::renderSVG(":/resources/images/other/select_active.svg", QSize(28, 28));

        QRect selectedRect(backgroundRect.x()+backgroundRect.width()-28,backgroundRect.y(),28,28);
        QPainterPath selectedBp;
        selectedBp.addRoundedRect(selectedRect, utils::common::BORDER_RADIUS, utils::common::BORDER_RADIUS);
        painter->setClipPath(selectedBp);

        painter->drawPixmap(selectedRect, selectedPixmap);
    }
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
    QRect rect = QRect(option.rect.x()+option.rect.width()-20-13-2,option.rect.y()+option.rect.height()-20-10-2,24,24);

    QMouseEvent *pMouseEvent = static_cast<QMouseEvent*>(event);

    if (event->type() == QEvent::MouseButtonPress && rect.contains(pMouseEvent->pos()) && FAVORITES_ALBUM == m_imageTypeStr)
    {
        emit sigCancelFavorite(index);
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
