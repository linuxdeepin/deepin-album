/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     WangZhengYang <wangzhengyang@uniontech.com>
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

#pragma once

//工具箱里面放一些常用的公共函数

#include <QString>
#include <QPoint>

class QMenu;
class QWidget;

//主界面切换
void clickToAllPictureView();
void clickToTimelineView();
void clickToAlbumView();

//双击标题栏
void dClickTitleBar();

//相册页面操作（会确认并尝试强制切换到相册页面以保证操作生效）

//左侧侧边栏操作
void clickToImportPage();
void clickToDeletePage();
void clickToFavoritePage();
void clickNewAlbumInAlbumPage(const QString &albumName);
void clickToCustomAlbum(int index); //需要先创建自定义相册

//菜单操作

//弹出菜单
QMenu *runContextMenu(QWidget *w, const QPoint &pos);

//执行动作
void runActionFromMenu(QMenu *menu, const QString &actionName);

//获取指针背后的数据类型（即从T*中把T提出来）
template <typename T>
struct PointerTypeGetter {
    using type = T;
    PointerTypeGetter(T t)
    {
        Q_UNUSED(t)
    };
};

template <typename T>
struct PointerTypeGetter<T *> {
    using type = T;
    PointerTypeGetter(T *t)
    {
        Q_UNUSED(t)
    };
};
