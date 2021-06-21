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
#ifndef ALBUMGLOBAL_H
#define ALBUMGLOBAL_H

#include <QPixmap>
#include <QListWidget>
#include <QListWidgetItem>
#include <QListView>
#include <QList>
#include <QWidgetAction>
#include <QPixmap>
#include <QIcon>
#include <QFileInfo>
#include <QSize>
#include <QStandardItemModel>
#include <QBuffer>
#include <QMouseEvent>
#include <QPointer>

struct ItemInfo {
    QString name = "";
    QString path = "";
    int imgWidth = 0;
    int imgHeight = 0;
    QString remainDays = "30天";
    bool isSelected;
    QPixmap image = QPixmap();
    bool bNotSupportedOrDamaged = false;


    friend bool operator== (const ItemInfo &left, const ItemInfo &right)
    {
        if (left.image == right.image)
            return true;
        return false;
    }
};

Q_DECLARE_METATYPE(ItemInfo)
#endif // ALBUMGLOBAL_H
