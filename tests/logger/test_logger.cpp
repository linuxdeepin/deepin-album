// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * 用例统计表
 * | MLogger::MLogger (constructor) | high | 4 | 4 | 5 |
 * | MLogger::setRules | high | 3 | 2 | 3 |
 * | MLogger::appendRules | high | 4 | 3 | 4 |
 * | MLogger::~MLogger (destructor) | mid | 1 | 1 | 1 |
 * | DConfig valueChanged signal | mid | 2 | 1 | 2 |
 *
 * 分支列表 (constructor):
 *   B1: QT_LOGGING_RULES env var set → read into m_rules, then qunsetenv
 *   B2: QT_LOGGING_RULES env var not set → m_rules starts empty
 *   B3: DConfig log_rules has value → appended to m_rules via appendRules
 *   B4: DConfig log_rules empty → no additional rules appended
 *   B5: QLoggingCategory filter rules applied after construction (setRules called)
 *
 * 分支列表 (setRules):
 *   B1: rules with semicolons → converted to newlines in m_rules
 *   B2: rules without semicolons → used as-is
 *   B3: empty rules → m_rules becomes empty
 *
 * 分支列表 (appendRules):
 *   B1: new rules not in m_rules → appended with newline separator
 *   B2: rules already in m_rules → deduplicated, not appended
 *   B3: empty rules → return immediately, no change
 *   B4: semicolon-separated mixed rules → only new ones appended
 *
 * 分支列表 (destructor):
 *   B1: m_config non-null → deleteLater called safely (no crash)
 *
 * 分支列表 (DConfig valueChanged):
 *   B1: key == "log_rules" → setRules called with new DConfig value
 *   B2: key != "log_rules" → no change to m_rules
 */

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QString>
#include <QByteArray>
#include <memory>

#include <DConfig>

#include "stubext.h"
#include "addr_pri.h"
#include "logger.h"

using namespace stub_ext;

DCORE_USE_NAMESPACE

// Access private members without "#define private public" (ODR-safe).
// Reference: tests/movieservice, tests/fileinotify use the same addr_pri.h approach.
ACCESS_PRIVATE_FUN(MLogger, void(const QString &), setRules)
ACCESS_PRIVATE_FUN(MLogger, void(const QString &), appendRules)
ACCESS_PRIVATE_FIELD(MLogger, QString, m_rules)
ACCESS_PRIVATE_FIELD(MLogger, DConfig *, m_config)

// Function-pointer alias to disambiguate the DConfig::create overload that
// takes (appId, name, subpath, parent).  logger.cpp calls this two-argument
// form, relying on default values for the remaining parameters.
using DConfigCreateFunc =
    DConfig *(*)(const QString &, const QString &, const QString &, QObject *);

class MLoggerTest : public ::testing::Test
{
protected:
    StubExt stub;

    // Controlled return value for DConfig::value("log_rules").
    // Tests change this variable to drive the stub's behaviour.
    QString dconfigRules;

    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test_logger";
            static char *argv[] = {arg0, nullptr};
            new QGuiApplication(argc, argv);
        }
    }

    void SetUp() override
    {
        // Start from a clean environment.
        qunsetenv("QT_LOGGING_RULES");

        // Stub DConfig::create — return a fresh DConfig each call so that
        // multiple MLogger instances do not share (and double-delete) the
        // same object.  The public constructor yields a non-null (possibly
        // invalid) DConfig even without the schema installed.
        stub.set_lamda(static_cast<DConfigCreateFunc>(&DConfig::create),
            [](const QString &, const QString &, const QString &,
               QObject *) -> DConfig * {
                return new DConfig("org.deepin.album");
            });

        // Stub DConfig::value — return the fixture-controlled log_rules
        // string for the "log_rules" key, empty QVariant otherwise.
        stub.set_lamda(ADDR(DConfig, value),
            [this](DConfig *, const QString &key,
                   const QVariant &) -> QVariant {
                if (key == "log_rules")
                    return QVariant(dconfigRules);
                return QVariant();
            });
    }

    void TearDown() override
    {
        stub.clear();
        qunsetenv("QT_LOGGING_RULES");
    }
};

// ===========================================================================
// Constructor tests
// ===========================================================================

// B1: QT_LOGGING_RULES env var set → read into m_rules, then cleared
TEST_F(MLoggerTest, Constructor_ReadsAndClearsEnvLoggingRules)
{
    // Arrange
    qputenv("QT_LOGGING_RULES", "env.category=true");

    // Act
    auto logger = std::make_unique<MLogger>();

    // Assert — environment variable is cleared after construction
    EXPECT_EQ(qgetenv("QT_LOGGING_RULES"), QByteArray());
    // Assert — the env var value is present in m_rules
    QString rules = access_private_field::MLoggerm_rules(*logger);
    EXPECT_TRUE(rules.contains("env.category=true"));
}

// B2: QT_LOGGING_RULES env var not set → m_rules starts with only DConfig rules
TEST_F(MLoggerTest, Constructor_NoEnvVar_UsesOnlyDConfigRules)
{
    // Arrange
    qunsetenv("QT_LOGGING_RULES");
    dconfigRules = "dconfig.category=true";

    // Act
    auto logger = std::make_unique<MLogger>();

    // Assert
    QString rules = access_private_field::MLoggerm_rules(*logger);
    EXPECT_TRUE(rules.contains("dconfig.category=true"));
}

// B3: DConfig log_rules has value → merged with env rules in m_rules
TEST_F(MLoggerTest, Constructor_MergesDConfigRulesWithEnvRules)
{
    // Arrange
    qputenv("QT_LOGGING_RULES", "env.rule=true");
    dconfigRules = "dconfig.rule=true";

    // Act
    auto logger = std::make_unique<MLogger>();

    // Assert — both env and DConfig rules are present
    QString rules = access_private_field::MLoggerm_rules(*logger);
    EXPECT_TRUE(rules.contains("env.rule=true"));
    EXPECT_TRUE(rules.contains("dconfig.rule=true"));
}

// B4: DConfig log_rules empty → only env rules in m_rules
TEST_F(MLoggerTest, Constructor_EmptyDConfigRules_OnlyEnvRules)
{
    // Arrange
    qputenv("QT_LOGGING_RULES", "env.only=true");
    dconfigRules = "";

    // Act
    auto logger = std::make_unique<MLogger>();

    // Assert — only the env rule is present
    QString rules = access_private_field::MLoggerm_rules(*logger);
    EXPECT_TRUE(rules.contains("env.only=true"));
}

// ===========================================================================
// setRules tests
// ===========================================================================

// B1: rules with semicolons → converted to newlines in m_rules
TEST_F(MLoggerTest, SetRules_ConvertsSemicolonsToNewlines)
{
    // Arrange
    auto logger = std::make_unique<MLogger>();

    // Act
    call_private_fun::MLoggersetRules(*logger, "a.b=true;c.d=false");

    // Assert
    QString rules = access_private_field::MLoggerm_rules(*logger);
    EXPECT_EQ(rules, QString("a.b=true\nc.d=false"));
}

// B2: rules without semicolons → used as-is
TEST_F(MLoggerTest, SetRules_NoSemicolons_UsedAsIs)
{
    // Arrange
    auto logger = std::make_unique<MLogger>();

    // Act
    call_private_fun::MLoggersetRules(*logger, "single.rule=true");

    // Assert
    QString rules = access_private_field::MLoggerm_rules(*logger);
    EXPECT_EQ(rules, QString("single.rule=true"));
}

// B3: empty rules → m_rules becomes empty
TEST_F(MLoggerTest, SetRules_EmptyRules_ClearsRules)
{
    // Arrange
    auto logger = std::make_unique<MLogger>();

    // Act
    call_private_fun::MLoggersetRules(*logger, "");

    // Assert
    QString rules = access_private_field::MLoggerm_rules(*logger);
    EXPECT_TRUE(rules.isEmpty());
}

// ===========================================================================
// appendRules tests
// ===========================================================================

// B1: new rules not in m_rules → appended with newline separator
TEST_F(MLoggerTest, AppendRules_NewRules_AppendedWithNewline)
{
    // Arrange
    auto logger = std::make_unique<MLogger>();
    access_private_field::MLoggerm_rules(*logger) = "existing.rule=true";

    // Act
    call_private_fun::MLoggerappendRules(*logger, "new.rule=false");

    // Assert
    QString rules = access_private_field::MLoggerm_rules(*logger);
    EXPECT_EQ(rules, QString("existing.rule=true\nnew.rule=false"));
}

// B2: rules already in m_rules → deduplicated, not appended
TEST_F(MLoggerTest, AppendRules_ExistingRules_Deduplicated)
{
    // Arrange
    auto logger = std::make_unique<MLogger>();
    access_private_field::MLoggerm_rules(*logger) = "existing.rule=true";

    // Act
    call_private_fun::MLoggerappendRules(*logger, "existing.rule=true");

    // Assert — no duplicate added
    QString rules = access_private_field::MLoggerm_rules(*logger);
    EXPECT_EQ(rules, QString("existing.rule=true"));
}

// B3: empty rules → return immediately, no change
TEST_F(MLoggerTest, AppendRules_EmptyRules_NoChange)
{
    // Arrange
    auto logger = std::make_unique<MLogger>();
    access_private_field::MLoggerm_rules(*logger) = "existing.rule=true";

    // Act
    call_private_fun::MLoggerappendRules(*logger, "");

    // Assert — unchanged
    QString rules = access_private_field::MLoggerm_rules(*logger);
    EXPECT_EQ(rules, QString("existing.rule=true"));
}

// B4: semicolon-separated mixed rules → only new ones appended
TEST_F(MLoggerTest, AppendRules_MixedRules_OnlyNewAppended)
{
    // Arrange
    auto logger = std::make_unique<MLogger>();
    access_private_field::MLoggerm_rules(*logger) = "existing.rule=true";

    // Act — one rule already exists, one is new
    call_private_fun::MLoggerappendRules(*logger,
                                         "existing.rule=true;new.rule=false");

    // Assert
    QString rules = access_private_field::MLoggerm_rules(*logger);
    EXPECT_TRUE(rules.contains("existing.rule=true"));
    EXPECT_TRUE(rules.contains("new.rule=false"));
    // existing.rule=true appears only once (deduplicated)
    EXPECT_EQ(rules.count("existing.rule=true"), 1);
}

// ===========================================================================
// DConfig valueChanged signal tests
// ===========================================================================

// B1: key == "log_rules" → setRules called with new DConfig value
TEST_F(MLoggerTest, DConfigValueChanged_LogRulesKey_UpdatesRules)
{
    // Arrange — construct with known initial rules
    qputenv("QT_LOGGING_RULES", "initial.rule=true");
    dconfigRules = "initial.dconfig=true";
    auto logger = std::make_unique<MLogger>();
    QString rulesBefore = access_private_field::MLoggerm_rules(*logger);

    // Change the value that the DConfig::value stub will return
    dconfigRules = "updated.dconfig=false";

    // Act — emit valueChanged("log_rules") on the internal DConfig
    DConfig *config = access_private_field::MLoggerm_config(*logger);
    ASSERT_NE(config, nullptr);
    QMetaObject::invokeMethod(config, "valueChanged",
                              Qt::DirectConnection,
                              Q_ARG(QString, "log_rules"));

    // Assert — m_rules reflects the updated DConfig value
    QString rulesAfter = access_private_field::MLoggerm_rules(*logger);
    EXPECT_NE(rulesAfter, rulesBefore);
    EXPECT_TRUE(rulesAfter.contains("updated.dconfig=false"));
}

// B2: key != "log_rules" → no change to m_rules
TEST_F(MLoggerTest, DConfigValueChanged_OtherKey_NoChange)
{
    // Arrange
    auto logger = std::make_unique<MLogger>();
    QString rulesBefore = access_private_field::MLoggerm_rules(*logger);

    // Act — emit valueChanged with a key other than "log_rules"
    DConfig *config = access_private_field::MLoggerm_config(*logger);
    ASSERT_NE(config, nullptr);
    QMetaObject::invokeMethod(config, "valueChanged",
                              Qt::DirectConnection,
                              Q_ARG(QString, "other_key"));

    // Assert — m_rules unchanged
    QString rulesAfter = access_private_field::MLoggerm_rules(*logger);
    EXPECT_EQ(rulesAfter, rulesBefore);
}

// ===========================================================================
// Destructor tests
// ===========================================================================

// B1: m_config non-null → deleteLater called safely (no crash)
TEST_F(MLoggerTest, Destructor_SafelyReleasesDConfig)
{
    // Arrange
    auto logger = std::make_unique<MLogger>();
    DConfig *config = access_private_field::MLoggerm_config(*logger);
    ASSERT_NE(config, nullptr);

    // Act & Assert — destructor must not crash
    logger.reset();
    SUCCEED();
}
