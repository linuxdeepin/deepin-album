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
#ifndef FILEINOTIFYGROUP_H
#define FILEINOTIFYGROUP_H

#include <QObject>

class FileInotify;

class FileInotifyGroup : public QObject
{
    Q_OBJECT
public:
    explicit FileInotifyGroup(QObject *parent = nullptr);
    ~FileInotifyGroup() = default;

    //添加：path下的改动会实时同步到对应的album中
    void startWatch(const QString &path, const QString &album);
};

#endif // FILEINOTIFYGROUP_H
