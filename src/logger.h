// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ALBUM_MLOGGER_H
#define ALBUM_MLOGGER_H

#include <QObject>
#include <dtkcore_global.h>

// 动态日志规则管理：从 DConfig(org.deepin.album) 读取 log_rules 并监听变更，
// 实现调试模式下日志级别的运行中动态调整（参考 deepin-reader 的 MLogger 实现）
DCORE_BEGIN_NAMESPACE
class DConfig;
DCORE_END_NAMESPACE

class MLogger : public QObject
{
    Q_OBJECT

public:
    explicit MLogger(QObject *parent = nullptr);
    ~MLogger();

private:
    void setRules(const QString &rules);
    void appendRules(const QString &rules);

private:
    QString m_rules;
    Dtk::Core::DConfig *m_config = nullptr;
};

#endif   // ALBUM_MLOGGER_H
