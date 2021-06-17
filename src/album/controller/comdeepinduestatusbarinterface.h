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

#ifndef COMDEEPINDUESTATUSBARINTERFACE_H
#define COMDEEPINDUESTATUSBARINTERFACE_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

//这个类是利用隔壁部门提供的XML文件，使用qdbusxml2cpp自动生成，然后在此基础上改装成单例

class ComDeepinDueStatusbarInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "com.deepin.due.statusbar"; }

public:
    static ComDeepinDueStatusbarInterface &instance() {
        static ComDeepinDueStatusbarInterface comDeepinDueStatusbarInterface;
        return comDeepinDueStatusbarInterface;
    }
    ~ComDeepinDueStatusbarInterface();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<uint> height()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("height"), argumentList);
    }

    inline QDBusPendingReply<> setVisible(bool visible)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(visible);
        return asyncCallWithArgumentList(QStringLiteral("setVisible"), argumentList);
    }

    inline QDBusPendingReply<bool> visible()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("visible"), argumentList);
    }

Q_SIGNALS: // SIGNALS
    void heightChanged();
    void visibleChanged();

private:
    ComDeepinDueStatusbarInterface(QObject *parent = nullptr);
};

namespace com {
  namespace deepin {
    namespace due {
      typedef ::ComDeepinDueStatusbarInterface statusbar;
    }
  }
}
#endif
