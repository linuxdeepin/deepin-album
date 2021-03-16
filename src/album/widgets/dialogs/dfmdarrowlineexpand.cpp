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
#include "dfmdarrowlineexpand.h"

#include <DFontSizeManager>
#include <DApplicationHelper>
#include <QPainter>
#include <QPainterPath>

DFMDArrowLineExpand::DFMDArrowLineExpand()
{
//    if (headerLine()) {
//        DFontSizeManager::instance()->bind(headerLine(), DFontSizeManager::T6);

//        DPalette pa = DApplicationHelper::instance()->palette(headerLine());
//        pa.setBrush(DPalette::Text, pa.color(DPalette::TextTitle));
//        headerLine()->setPalette(pa);
//        connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this, [ = ] {
//            DPalette pa = DApplicationHelper::instance()->palette(headerLine());
//            pa.setBrush(DPalette::Text, pa.color(DPalette::TextTitle));
//            headerLine()->setPalette(pa);
//        });

//        headerLine()->setLeftMargin(10);
//    }
}

void DFMDArrowLineExpand::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    QRectF bgRect;
    bgRect.setSize(size());
    const QPalette pal = QGuiApplication::palette();//this->palette();
    QColor bgColor = pal.color(QPalette::Background);

    QPainterPath path;
    path.addRoundedRect(bgRect, 8, 8);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillPath(path, bgColor);
    painter.setRenderHint(QPainter::Antialiasing, false);
}
