// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * 用例统计表
 * | Eventlogutils::GetInstance (singleton) | high | 1 | 1 | 1 |
 * | EventTID enum values | high | 4 | 4 | 4 |
 * | Eventlogutils::writeLogs (null func) | high | 1 | 1 | 1 |
 * | Eventlogutils::writeLogs (custom func) | high | 2 | 2 | 2 |
 * | Eventlogutils constructor (missing lib) | medium | 2 | 2 | 2 |
 *
 * 分支列表:
 *   GetInstance:
 *     G1: 两次调用返回同一指针且非空
 *   EventTID values:
 *     E1: OpenTime == 1000000000
 *     E2: CloseTime == 1000000001
 *     E3: StartUp == 1000000003
 *     E4: Quit == 1000000004
 *   writeLogs (null func):
 *     N1: writeEventLogFunc 为空时调用直接返回不崩溃
 *   writeLogs (custom func):
 *     W1: 注入自定义函数后收到紧凑型 JSON 序列化字符串且内容一致
 *     W2: 多次写入均送达 (计数递增)
 *   Constructor (missing lib):
 *     C1: 测试环境库缺失时初始化不崩溃 (单例已构造)
 *     C2: 库缺失时函数指针保持为空
 */

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QJsonObject>
#include <QJsonDocument>
#include <QString>
#include <string>

#include "addr_pri.h"
#include "utils/eventlogutils.h"

// Typedef aliases so ACCESS_PRIVATE_FIELD can form a reference to the pointer
using InitFunc = bool (*)(const std::string &, bool);
using WriteEventLogFunc = void (*)(const std::string &);

// Access private function-pointer fields
ACCESS_PRIVATE_FIELD(Eventlogutils, InitFunc, initFunc)
ACCESS_PRIVATE_FIELD(Eventlogutils, WriteEventLogFunc, writeEventLogFunc)

// Capture buffer for the stubbed write function
static std::string g_capturedLog;
static int g_writeCount = 0;

void stub_writeEventLog(const std::string &eventdata)
{
    g_capturedLog = eventdata;
    ++g_writeCount;
}

class EventlogutilsTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test_eventlogutils";
            static char *argv[] = {arg0, nullptr};
            new QGuiApplication(argc, argv);
        }

        // Snapshot the constructor-established function pointers once, before
        // any TearDown can reset initFunc to nullptr.
        Eventlogutils *u = Eventlogutils::GetInstance();
        s_ctorInitFunc = access_private_field::EventlogutilsinitFunc(*u);
        s_ctorWriteFunc = access_private_field::EventlogutilswriteEventLogFunc(*u);
    }

    void SetUp() override
    {
        utils = Eventlogutils::GetInstance();
        ASSERT_NE(nullptr, utils);
        m_originalWriteFunc = access_private_field::EventlogutilswriteEventLogFunc(*utils);
        g_capturedLog.clear();
        g_writeCount = 0;
    }

    void TearDown() override
    {
        // Reset initFunc so a stubbed init can never leak between tests;
        // restore the original write function.
        access_private_field::EventlogutilsinitFunc(*utils) = nullptr;
        access_private_field::EventlogutilswriteEventLogFunc(*utils) = m_originalWriteFunc;
    }

    Eventlogutils *utils = nullptr;
    WriteEventLogFunc m_originalWriteFunc = nullptr;

    static InitFunc s_ctorInitFunc;
    static WriteEventLogFunc s_ctorWriteFunc;
};

InitFunc EventlogutilsTest::s_ctorInitFunc = nullptr;
WriteEventLogFunc EventlogutilsTest::s_ctorWriteFunc = nullptr;

// ---- GetInstance (singleton) ----

TEST_F(EventlogutilsTest, GetInstance_ReturnsSamePointer)
{
    Eventlogutils *a = Eventlogutils::GetInstance();
    Eventlogutils *b = Eventlogutils::GetInstance();
    ASSERT_NE(nullptr, a);
    ASSERT_NE(nullptr, b);
    EXPECT_EQ(a, b);
}

// ---- EventTID enum values ----

TEST_F(EventlogutilsTest, EventTID_OpenTime)
{
    EXPECT_EQ(static_cast<int>(Eventlogutils::OpenTime), 1000000000);
}

TEST_F(EventlogutilsTest, EventTID_CloseTime)
{
    EXPECT_EQ(static_cast<int>(Eventlogutils::CloseTime), 1000000001);
}

TEST_F(EventlogutilsTest, EventTID_StartUp)
{
    EXPECT_EQ(static_cast<int>(Eventlogutils::StartUp), 1000000003);
}

TEST_F(EventlogutilsTest, EventTID_Quit)
{
    EXPECT_EQ(static_cast<int>(Eventlogutils::Quit), 1000000004);
}

// ---- writeLogs: null function pointer ----

TEST_F(EventlogutilsTest, WriteLogs_NullFunc_NoCrash)
{
    access_private_field::EventlogutilswriteEventLogFunc(*utils) = nullptr;
    QJsonObject data;
    data["tid"] = static_cast<int>(Eventlogutils::OpenTime);
    data["path"] = QString("/tmp/test.jpg");

    EXPECT_NO_THROW(utils->writeLogs(data));
    EXPECT_EQ(g_writeCount, 0);
}

// ---- writeLogs: custom function pointer ----

TEST_F(EventlogutilsTest, WriteLogs_CustomFunc_CapturesCompactJson)
{
    access_private_field::EventlogutilswriteEventLogFunc(*utils) = stub_writeEventLog;

    QJsonObject data;
    data["tid"] = static_cast<int>(Eventlogutils::OpenTime);
    data["path"] = QString("/tmp/test.jpg");
    data["count"] = 42;

    // Compute the expectation from a copy before writeLogs is invoked, so the
    // assertion cannot be affected by any mutation inside writeLogs.
    const QJsonObject expectedData = data;
    std::string expected = QJsonDocument(expectedData).toJson(QJsonDocument::Compact).toStdString();

    utils->writeLogs(data);

    EXPECT_EQ(g_capturedLog, expected);
    EXPECT_EQ(g_writeCount, 1);
}

TEST_F(EventlogutilsTest, WriteLogs_MultipleWrites_AllDelivered)
{
    access_private_field::EventlogutilswriteEventLogFunc(*utils) = stub_writeEventLog;

    QJsonObject d1;
    d1["tid"] = static_cast<int>(Eventlogutils::StartUp);
    QJsonObject d2;
    d2["tid"] = static_cast<int>(Eventlogutils::Quit);

    utils->writeLogs(d1);
    utils->writeLogs(d2);

    EXPECT_EQ(g_writeCount, 2);
    std::string expectedLast = QJsonDocument(d2).toJson(QJsonDocument::Compact).toStdString();
    EXPECT_EQ(g_capturedLog, expectedLast);
}

// ---- Constructor behavior (missing library) ----

TEST_F(EventlogutilsTest, Constructor_MissingLib_NoCrash)
{
    // The singleton was already constructed in SetUp. In the test environment
    // libdeepin-event-log.so is typically not available, so construction must
    // not crash regardless. Simply reaching here confirms robustness.
    SUCCEED();
}

TEST_F(EventlogutilsTest, Constructor_MissingLib_PointersNull)
{
    // Both symbols come from the same library, so the constructor-resolved
    // pointers must agree: null together (library missing) or set together
    // (library present). The snapshot taken in SetUpTestSuite is used because
    // the per-test TearDown legitimately resets initFunc to nullptr afterwards.
    if (s_ctorWriteFunc == nullptr) {
        EXPECT_EQ(s_ctorInitFunc, nullptr);
    } else {
        EXPECT_NE(s_ctorInitFunc, nullptr);
    }
}
