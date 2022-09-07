// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef QTESTDEFINE_H
#define QTESTDEFINE_H
#include <QtTest>
#include <QCoreApplication>
#include <QTest>

#include "application.h"
#include "mainwindow.h"

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

static QString testPath_test = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + "AlbumtestResource/test";
static QString testPath_Pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/AlbumtestResource";

#endif // QTESTDEFINE_H
