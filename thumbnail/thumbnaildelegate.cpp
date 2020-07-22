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
#include "utils/imageutils.h"
#include "utils/baseutils.h"
#include "utils/snifferimageformat.h"
#include "application.h"
#include <QDateTime>

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>
#include <QStandardItemModel>
#include <QThread>
#include <QTimer>
#include <QMouseEvent>
#include <QImageReader>
#include <QApplication>
#include <DFontSizeManager>
#include <DGuiApplicationHelper>

namespace {
const QString IMAGE_DEFAULTTYPE = "All pics";
}

ThumbnailDelegate::ThumbnailDelegate(DelegateType type, QObject *parent)
    : QStyledItemDelegate(parent), m_delegatetype(type)
{
    m_imageTypeStr = IMAGE_DEFAULTTYPE;
}


void ThumbnailDelegate::setNeedPaint(bool value)
{
    bneedpaint = value;
}

void ThumbnailDelegate::paint(QPainter *painter,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    if (!bneedpaint) {
        return;
    }
    painter->save();
    const ItemData data = itemData(index);

//    if (data.path.isEmpty()) return;

    bool selected = false;

    if (/*(option.state & QStyle::State_MouseOver) &&*/
        (option.state & QStyle::State_Selected) != 0) {
        selected = true;
    }

    painter->setRenderHints(QPainter::HighQualityAntialiasing |
                            QPainter::SmoothPixmapTransform |
                            QPainter::Antialiasing);

    QRect backgroundRect = option.rect;
    if (AllPicViewType == m_delegatetype) {
        if ("First" == data.firstorlast) {
            QStyleOptionFrame *FrameOption = new QStyleOptionFrame();
            FrameOption->rect = QRect(backgroundRect.x(), backgroundRect.y(), backgroundRect.width(), 50);
            //绘制
            QApplication::style()->drawControl(QStyle::CE_ShapedFrame, FrameOption, painter);
            backgroundRect.setY(backgroundRect.y() + 50);
            delete FrameOption;
        }
        if ("Last" == data.firstorlast) {
            backgroundRect.setHeight(backgroundRect.height() - 27);
        }
    } else if (ThumbnailDelegate::AlbumViewType == m_delegatetype) {
        if ("Last" == data.firstorlast) {
            backgroundRect.setHeight(backgroundRect.height() - 27);
        }
    } else if (ThumbnailDelegate::SearchViewType == m_delegatetype || ThumbnailDelegate::AlbumViewPhoneType == m_delegatetype) {
        if ("First" == data.firstorlast) {
            QStyleOptionFrame *FrameOption = new QStyleOptionFrame();
            FrameOption->rect = QRect(backgroundRect.x(), backgroundRect.y(), backgroundRect.width(), 130);
            //绘制
            QApplication::style()->drawControl(QStyle::CE_ShapedFrame, FrameOption, painter);
            backgroundRect.setY(backgroundRect.y() + 130);
        }
        if ("Last" == data.firstorlast) {
            backgroundRect.setHeight(backgroundRect.height() - 27);
        }
    }

    if (selected) {
        QPainterPath backgroundBp;
        backgroundBp.addRoundedRect(backgroundRect, utils::common::BORDER_RADIUS, utils::common::BORDER_RADIUS);
        painter->setClipPath(backgroundBp);

        painter->fillRect(backgroundRect, QBrush(utils::common::LIGHT_CHECKER_COLOR));

        QPixmap selectedPixmap;
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType) {
            selectedPixmap = utils::base::renderSVG(":/resources/images/other/photo_checked.svg", QSize(data.width, data.height));
        }
        if (themeType == DGuiApplicationHelper::DarkType) {
            selectedPixmap = utils::base::renderSVG(":/images/logo/resources/images/other/photo_checked_dark.svg", QSize(data.width, data.height));
        }
        painter->drawPixmap(backgroundRect, selectedPixmap);
    }

    float fwidth = ((float)backgroundRect.height()) / ((float)data.baseHeight) * ((float)data.baseWidth) / ((float)backgroundRect.width());
    float fheight = ((float)backgroundRect.width()) / ((float)data.baseWidth) * ((float)data.baseHeight) / ((float)backgroundRect.height());
    QRect pixmapRect;
    if ((data.width > data.imgWidth + 12) && fheight <= 3) {
        pixmapRect.setX(backgroundRect.x() + (data.width - data.imgWidth) / 2);
        pixmapRect.setWidth(data.imgWidth);
    } else {
        pixmapRect.setX(backgroundRect.x() + 6);
        pixmapRect.setWidth(backgroundRect.width() - 12);
    }
    if ((data.height > data.imgHeight + 12) && fwidth <= 1.5) {
        pixmapRect.setY(backgroundRect.y() + (data.height - data.imgHeight) / 2);
        pixmapRect.setHeight(data.imgHeight);
    } else {
        pixmapRect.setY(backgroundRect.y() + 6);;
        pixmapRect.setHeight(backgroundRect.height() - 12);
    }

    QPainterPath bp1;
    bp1.addRoundedRect(pixmapRect, utils::common::BORDER_RADIUS, utils::common::BORDER_RADIUS);
    painter->setClipPath(bp1);

    if (fwidth > 1.5) {
        painter->drawPixmap(pixmapRect.x(), pixmapRect.y(), /*dApp->m_imagemap.value(data.path)*/data.image.scaled(((float)pixmapRect.height()) / ((float)data.baseHeight) * data.baseWidth, pixmapRect.height()));
    } else if (fheight > 3) {
        painter->drawPixmap(pixmapRect.x(), pixmapRect.y(), /*dApp->m_imagemap.value(data.path)*/data.image.scaled(pixmapRect.width(), ((float)pixmapRect.width()) / ((float)data.baseWidth) * data.baseHeight));
    } else {
        painter->drawPixmap(pixmapRect, /*dApp->m_imagemap.value(data.path)*/data.image);
    }
    //绘制剩余天数
    if (COMMON_STR_TRASH == m_imageTypeStr) {

        painter->setPen(QColor(85, 85, 85, 170));
//        QBrush brush;

        painter->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T8));
        //字符串的像素宽度
        const int m_Width = painter->fontMetrics().width(data.remainDays);
//        int m_Height = painter->fontMetrics().height();

        painter->setBrush(QBrush(QColor(85, 85, 85, 170)));
//        painter->drawRoundedRect(pixmapRect.x() + pixmapRect.width() - 40, pixmapRect.y() + pixmapRect.height() - 18, 48, 16, 8, 8);

        //2020/3/13-xiaolong
        int rectwidth =pixmapRect.width();  //缩略图宽度
        int tempcha = (rectwidth-m_Width>4)?(rectwidth-m_Width-4):4;
        int posx = pixmapRect.x() + tempcha;    //剩余天数起始坐标
        int textwidth =m_Width;
        if(m_Width+tempcha>rectwidth)   //文字像素宽度大于缩略图宽度
            textwidth = rectwidth-4;

        painter->drawRoundedRect(posx,pixmapRect.y() + pixmapRect.height() - 28,textwidth,20,8,8);

        //painter->drawRoundedRect(pixmapRect.x() + pixmapRect.width() - m_Width - 4, pixmapRect.y() + pixmapRect.height() - 28, m_Width + 4, 20, 8, 8);
        painter->setPen(QColor(255, 255, 255));

//        qDebug() << "m_Width      " << m_Width << endl;
//        painter->drawText(pixmapRect.x() + pixmapRect.width() - m_Width - 4, pixmapRect.y() + pixmapRect.height() - 13, data.remainDays);

        QString str(data.remainDays);
        QFontMetrics fontwidth(str);
        if(m_Width-textwidth>0)
            str = fontwidth.elidedText(str,Qt::ElideRight,textwidth);   //超出部分隐藏...
        painter->drawText(posx,pixmapRect.y() + pixmapRect.height() - 13,str);
    }

    if (COMMON_STR_FAVORITES == m_imageTypeStr) {
        QPixmap favPixmap;
        favPixmap = utils::base::renderSVG(":/resources/images/other/fav_icon .svg", QSize(20, 20));

        QRect favRect(pixmapRect.x() + pixmapRect.width() - 20 - 13, pixmapRect.y() + pixmapRect.height() - 20 - 10, 20, 20);

        painter->drawPixmap(favRect, favPixmap);
    }

    if (selected) {
        QPixmap selectedPixmap;
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType) {
            selectedPixmap = utils::base::renderSVG(":/resources/images/other/select_active.svg", QSize(28, 28));
        }
        if (themeType == DGuiApplicationHelper::DarkType) {
            selectedPixmap = utils::base::renderSVG(":/images/logo/resources/images/other/select_active_dark.svg", QSize(28, 28));
        }
        QRect selectedRect(backgroundRect.x() + backgroundRect.width() - 28, backgroundRect.y(), 28, 28);
        QPainterPath selectedBp;
        selectedBp.addRoundedRect(selectedRect, utils::common::BORDER_RADIUS, utils::common::BORDER_RADIUS);
        painter->setClipPath(selectedBp);

        painter->drawPixmap(selectedRect, selectedPixmap);
    }
//    if ("Last" == data.firstorlast) {
//        QStyleOptionFrame *FrameOption = new QStyleOptionFrame();
//        FrameOption->rect = QRect(backgroundRect.x(), backgroundRect.y() + backgroundRect.height(), backgroundRect.width(), 27);
//        //绘制
//        QApplication::style()->drawControl(QStyle::CE_ShapedFrame, FrameOption, painter);
//    }
    painter->restore();
}

QSize ThumbnailDelegate::sizeHint(const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
{
    Q_UNUSED(option)
    bool bl = false;
    bl = index.isValid();
    if (bl)
        return index.model()->data(index, Qt::SizeHintRole).toSize();
    else
        return QSize(0, 0);
}

ThumbnailDelegate::ItemData ThumbnailDelegate::itemData(const QModelIndex &index) const
{
    QVariantList datas = index.model()->data(index, Qt::DisplayRole).toList();
    ItemData data;
    if (datas.length() >= 1) {
        data.name = datas[0].toString();
    }
    if (datas.length() >= 2) {
        data.path =  datas[1].toString();
    }
    if (datas.length() >= 3) {
        data.width = datas[2].toInt();
    }
    if (datas.length() >= 4) {
        data.height = datas[3].toInt();
    }
    if (datas.length() >= 5) {
        data.remainDays = datas[4].toString();
    }
    if (datas.length() >= 6) {
        data.image = datas[5].value<QPixmap>();
    }
    if (datas.length() >= 7) {
        data.imgWidth = datas[6].toInt();
    }
    if (datas.length() >= 8) {
        data.imgHeight = datas[7].toInt();
    }
    if (datas.length() >= 9) {
        data.baseWidth = datas[8].toInt();
    }
    if (datas.length() >= 10) {
        data.baseHeight = datas[9].toInt();
    }
    if (datas.length() >= 11) {
        data.firstorlast = datas[10].toString();
    }
    return data;
}

bool ThumbnailDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    qDebug() << "option:" << option << endl;
    QRect rect = QRect(option.rect.x() + option.rect.width() - 20 - 13 - 2, option.rect.y() + option.rect.height() - 20 - 10 - 2, 20, 20);

    QMouseEvent *pMouseEvent = static_cast<QMouseEvent *>(event);
//    qDebug() << "rect+++++++++++++++++" << rect << endl;
//    qDebug() << "pMouseEvent->pos()+++++++++++++" << pMouseEvent->pos() << endl;
//    qDebug() << "rect.contains(pMouseEvent->pos()" << rect.contains(pMouseEvent->pos()) << endl;
//    qDebug() << " rect.contains(QPoint(pMouseEvent->pos().x(), pMouseEvent->pos().y() - 20)" << rect.contains(pMouseEvent->x(), pMouseEvent->y() + 20) << endl;
    if (COMMON_STR_FAVORITES == m_imageTypeStr) {
//        if (event->type() == QEvent::MouseButtonPress && rect.contains(pMouseEvent->pos())) {
//            emit sigCancelFavorite(index);
//        }
//        if (event->type() == QEvent::MouseButtonPress && rect.contains(pMouseEvent->x(), pMouseEvent->y() + 27)) {
//            emit sigCancelFavorite(index);
//        }

        if (event->type() == QEvent::MouseButtonPress) {
            const ItemData data = itemData(index);

            bool blast = false;
            if (AllPicViewType == m_delegatetype) {
                if ("Last" == data.firstorlast) {
                    blast = true;
                }
            } else if (ThumbnailDelegate::AlbumViewType == m_delegatetype) {
                if ("Last" == data.firstorlast) {
                    blast = true;
                }
            } else if (ThumbnailDelegate::SearchViewType == m_delegatetype || ThumbnailDelegate::AlbumViewPhoneType == m_delegatetype) {
                if ("Last" == data.firstorlast) {
                    blast = true;
                }
            }
            if (!blast && rect.contains(pMouseEvent->pos())) {
                emit sigCancelFavorite(index);
            } else if (blast && event->type() == QEvent::MouseButtonPress && rect.contains(pMouseEvent->x(), pMouseEvent->y() + 27)) {
                emit sigCancelFavorite(index);
            }

        }
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
