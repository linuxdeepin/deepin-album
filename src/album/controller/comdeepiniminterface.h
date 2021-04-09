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

#ifndef COMDEEPINIMINTERFACE_H
#define COMDEEPINIMINTERFACE_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

/*
 * Proxy class for interface com.deepin.im
 */
class ComDeepinImInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "com.deepin.im"; }

public:
    static ComDeepinImInterface &instance() {
        static ComDeepinImInterface comDeepinImInterface;
        return comDeepinImInterface;
    }
    ~ComDeepinImInterface();

    Q_PROPERTY(QRect geometry READ geometry)
    inline QRect geometry() const
    { return qvariant_cast< QRect >(property("geometry")); }

    Q_PROPERTY(bool imActive READ imActive WRITE setImActive)
    inline bool imActive() const
    { return qvariant_cast< bool >(property("imActive")); }
    inline void setImActive(bool value)
    { setProperty("imActive", QVariant::fromValue(value)); }

    Q_PROPERTY(bool imSignalLock READ imSignalLock WRITE setImSignalLock)
    inline bool imSignalLock() const
    { return qvariant_cast< bool >(property("imSignalLock")); }
    inline void setImSignalLock(bool value)
    { setProperty("imSignalLock", QVariant::fromValue(value)); }

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<> setKeyboardHeight(int h)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(h);
        return asyncCallWithArgumentList(QStringLiteral("setKeyboardHeight"), argumentList);
    }

Q_SIGNALS: // SIGNALS
    void geometryChanged(const QRect &rect);
    void imActiveChanged(bool is);
    void imSignalLockChanged(bool is);

private:
    ComDeepinImInterface(QObject *parent = nullptr);
};

namespace com {
  namespace deepin {
    typedef ::ComDeepinImInterface im;
  }
}

#endif // COMDEEPINIMINTERFACE_H
