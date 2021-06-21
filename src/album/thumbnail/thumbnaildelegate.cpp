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

void ThumbnailDelegate::setItemSize(QSize size)
{
    m_size = size;
}

void ThumbnailDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!bneedpaint) {
        return;
    }
    painter->save();
    const ItemInfo data = itemData(index);
    bool selected = data.isSelected;
    if (/*(option.state & QStyle::State_MouseOver) &&*/
        (option.state & QStyle::State_Selected) != 0) {
        selected = true;
    }
    painter->setRenderHints(QPainter::HighQualityAntialiasing |
                            QPainter::SmoothPixmapTransform |
                            QPainter::Antialiasing);
    QRect backgroundRect = option.rect;
    //选中阴影框
    if (selected) {
        QPainterPath backgroundBp;
        backgroundBp.addRoundedRect(backgroundRect, utils::common::SHADOW_BORDER_RADIUS, utils::common::SHADOW_BORDER_RADIUS);
        painter->setClipPath(backgroundBp);

        QBrush  shadowbrush;
        QPixmap selectedPixmap;
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType) {
            shadowbrush = QBrush(QColor("#DEDEDE"));
        }
        if (themeType == DGuiApplicationHelper::DarkType) {
            shadowbrush = QBrush(QColor("#4F4F4F"));
        }
        painter->fillRect(backgroundRect, shadowbrush);

        //绘制选中默认背景
        QRect backRect(backgroundRect.x() + 8, backgroundRect.y() + 8, backgroundRect.width() - 16, backgroundRect.height() - 16);
        QPainterPath backBp;
        backBp.addRoundedRect(backRect, utils::common::BORDER_RADIUS, utils::common::BORDER_RADIUS);
        painter->setClipPath(backBp);
        painter->fillRect(backRect, shadowbrush);
    }

    QRect pixmapRect;
    if (data.bNotSupportedOrDamaged) {
        pixmapRect.setX(backgroundRect.x() + backgroundRect.width() / 2 - NotSupportedOrDamagedWidth / 2);
        pixmapRect.setWidth(NotSupportedOrDamagedWidth);

        pixmapRect.setY(backgroundRect.y() + backgroundRect.height() / 2 - NotSupportedOrDamagedHeigh / 2);
        pixmapRect.setHeight(NotSupportedOrDamagedHeigh);
    } else {
        pixmapRect.setX(backgroundRect.x() + 8);
        pixmapRect.setWidth(backgroundRect.width() - 16);

        pixmapRect.setY(backgroundRect.y() + 8);;
        pixmapRect.setHeight(backgroundRect.height() - 16);
    }
    //2020/6/9 DJH UI 透明图片背景
    QBrush transparentbrush;
    DGuiApplicationHelper::ColorType themeType1 = DGuiApplicationHelper::instance()->themeType();
    if (themeType1 == DGuiApplicationHelper::LightType) {
        transparentbrush = QBrush(QColor("#FFFFFF"));
    } else if (themeType1 == DGuiApplicationHelper::DarkType) { //#BUG77517，去除下方的虚化代码，改为直接填充黑色
        transparentbrush = QBrush(QColor("#000000"));
    }
    QRect transparentRect(backgroundRect.x() + 8, backgroundRect.y() + 8, backgroundRect.width() - 16, backgroundRect.height() - 16);
    QPainterPath transparentBp;
    transparentBp.addRoundedRect(transparentRect, utils::common::BORDER_RADIUS, utils::common::BORDER_RADIUS);
    painter->setClipPath(transparentBp);
    painter->fillRect(transparentRect, transparentbrush);

    QPainterPath bp1;
    bp1.addRoundedRect(pixmapRect, utils::common::BORDER_RADIUS, utils::common::BORDER_RADIUS);
    painter->setClipPath(bp1);

    painter->drawPixmap(pixmapRect, data.image);

    if (COMMON_STR_FAVORITES == m_imageTypeStr) {
        QPixmap favPixmap;
        favPixmap = utils::base::renderSVG(":/resources/images/other/fav_icon .svg", QSize(20, 20));
        QRect favRect(pixmapRect.x() + pixmapRect.width() - 20 - 13, pixmapRect.y() + pixmapRect.height() - 20 - 10, 20, 20);
        painter->drawPixmap(favRect, favPixmap);
    }

    //绘制选中图标
    if (selected) {
        QPixmap selectedPixmap;
        DGuiApplicationHelper::ColorType themeType3 = DGuiApplicationHelper::instance()->themeType();
        if (themeType3 == DGuiApplicationHelper::LightType) {
            selectedPixmap = selectedPixmapLight;
        }
        if (themeType3 == DGuiApplicationHelper::DarkType) {
            selectedPixmap = selectedPixmapDark;
        }
//        QRect selectedRect(backgroundRect.x() + backgroundRect.width() - 28, backgroundRect.y(), 28, 28);
        QRect selectedRect(backgroundRect.x() + backgroundRect.width() - 30, backgroundRect.y() + 4, 28, 28);
        QPainterPath selectedBp;
        selectedBp.addRoundedRect(selectedRect, utils::common::BORDER_RADIUS, utils::common::BORDER_RADIUS);
        painter->setClipPath(selectedBp);
        painter->drawPixmap(selectedRect, selectedPixmap);
    }

    //绘制剩余天数
    if (COMMON_STR_TRASH == m_imageTypeStr) {
        QPainterPath bp;
        bp.addRoundedRect(backgroundRect, utils::common::BORDER_RADIUS, utils::common::BORDER_RADIUS);
        painter->setClipPath(bp);
        painter->setPen(QColor(85, 85, 85, 170)); //边框颜色：灰色
        //字符串的像素宽度
        const int m_Width = painter->fontMetrics().width(data.remainDays);
        painter->setBrush(QBrush(QColor(85, 85, 85, 170)));//填充颜色：灰色
        QString str(data.remainDays);
        QFontMetrics Text(str);

        //2020/3/13-xiaolong
        int textwidth = m_Width + 6;        //阴影图框：6：左3像素+右3像素
        int textheight = DFontSizeManager::instance()->fontPixelSize(painter->font());
        int rectwidth = backgroundRect.width() - 8; //缩略图宽度：总宽度减去选中框宽度8
        if (textwidth > rectwidth) { //容纳文字像素的宽度大于缩略图宽度
            textwidth = rectwidth - 4;//减少距离：左右各2
        }
        int tempcha = (rectwidth - textwidth > 4) ? (rectwidth - textwidth - 4) : 4;
        int posx = backgroundRect.x() + tempcha;    //剩余天数起始坐标
        //文字背景圆角矩形框弧度，与字号相关
        int radious = 6;
        if (textheight < 14) {
            radious = 4;
        }

        painter->drawRoundedRect(posx, backgroundRect.y() + backgroundRect.height() - textheight - 14,
                                 textwidth, textheight + 2, radious, radious); //Y参数：backgroundRect宽度-文字宽度-14边距

        painter->setPen(QColor(255, 255, 255));
        if (m_Width - textwidth > 0) {
            str = Text.elidedText(str, Qt::ElideRight, textwidth);
        }
        painter->drawText(posx + 3, backgroundRect.y() + backgroundRect.height() - 15, str);//在框中绘制文字，起点位置离最下方15像素
    }

    painter->restore();
}

QSize ThumbnailDelegate::sizeHint(const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
{
    Q_UNUSED(option)
    return m_size;
}

ItemInfo ThumbnailDelegate::itemData(const QModelIndex &index) const
{
    ItemInfo data = index.data(Qt::DisplayRole).value<ItemInfo>();
    data.isSelected = index.data(Qt::UserRole).toBool();
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
            const ItemInfo data = itemData(index);
            bool blast = false;
            if (!blast && rect.contains(pMouseEvent->pos())) {
                emit sigCancelFavorite(index);
            } else if (blast && event->type() == QEvent::MouseButtonPress && rect.contains(pMouseEvent->x(), pMouseEvent->y() + 27)) {
                emit sigCancelFavorite(index);
            }
        }
    }
    return false;
}
