// Copyright (C) 2020 ~ 2020 Uniontech Technology Co., Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <QDebug>
#include <QGuiApplication>
#include <DAppLoader>

DQUICK_USE_NAMESPACE

static int qArgc = 0;
static char **qArgv = nullptr;

class tst_DAppLoader : public testing::Test
{
public:
    static void SetUpTestCase()
    {
        qDebug() << "*****************" << __FUNCTION__;
    }
    static void TearDownTestCase()
    {
        qDebug() << "*****************" << __FUNCTION__;
    }
    virtual void SetUp();
    virtual void TearDown();
};
void tst_DAppLoader::SetUp()
{
    qDebug() << "*****************" << __FUNCTION__;
}
void tst_DAppLoader::TearDown()
{
    qDebug() << "*****************" << __FUNCTION__;
}

TEST_F(tst_DAppLoader, DAppLoader_pluginPaths)
{
    qDebug() << "Lib Name: " << LIB_NAME
             << "Lib Path: " << LIB_PATH;

    // Qt6 版 DtkDeclarative 的 DAppLoader 不再提供 createApplication，
    // 此处验证 DAppLoader 基本接口（插件路径管理）可用。
    DAppLoader appLoader(LIB_NAME, LIB_PATH);
    appLoader.addPluginPath(LIB_PATH);

    EXPECT_TRUE(appLoader.pluginPaths().contains(LIB_PATH));
    EXPECT_NE(DAppLoader::instance(), nullptr);
}

int main(int argc, char *argv[])
{
    qArgc = argc;
    qArgv = argv;

    // 无显示环境（如 CI）时回退到 offscreen 平台
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }

    QGuiApplication app(qArgc, qArgv);
    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
