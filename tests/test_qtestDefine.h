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
#ifndef QTESTDEFINE_H
#define QTESTDEFINE_H
#include <QtTest>
#include <QCoreApplication>
#include <QTest>

#include "application.h"
#include "mainwindow.h"

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

static QString testPath_test = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + "test";
static QString testPath_Pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

#endif // QTESTDEFINE_H
