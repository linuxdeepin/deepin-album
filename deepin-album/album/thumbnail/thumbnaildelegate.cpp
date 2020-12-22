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
#include "application.h"
#include <QDateTime>

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPainter>
#include <QPixmapCache>
#include <QStandardItemModel>
#include <QThread>
#include <QTimer>
#include <QPainterPath>
#include <QMouseEvent>
#include <QImageReader>
#include <QApplication>
#include <DFontSizeManager>
#include <DGuiApplicationHelper>

namespace {
const QString IMAGE_DEFAULTTYPE = "All pics";
}

const int NotSupportedOrDamagedWidth = 40;      //损坏图片宽度
const int NotSupportedOrDamagedHeigh = 40;

ThumbnailDelegate::ThumbnailDelegate(DelegateType type, QObject *parent)
    : QStyledItemDelegate(parent)
    , m_imageTypeStr(IMAGE_DEFAULTTYPE)
    , selectedPixmapLight(utils::base::renderSVG(":/resources/images/other/select_active.svg", QSize(28, 28)))
    , selectedPixmapDark(utils::base::renderSVG(":/images/logo/resources/images/other/select_active_dark.svg", QSize(28, 28)))
    , m_delegatetype(type)
{

}

void ThumbnailDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!bneedpaint) {
        return;
    }
    painter->save();
    const ItemData data = itemData(index);
    bool selected = data.isSelected;
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
            FrameOption->rect = QRect(backgroundRect.x(), backgroundRect.y(), backgroundRect.width(), 134);
            //绘制
            QApplication::style()->drawControl(QStyle::CE_ShapedFrame, FrameOption, painter);
            backgroundRect.setY(backgroundRect.y() + 134);
        }
        if ("Last" == data.firstorlast) {
            backgroundRect.setHeight(backgroundRect.height() - 27);
        }
    }
    //选中阴影框
//    QBrush  backbrush;
    if (selected) {
        QPainterPath backgroundBp;
        backgroundBp.addRoundedRect(backgroundRect, utils::common::SHADOW_BORDER_RADIUS, utils::common::SHADOW_BORDER_RADIUS);
        painter->setClipPath(backgroundBp);

        QBrush  shadowbrush;
//        QBrush  backbrush;
        QPixmap selectedPixmap;
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType) {
//            selectedPixmap = utils::base::renderSVG(":/resources/images/other/photo_checked.svg", QSize(data.width, data.height));
//            backbrush = QBrush(utils::common::LIGHT_BACKGROUND_COLOR);
            shadowbrush = QBrush(QColor("#DEDEDE"));
        }
        if (themeType == DGuiApplicationHelper::DarkType) {
//            selectedPixmap = utils::base::renderSVG(":/images/logo/resources/images/other/photo_checked_dark.svg", QSize(data.width, data.height));
//            backbrush = QBrush(utils::common::DARK_BACKGROUND_COLOR2);
//            shadowbrush = QBrush(QColor("#1E1E1E"));
            shadowbrush = QBrush(QColor("#4F4F4F"));
        }
        painter->fillRect(backgroundRect, shadowbrush);
//        painter->drawPixmap(backgroundRect, selectedPixmap);
//        painter->drawPath(backgroundBp);

        //绘制选中默认背景
        QRect backRect(backgroundRect.x() + 8, backgroundRect.y() + 8, backgroundRect.width() - 16, backgroundRect.height() - 16);
        QPainterPath backBp;
        backBp.addRoundedRect(backRect, utils::common::BORDER_RADIUS, utils::common::BORDER_RADIUS);
        painter->setClipPath(backBp);
        painter->fillRect(backRect, shadowbrush);
    }

    float fwidth = (backgroundRect.height()) / (data.baseHeight == 0 ? 1 : data.baseHeight) * (data.baseWidth) / (backgroundRect.width());
    float fheight = (backgroundRect.width()) / (data.baseWidth  == 0 ? 1 : data.baseWidth) * (data.baseHeight) / (backgroundRect.height());
    QRect pixmapRect;
    if ((data.width > data.imgWidth + 16) && fheight <= 3) {
        pixmapRect.setX(backgroundRect.x() + (data.width - data.imgWidth) / 2);
        pixmapRect.setWidth(data.imgWidth);
    } else {
        if (data.bNotSupportedOrDamaged) {
            pixmapRect.setX(backgroundRect.x() + backgroundRect.width() / 2 - NotSupportedOrDamagedWidth / 2);
            pixmapRect.setWidth(NotSupportedOrDamagedWidth);
        } else {
            pixmapRect.setX(backgroundRect.x() + 8);
            pixmapRect.setWidth(backgroundRect.width() - 16);
        }
    }
    if ((data.height > data.imgHeight + 16) && (fwidth <= 1.5f)) {
        pixmapRect.setY(backgroundRect.y() + (data.height - data.imgHeight) / 2);
        pixmapRect.setHeight(data.imgHeight);
    } else {
        if (data.bNotSupportedOrDamaged) {
            pixmapRect.setY(backgroundRect.y() + backgroundRect.height() / 2 - NotSupportedOrDamagedHeigh / 2);
            pixmapRect.setHeight(NotSupportedOrDamagedHeigh);
        } else {
            pixmapRect.setY(backgroundRect.y() + 8);;
            pixmapRect.setHeight(backgroundRect.height() - 16);
        }
    }
    //2020/6/9 DJH UI 透明图片背景
    QBrush transparentbrush;
    DGuiApplicationHelper::ColorType themeType1 = DGuiApplicationHelper::instance()->themeType();
    if (themeType1 == DGuiApplicationHelper::LightType) {
        transparentbrush = QBrush(QColor("#FFFFFF"));
    }
    QRect transparentRect(backgroundRect.x() + 8, backgroundRect.y() + 8, backgroundRect.width() - 16, backgroundRect.height() - 16);
    QPainterPath transparentBp;
    transparentBp.addRoundedRect(transparentRect, utils::common::BORDER_RADIUS, utils::common::BORDER_RADIUS);
    painter->setClipPath(transparentBp);
    painter->fillRect(transparentRect, transparentbrush);
    //阴影框-深色主题
    QPainterPath backgroundBp;
    backgroundBp.addRoundedRect(backgroundRect, utils::common::SHADOW_BORDER_RADIUS, utils::common::SHADOW_BORDER_RADIUS);
    painter->setClipPath(backgroundBp);
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::DarkType) {
        QRect tempRect(backgroundRect.x() + 8, backgroundRect.y() + 8, backgroundRect.width() - 16, backgroundRect.height() - 16);
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter->setRenderHint(QPainter::HighQualityAntialiasing, true);
        QColor color(00, 00, 00, 255);
        QBrush  shadowbrush;
        int arr[17] = {255, 200, 100, 90, 80, 70, 60, 50, 40, 30, 20, 10, 8, 6, 4, 2, 1};
        for (int i = 0; i < 17; i++) {
            QPainterPath path;
            path.setFillRule(Qt::OddEvenFill);
            path.addRoundedRect(tempRect.x() + 15 - i, tempRect.y() + 20 - i, tempRect.width() - (15 - i) * 2, tempRect.height() - (15 - i) * 2, 8, 8);
            color.setAlpha(arr[i]);
            painter->setPen(color);
            painter->drawPath(path);
            painter->fillPath(path, QBrush(color));
        }
    }

    QPainterPath bp1;
    bp1.addRoundedRect(pixmapRect, utils::common::BORDER_RADIUS, utils::common::BORDER_RADIUS);
    painter->setClipPath(bp1);

    if (fwidth > 1.5f) {
        painter->drawPixmap(pixmapRect.x(), pixmapRect.y(), data.image.scaled((pixmapRect.height()) / (data.baseHeight) * data.baseWidth, pixmapRect.height()));
    } else if (fheight > 3) {
        QPixmap temp = data.image.scaled(pixmapRect.width(), (pixmapRect.width()) / (data.baseWidth) * data.baseHeight);
        if (temp.isNull()) {
            painter->drawPixmap(pixmapRect.x(), pixmapRect.y(), data.image.scaled(pixmapRect.width(), pixmapRect.height()));
        } else {
            painter->drawPixmap(pixmapRect.x(), pixmapRect.y(), temp);
        }
    } else {
        painter->drawPixmap(pixmapRect, data.image);
    }
    //绘制剩余天数
    if (COMMON_STR_TRASH == m_imageTypeStr) {
        painter->setPen(QColor(85, 85, 85, 170));
        //字符串的像素宽度
        const int m_Width = painter->fontMetrics().width(data.remainDays);
        painter->setBrush(QBrush(QColor(85, 85, 85, 170)));
        QString str(data.remainDays);
        QFontMetrics Text(str);

        //2020/3/13-xiaolong
        int textwidth = m_Width + 6;        //阴影图框
        int textheight = DFontSizeManager::instance()->fontPixelSize(painter->font());
        int rectwidth = pixmapRect.width(); //缩略图宽度
        if (textwidth > rectwidth) //容纳文字像素的宽度大于缩略图宽度
            textwidth = rectwidth - 4;
        int tempcha = (rectwidth - textwidth > 4) ? (rectwidth - textwidth - 4) : 4;
        int posx = pixmapRect.x() + tempcha;    //剩余天数起始坐标
        //文字背景圆角矩形框弧度，与字号相关
        int radious = 6;
        if (textheight < 14)
            radious = 4;
        painter->drawRoundedRect(posx, pixmapRect.y() + pixmapRect.height() - textheight - 4 - 2, textwidth, textheight + 2, radious, radious);
        painter->setPen(QColor(255, 255, 255));
        if (m_Width - textwidth > 0)
            str = Text.elidedText(str, Qt::ElideRight, textwidth);
        painter->drawText(posx + 3, pixmapRect.y() + pixmapRect.height() - 4 - 2, str);
    }

    if (COMMON_STR_FAVORITES == m_imageTypeStr) {
        QPixmap favPixmap;
        favPixmap = utils::base::renderSVG(":/resources/images/other/fav_icon .svg", QSize(20, 20));
        QRect favRect(pixmapRect.x() + pixmapRect.width() - 20 - 13, pixmapRect.y() + pixmapRect.height() - 20 - 10, 20, 20);
        painter->drawPixmap(favRect, favPixmap);
    }

    //绘制选中图标
    if (selected) {
        QPixmap selectedPixmap;
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType) {
            selectedPixmap = selectedPixmapLight;
        }
        if (themeType == DGuiApplicationHelper::DarkType) {
            selectedPixmap = selectedPixmapDark;
        }
//        QRect selectedRect(backgroundRect.x() + backgroundRect.width() - 28, backgroundRect.y(), 28, 28);
        QRect selectedRect(backgroundRect.x() + backgroundRect.width() - 30, backgroundRect.y() + 4, 28, 28);
        QPainterPath selectedBp;
        selectedBp.addRoundedRect(selectedRect, utils::common::BORDER_RADIUS, utils::common::BORDER_RADIUS);
        painter->setClipPath(selectedBp);
        painter->drawPixmap(selectedRect, selectedPixmap);
    }
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
    data.isSelected = index.data(Qt::UserRole).toBool();
    if (datas.length() >= 12) {
        data.bNotSupportedOrDamaged = datas[11].toBool();
    }
    return data;
}

bool ThumbnailDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    Q_UNUSED(model);
    if (!index.isValid())
        return false;
    QRect rect = QRect(option.rect.x() + option.rect.width() - 20 - 13 - 2, option.rect.y() + option.rect.height() - 20 - 10 - 2, 20, 20);
    QMouseEvent *pMouseEvent = static_cast<QMouseEvent *>(event);
    if (COMMON_STR_FAVORITES == m_imageTypeStr) {
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
    return false;
}
