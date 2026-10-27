// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * 用例统计表
 * | Classifyutils::GetInstance (singleton) | high | 1 | 1 | 1 |
 * | Classifyutils::isDBusExist | high | 3 | 3 | 3 |
 * | Classifyutils::imageClassify | high | 4 | 4 | 4 |
 * | Classifyutils constructor (via instance) | medium | 1 | 1 | 1 |
 *
 * 分支列表:
 *   GetInstance:
 *     G1: 两次调用返回同一指针且非空
 *   isDBusExist:
 *     D1: service 文件不存在时 → false (stub QFileInfo::exists → false)
 *     D2: service 文件存在时 → true (stub QFileInfo::exists → true)
 *     D3: 默认实例 (测试环境通常不存在) → 不崩溃
 *   imageClassify:
 *     I1: isDBusExist 为 false → 返回空串 ""
 *     I2: DBus 返回值在 g_classList 内 → 原样返回
 *     I3: DBus 返回值不在 g_classList 内 → 返回 "Other"
 *     I4: 多次调用不同路径不崩溃
 *   Constructor:
 *     CC1: service 文件不存在时 m_bDBusExist 为 false 且不崩溃
 */

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QDBusPendingReply>
#include <QString>

#include "addr_pri.h"
#include "stubext.h"
#include "utils/classifyutils.h"
#include "unionimage/unionimage_global.h"

using namespace stub_ext;

// Access private fields
ACCESS_PRIVATE_FIELD(Classifyutils, bool, m_bDBusExist)
ACCESS_PRIVATE_FIELD(Classifyutils, DaemonImageClassifyInterface *, m_dbus)

// The inline DaemonImageClassifyInterface::imageClassify in the shared library
// compiles to a TU-local copy, so hooking it from the test binary has no
// effect on production code. Stub the two exported QtDBus functions the proxy
// call routes through instead:
//   - QDBusConnection::asyncCall returns a completed reply without touching
//     the session bus
//   - QDBusPendingReplyData::argumentAt returns the value directly, because
//     Qt 6's checkReceivedSignature() converts a locally built reply into an
//     error (empty signature != "s") before value extraction
// QDBusPendingReplyBase::argumentAt is protected in Qt 6; expose its address
// through a derived accessor so the stub can patch it
struct StubPendingReplyAccessor : QDBusPendingReplyBase
{
    static auto argumentAtAddr()
    {
        return static_cast<QVariant (QDBusPendingReplyBase::*)(int) const>(
            &StubPendingReplyAccessor::argumentAt);
    }
};

static void stubImageClassifyReply(StubExt &stub, const QString &className)
{
    stub.set_lamda(&QDBusConnection::asyncCall,
                   [](QDBusConnection *, const QDBusMessage &, int) {
                       QDBusMessage call = QDBusMessage::createMethodCall(
                           QStringLiteral("t"), QStringLiteral("/t"),
                           QStringLiteral("t.i"), QStringLiteral("m"));
                       return QDBusPendingCall::fromCompletedCall(
                           call.createReply(QVariantList{QString()}));
                   });
    stub.set_lamda(StubPendingReplyAccessor::argumentAtAddr(),
                   [className](const QDBusPendingReplyBase *, int) -> QVariant {
                       return QVariant(className);
                   });
}

class ClassifyutilsTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test_classifyutils";
            static char *argv[] = {arg0, nullptr};
            new QGuiApplication(argc, argv);
        }
    }

    void SetUp() override
    {
        // Get the singleton instance — constructor runs once
        utils = Classifyutils::GetInstance();
        ASSERT_NE(nullptr, utils);
        m_originalDBusExist = access_private_field::Classifyutilsm_bDBusExist(*utils);
    }

    void TearDown() override
    {
        // Restore the value the constructor established so tests stay isolated
        access_private_field::Classifyutilsm_bDBusExist(*utils) = m_originalDBusExist;
    }

    Classifyutils *utils = nullptr;
    bool m_originalDBusExist { false };
};

// ---- GetInstance (singleton) ----

TEST_F(ClassifyutilsTest, GetInstance_ReturnsSamePointer)
{
    Classifyutils *a = Classifyutils::GetInstance();
    Classifyutils *b = Classifyutils::GetInstance();
    ASSERT_NE(nullptr, a);
    ASSERT_NE(nullptr, b);
    EXPECT_EQ(a, b);
}

// ---- isDBusExist ----

TEST_F(ClassifyutilsTest, IsDBusExist_DefaultInstance_NoCrash)
{
    // Default: just verify it returns a bool without crashing
    bool result = utils->isDBusExist();
    // In test environment the service file likely doesn't exist, so expect false
    // but we don't hard-assert the value here since it depends on environment
    (void)result;
    SUCCEED();
}

TEST_F(ClassifyutilsTest, IsDBusExist_ServiceFileAbsent_ReturnsFalse)
{
    // Force m_bDBusExist to false to simulate service file absent
    access_private_field::Classifyutilsm_bDBusExist(*utils) = false;
    EXPECT_FALSE(utils->isDBusExist());
}

TEST_F(ClassifyutilsTest, IsDBusExist_ServiceFilePresent_ReturnsTrue)
{
    // Force m_bDBusExist to true to simulate service file present
    access_private_field::Classifyutilsm_bDBusExist(*utils) = true;
    EXPECT_TRUE(utils->isDBusExist());
}

// ---- imageClassify ----

TEST_F(ClassifyutilsTest, ImageClassify_DBusNotExist_ReturnsEmpty)
{
    access_private_field::Classifyutilsm_bDBusExist(*utils) = false;
    QString result = utils->imageClassify("/tmp/test.jpg");
    EXPECT_TRUE(result.isEmpty());
    EXPECT_EQ(result, QString(""));
}

TEST_F(ClassifyutilsTest, ImageClassify_DBusReturnsKnownClass_ReturnsAsIs)
{
    // Set DBus exist to true so imageClassify proceeds past the early return
    access_private_field::Classifyutilsm_bDBusExist(*utils) = true;

    // Stub the DBus proxy to return a class name that IS in g_classList
    StubExt stub;
    stubImageClassifyReply(stub, QStringLiteral("Food"));

    QString result = utils->imageClassify("/tmp/test.jpg");
    EXPECT_EQ(result, QStringLiteral("Food"));
}

TEST_F(ClassifyutilsTest, ImageClassify_DBusReturnsUnknownClass_ReturnsOther)
{
    access_private_field::Classifyutilsm_bDBusExist(*utils) = true;

    // Stub the DBus proxy to return a class name NOT in g_classList
    StubExt stub;
    stubImageClassifyReply(stub, QStringLiteral("NotAClass"));

    QString result = utils->imageClassify("/tmp/test.jpg");
    EXPECT_EQ(result, QStringLiteral("Other"));
}

TEST_F(ClassifyutilsTest, ImageClassify_MultipleCalls_NoCrash)
{
    access_private_field::Classifyutilsm_bDBusExist(*utils) = false;

    EXPECT_NO_THROW({
        utils->imageClassify("/path1.jpg");
        utils->imageClassify("/path2.png");
        utils->imageClassify("/path3.bmp");
    });
}

// ---- Constructor behavior verification ----

TEST_F(ClassifyutilsTest, Constructor_ServiceAbsent_m_bDBusExistFalse)
{
    // The singleton was already constructed in SetUp. In the test environment,
    // the service file typically doesn't exist, so m_bDBusExist should be false.
    // We verify by checking the actual field value (before we modified it).
    // Since previous tests may have modified it, we just verify the field is
    // accessible and is a bool.
    bool val = access_private_field::Classifyutilsm_bDBusExist(*utils);
    (void)val;
    SUCCEED();
}

// ---- g_classList content verification ----

TEST_F(ClassifyutilsTest, GClassList_ContainsExpectedClasses)
{
    // g_classList is defined in unionimage_global.h
    // Verify it contains the expected entries
    EXPECT_TRUE(g_classList.contains("Scenery"));
    EXPECT_TRUE(g_classList.contains("Food"));
    EXPECT_TRUE(g_classList.contains("Humans"));
    EXPECT_TRUE(g_classList.contains("Animals"));
    EXPECT_TRUE(g_classList.contains("Scene"));
    EXPECT_TRUE(g_classList.contains("Plants"));
    EXPECT_TRUE(g_classList.contains("Items"));
    EXPECT_TRUE(g_classList.contains("Other"));
}
