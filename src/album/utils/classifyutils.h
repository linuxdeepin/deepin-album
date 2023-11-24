// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CLASSIFYUTILS_H
#define CLASSIFYUTILS_H
#include <QString>
#include <QtDBus/QtDBus>
/*
 * Proxy class for interface com.deepin.logviewer
 */
class DaemonImageClassifyInterface : public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "com.deepin.imageclassify"; }
    static inline const char *staticServiceName()
    { return "com.deepin.imageclassify"; }
    static inline const char *staticObjectPath()
    { return "/com/deepin/imageclassify"; }

public:
    DaemonImageClassifyInterface(QObject *parent = nullptr)
        : QDBusAbstractInterface(staticServiceName(), staticObjectPath(), staticInterfaceName(), QDBusConnection::systemBus(), parent)
    {

    }

    ~DaemonImageClassifyInterface()
    {

    }

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<QString> imageClassify(const QString &filePath)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(filePath);
        return asyncCallWithArgumentList(QStringLiteral("imageClassify"), argumentList);
    }

Q_SIGNALS: // SIGNALS
};

namespace com {
namespace deepin {
typedef ::DaemonImageClassifyInterface imageclassify;
}
} // namespace com

class Classifyutils : public QObject
{
    Q_OBJECT

public:
    static Classifyutils *GetInstance();
    QString imageClassify(const QString &path);
    bool isDBusExist();
private :
    static Classifyutils *m_pInstance;
    Classifyutils();

    bool m_bDBusExist{false};
    DaemonImageClassifyInterface *m_dbus {nullptr};
};

#endif // CLASSIFYUTILS_H
