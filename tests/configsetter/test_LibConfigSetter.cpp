// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QSettings>
#include <QVariant>
#include <QString>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QMutex>

#include "stubext.h"
#include "addr_pri.h"
#include "configsetter.h"

using namespace stub_ext;

ACCESS_PRIVATE_FIELD(LibConfigSetter, QSettings *, m_settings)

class LibConfigSetterTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QCoreApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test";
            static char *argv[] = {arg0, nullptr};
            new QCoreApplication(argc, argv);
        }
    }

    void SetUp() override
    {
        setter = LibConfigSetter::instance();
        ASSERT_NE(setter, nullptr);

        tempFile.reset(new QTemporaryFile);
        ASSERT_TRUE(tempFile->open());
        QString path = tempFile->fileName();
        tempFile->close();

        testSettings = new QSettings(path, QSettings::IniFormat);
        access_private_field::LibConfigSetterm_settings(*setter) = testSettings;
    }

    void TearDown() override
    {
        access_private_field::LibConfigSetterm_settings(*setter) = nullptr;
        delete testSettings;
        testSettings = nullptr;
    }

    LibConfigSetter *setter = nullptr;
    QSettings *testSettings = nullptr;
    std::unique_ptr<QTemporaryFile> tempFile;
};

// ---- setValue + value round-trip ----

TEST_F(LibConfigSetterTest, SetValue_ThenValueReturnsSame)
{
    setter->setValue("group1", "key1", 42);
    QVariant result = setter->value("group1", "key1");
    EXPECT_EQ(result.toInt(), 42);
}

TEST_F(LibConfigSetterTest, SetValue_StringValue)
{
    setter->setValue("groupA", "name", QString("hello"));
    EXPECT_EQ(setter->value("groupA", "name").toString(), QString("hello"));
}

TEST_F(LibConfigSetterTest, SetValue_BoolValue)
{
    setter->setValue("groupB", "flag", true);
    EXPECT_TRUE(setter->value("groupB", "flag").toBool());
}

TEST_F(LibConfigSetterTest, SetValue_OverwritesExistingKey)
{
    setter->setValue("g", "k", 1);
    setter->setValue("g", "k", 99);
    EXPECT_EQ(setter->value("g", "k").toInt(), 99);
}

TEST_F(LibConfigSetterTest, setValue_EmitsValueChangedSignal)
{
    QSignalSpy spy(setter, &LibConfigSetter::valueChanged);
    setter->setValue("grp", "key", "val");
    EXPECT_EQ(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toString(), QString("grp"));
    EXPECT_EQ(args.at(1).toString(), QString("key"));
    EXPECT_EQ(args.at(2).toString(), QString("val"));
}

// ---- value ----

TEST_F(LibConfigSetterTest, Value_NonExistentKeyReturnsDefault)
{
    QVariant result = setter->value("group1", "nokey", 123);
    EXPECT_EQ(result.toInt(), 123);
}

TEST_F(LibConfigSetterTest, Value_EmptyGroup)
{
    setter->setValue("", "emptyGroupKey", "data");
    EXPECT_EQ(setter->value("", "emptyGroupKey").toString(), QString("data"));
}

TEST_F(LibConfigSetterTest, Value_DifferentGroupsAreIndependent)
{
    setter->setValue("g1", "k", 1);
    setter->setValue("g2", "k", 2);
    EXPECT_EQ(setter->value("g1", "k").toInt(), 1);
    EXPECT_EQ(setter->value("g2", "k").toInt(), 2);
}

// ---- contains ----

TEST_F(LibConfigSetterTest, Contains_ReturnsTrueForExisting)
{
    setter->setValue("group1", "key1", "val");
    EXPECT_TRUE(setter->contains("group1", "key1"));
}

TEST_F(LibConfigSetterTest, Contains_ReturnsFalseForNonExistent)
{
    EXPECT_FALSE(setter->contains("group1", "nonexistent"));
}

TEST_F(LibConfigSetterTest, Contains_DoesNotFindKeyInWrongGroup)
{
    setter->setValue("g1", "k", 1);
    EXPECT_TRUE(setter->contains("g1", "k"));
    EXPECT_FALSE(setter->contains("g2", "k"));
}

TEST_F(LibConfigSetterTest, Contains_EmptyGroup)
{
    setter->setValue("", "key", "val");
    EXPECT_TRUE(setter->contains("", "key"));
}
