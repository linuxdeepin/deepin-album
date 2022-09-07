// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CONFIGSETTER_H
#define CONFIGSETTER_H

#include <QObject>
#include <QSettings>
#include <QMutex>

class ConfigSetter : public QObject
{
    Q_OBJECT
public:
    static ConfigSetter *instance();
    void setValue(const QString &group, const QString &key,
                  const QVariant &value);
    QVariant value(const QString &group, const QString &key,
                   const QVariant &defaultValue = QVariant());
//    QStringList keys(const QString &group);

signals:
    void valueChanged(const QString &group, const QString &key,
                      const QVariant &value);

private:
    explicit ConfigSetter(QObject *parent = nullptr);

private:
    static ConfigSetter *m_setter;
    QSettings *m_settings;
    QMutex m_mutex;
};

#endif // CONFIGSETTER_H
