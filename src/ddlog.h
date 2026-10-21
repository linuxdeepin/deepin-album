// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DDLOG_H
#define DDLOG_H

#include <QDebug>
#include <QLoggingCategory>

// 全应用统一日志分类：调试模式(deepin-debug-config)通过 DConfig log_rules
// 按 org.deepin.album 前缀动态过滤，便于按级别开关
inline Q_LOGGING_CATEGORY(albumLog, "org.deepin.album")

#endif   // DDLOG_H
