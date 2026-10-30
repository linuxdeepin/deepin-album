// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QApplication>
#include <QSignalSpy>
#include <QStandardItem>
#include <DCheckBox>
#include <DLabel>
#include <DCommandLinkButton>
#include <DGuiApplicationHelper>
#include <dpalettehelper.h>
#include <memory>

#include "widgets/thumbnail/timelinedatewidget.h"

#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

// Helper to find a child by objectName
template <typename T>
T *findChildByObjectName(QObject *parent, const QString &name)
{
    return parent->findChild<T *>(name);
}

class TimeLineDateWidgetTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test";
            static char *argv[] = {arg0, nullptr};
            new QApplication(argc, argv);
        }
    }

    static void TearDownTestSuite() {}

    StubExt stub;
    std::unique_ptr<QStandardItem> m_item;
    std::unique_ptr<TimeLineDateWidget> m_widget;
    QString m_time = "2026-01-01";
    QString m_num = "10 photos";

    void SetUp() override
    {
        stub.clear();
        m_item = std::make_unique<QStandardItem>();
        m_widget = std::make_unique<TimeLineDateWidget>(m_item.get(), m_time, m_num);
    }

    void TearDown() override
    {
        m_widget.reset();
        m_item.reset();
        stub.clear();
    }
};

class ImportTimeLineDateWidgetTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test";
            static char *argv[] = {arg0, nullptr};
            new QApplication(argc, argv);
        }
    }

    static void TearDownTestSuite() {}

    StubExt stub;
    std::unique_ptr<QStandardItem> m_item;
    std::unique_ptr<importTimeLineDateWidget> m_widget;
    QString m_time = "2026-06-15";
    QString m_num = "5 photos";

    void SetUp() override
    {
        stub.clear();
        m_item = std::make_unique<QStandardItem>();
        m_widget = std::make_unique<importTimeLineDateWidget>(m_item.get(), m_time, m_num);
    }

    void TearDown() override
    {
        m_widget.reset();
        m_item.reset();
        stub.clear();
    }
};

// ===== TimeLineDateWidget =====

// --- Constructor ---

TEST_F(TimeLineDateWidgetTest, Constructor_ObjectNameAndAccessibleName)
{
    EXPECT_EQ(m_widget->objectName(), "TimeLineDateWidget");
    EXPECT_EQ(m_widget->accessibleName(), "TimeLineDateWidget");
}

TEST_F(TimeLineDateWidgetTest, Constructor_DateTextSet)
{
    auto *dateLabel = findChildByObjectName<DLabel>(m_widget.get(), QString());
    // m_pDate doesn't have an objectName set, find by text
    auto labels = m_widget->findChildren<DLabel *>();
    bool foundDate = false;
    for (auto *label : labels) {
        if (label->text() == m_time) {
            foundDate = true;
            break;
        }
    }
    EXPECT_TRUE(foundDate);
}

TEST_F(TimeLineDateWidgetTest, Constructor_NumCheckBoxTextSet)
{
    auto *checkBox = m_widget->findChild<DCheckBox *>("PNumCheckBox");
    ASSERT_NE(checkBox, nullptr);
    EXPECT_EQ(checkBox->text(), m_num);
}

TEST_F(TimeLineDateWidgetTest, Constructor_CheckBoxInitiallyHidden)
{
    auto *checkBox = m_widget->findChild<DCheckBox *>("PNumCheckBox");
    ASSERT_NE(checkBox, nullptr);
    EXPECT_FALSE(checkBox->isVisible());
}

TEST_F(TimeLineDateWidgetTest, Constructor_ChooseBtnInitiallyHidden)
{
    auto *btn = m_widget->findChild<DCommandLinkButton *>("ChooseBtn");
    ASSERT_NE(btn, nullptr);
    EXPECT_FALSE(btn->isVisible());
}

// --- onShowCheckBox ---

TEST_F(TimeLineDateWidgetTest, OnShowCheckBox_True_ShowsCheckBox)
{
    m_widget->onShowCheckBox(true);
    auto *checkBox = m_widget->findChild<DCheckBox *>("PNumCheckBox");
    ASSERT_NE(checkBox, nullptr);
    EXPECT_FALSE(checkBox->isHidden());
}

TEST_F(TimeLineDateWidgetTest, OnShowCheckBox_False_HidesCheckBox)
{
    m_widget->onShowCheckBox(true);
    m_widget->onShowCheckBox(false);
    auto *checkBox = m_widget->findChild<DCheckBox *>("PNumCheckBox");
    ASSERT_NE(checkBox, nullptr);
    EXPECT_FALSE(checkBox->isVisible());
}

// --- onCheckBoxCliked ---

TEST_F(TimeLineDateWidgetTest, OnCheckBoxCliked_EmitsSignal)
{
    QSignalSpy spy(m_widget.get(), &TimeLineDateWidget::sigIsSelectCurrentDatePic);

    auto *checkBox = m_widget->findChild<DCheckBox *>("PNumCheckBox");
    ASSERT_NE(checkBox, nullptr);
    checkBox->setChecked(true);
    m_widget->onCheckBoxCliked();

    ASSERT_EQ(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    EXPECT_TRUE(args.at(0).toBool());
    EXPECT_EQ(args.at(1).value<QStandardItem *>(), m_item.get());
}

TEST_F(TimeLineDateWidgetTest, OnCheckBoxCliked_Unchecked_EmitsFalse)
{
    QSignalSpy spy(m_widget.get(), &TimeLineDateWidget::sigIsSelectCurrentDatePic);

    auto *checkBox = m_widget->findChild<DCheckBox *>("PNumCheckBox");
    ASSERT_NE(checkBox, nullptr);
    checkBox->setChecked(false);
    m_widget->onCheckBoxCliked();

    ASSERT_EQ(spy.count(), 1);
    EXPECT_FALSE(spy.takeFirst().at(0).toBool());
}

// --- onTimeLinePicSelectAll ---

TEST_F(TimeLineDateWidgetTest, OnTimeLinePicSelectAll_True)
{
    m_widget->onTimeLinePicSelectAll(true);
    auto *checkBox = m_widget->findChild<DCheckBox *>("PNumCheckBox");
    ASSERT_NE(checkBox, nullptr);
    EXPECT_TRUE(checkBox->isChecked());
}

TEST_F(TimeLineDateWidgetTest, OnTimeLinePicSelectAll_False)
{
    m_widget->onTimeLinePicSelectAll(true);
    m_widget->onTimeLinePicSelectAll(false);
    auto *checkBox = m_widget->findChild<DCheckBox *>("PNumCheckBox");
    ASSERT_NE(checkBox, nullptr);
    EXPECT_FALSE(checkBox->isChecked());
}

// --- onGetBtnStatus ---

TEST_F(TimeLineDateWidgetTest, OnGetBtnStatus_ReturnsButtonText)
{
    QString status = m_widget->onGetBtnStatus();
    auto *btn = m_widget->findChild<DCommandLinkButton *>("ChooseBtn");
    ASSERT_NE(btn, nullptr);
    EXPECT_EQ(status, btn->text());
}

// --- onThemeChanged ---

TEST_F(TimeLineDateWidgetTest, OnThemeChanged_LightType_NoCrash)
{
    m_widget->onThemeChanged(DGuiApplicationHelper::LightType);
    SUCCEED();
}

TEST_F(TimeLineDateWidgetTest, OnThemeChanged_DarkType_NoCrash)
{
    m_widget->onThemeChanged(DGuiApplicationHelper::DarkType);
    SUCCEED();
}

TEST_F(TimeLineDateWidgetTest, OnThemeChanged_LightType_PaletteApplied)
{
    m_widget->onThemeChanged(DGuiApplicationHelper::LightType);
    auto *checkBox = m_widget->findChild<DCheckBox *>("PNumCheckBox");
    ASSERT_NE(checkBox, nullptr);
    DPalette pal = DPaletteHelper::instance()->palette(checkBox);
    EXPECT_EQ(pal.color(DPalette::Text), lightTextColor);
}

TEST_F(TimeLineDateWidgetTest, OnThemeChanged_DarkType_PaletteApplied)
{
    m_widget->onThemeChanged(DGuiApplicationHelper::DarkType);
    auto *checkBox = m_widget->findChild<DCheckBox *>("PNumCheckBox");
    ASSERT_NE(checkBox, nullptr);
    DPalette pal = DPaletteHelper::instance()->palette(checkBox);
    EXPECT_EQ(pal.color(DPalette::Text), darkTextColor);
}

// --- onChangeChooseBtnVisible ---

TEST_F(TimeLineDateWidgetTest, OnChangeChooseBtnVisible_True)
{
    m_widget->onChangeChooseBtnVisible(true);
    auto *btn = m_widget->findChild<DCommandLinkButton *>("ChooseBtn");
    ASSERT_NE(btn, nullptr);
    EXPECT_FALSE(btn->isHidden());
}

TEST_F(TimeLineDateWidgetTest, OnChangeChooseBtnVisible_False)
{
    m_widget->onChangeChooseBtnVisible(true);
    m_widget->onChangeChooseBtnVisible(false);
    auto *btn = m_widget->findChild<DCommandLinkButton *>("ChooseBtn");
    ASSERT_NE(btn, nullptr);
    EXPECT_FALSE(btn->isVisible());
}

// ===== importTimeLineDateWidget =====

// --- Constructor ---

TEST_F(ImportTimeLineDateWidgetTest, Constructor_ObjectNameAndAccessibleName)
{
    EXPECT_EQ(m_widget->objectName(), "ImportTimeLineDateWidget");
    EXPECT_EQ(m_widget->accessibleName(), "ImportTimeLineDateWidget");
}

TEST_F(ImportTimeLineDateWidgetTest, Constructor_CheckBoxTextSet)
{
    auto *checkBox = m_widget->findChild<DCheckBox *>("PDateandNumCheckBox");
    ASSERT_NE(checkBox, nullptr);
    EXPECT_EQ(checkBox->text(), m_time + " " + m_num);
}

TEST_F(ImportTimeLineDateWidgetTest, Constructor_CheckBoxInitiallyHidden)
{
    auto *checkBox = m_widget->findChild<DCheckBox *>("PDateandNumCheckBox");
    ASSERT_NE(checkBox, nullptr);
    EXPECT_FALSE(checkBox->isVisible());
}

TEST_F(ImportTimeLineDateWidgetTest, Constructor_ChooseBtnInitiallyHidden)
{
    // ChooseBtn has no objectName in importTimeLineDateWidget; find all command link buttons
    auto btns = m_widget->findChildren<DCommandLinkButton *>();
    bool foundHidden = false;
    for (auto *btn : btns) {
        if (btn->text() == QObject::tr("Select") && !btn->isVisible()) {
            foundHidden = true;
            break;
        }
    }
    EXPECT_TRUE(foundHidden);
}

// --- onShowCheckBox ---

TEST_F(ImportTimeLineDateWidgetTest, OnShowCheckBox_True_ShowsCheckBox)
{
    m_widget->onShowCheckBox(true);
    auto *checkBox = m_widget->findChild<DCheckBox *>("PDateandNumCheckBox");
    ASSERT_NE(checkBox, nullptr);
    EXPECT_FALSE(checkBox->isHidden());
}

TEST_F(ImportTimeLineDateWidgetTest, OnShowCheckBox_False_HidesCheckBox)
{
    m_widget->onShowCheckBox(true);
    m_widget->onShowCheckBox(false);
    auto *checkBox = m_widget->findChild<DCheckBox *>("PDateandNumCheckBox");
    ASSERT_NE(checkBox, nullptr);
    EXPECT_FALSE(checkBox->isVisible());
}

// --- onCheckBoxCliked ---

TEST_F(ImportTimeLineDateWidgetTest, OnCheckBoxCliked_EmitsSignal)
{
    QSignalSpy spy(m_widget.get(), &importTimeLineDateWidget::sigIsSelectCurrentDatePic);

    auto *checkBox = m_widget->findChild<DCheckBox *>("PDateandNumCheckBox");
    ASSERT_NE(checkBox, nullptr);
    checkBox->setChecked(true);
    m_widget->onCheckBoxCliked();

    ASSERT_EQ(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    EXPECT_TRUE(args.at(0).toBool());
    EXPECT_EQ(args.at(1).value<QStandardItem *>(), m_item.get());
}

// --- onTimeLinePicSelectAll ---

TEST_F(ImportTimeLineDateWidgetTest, OnTimeLinePicSelectAll_True)
{
    m_widget->onTimeLinePicSelectAll(true);
    auto *checkBox = m_widget->findChild<DCheckBox *>("PDateandNumCheckBox");
    ASSERT_NE(checkBox, nullptr);
    EXPECT_TRUE(checkBox->isChecked());
}

TEST_F(ImportTimeLineDateWidgetTest, OnTimeLinePicSelectAll_False)
{
    m_widget->onTimeLinePicSelectAll(true);
    m_widget->onTimeLinePicSelectAll(false);
    auto *checkBox = m_widget->findChild<DCheckBox *>("PDateandNumCheckBox");
    ASSERT_NE(checkBox, nullptr);
    EXPECT_FALSE(checkBox->isChecked());
}

// --- onGetBtnStatus ---

TEST_F(ImportTimeLineDateWidgetTest, OnGetBtnStatus_ReturnsButtonText)
{
    QString status = m_widget->onGetBtnStatus();
    auto btns = m_widget->findChildren<DCommandLinkButton *>();
    bool found = false;
    for (auto *btn : btns) {
        if (btn->text() == status && btn->text() == QObject::tr("Select")) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

// --- onThemeChanged ---

TEST_F(ImportTimeLineDateWidgetTest, OnThemeChanged_LightType_NoCrash)
{
    m_widget->onThemeChanged(DGuiApplicationHelper::LightType);
    SUCCEED();
}

TEST_F(ImportTimeLineDateWidgetTest, OnThemeChanged_DarkType_NoCrash)
{
    m_widget->onThemeChanged(DGuiApplicationHelper::DarkType);
    SUCCEED();
}

TEST_F(ImportTimeLineDateWidgetTest, OnThemeChanged_LightType_PaletteApplied)
{
    m_widget->onThemeChanged(DGuiApplicationHelper::LightType);
    auto *checkBox = m_widget->findChild<DCheckBox *>("PDateandNumCheckBox");
    ASSERT_NE(checkBox, nullptr);
    DPalette pal = DPaletteHelper::instance()->palette(checkBox);
    EXPECT_EQ(pal.color(DPalette::Text), lightTextColor);
}

TEST_F(ImportTimeLineDateWidgetTest, OnThemeChanged_DarkType_PaletteApplied)
{
    m_widget->onThemeChanged(DGuiApplicationHelper::DarkType);
    auto *checkBox = m_widget->findChild<DCheckBox *>("PDateandNumCheckBox");
    ASSERT_NE(checkBox, nullptr);
    DPalette pal = DPaletteHelper::instance()->palette(checkBox);
    EXPECT_EQ(pal.color(DPalette::Text), darkTextColor);
}

// --- onChangeChooseBtnVisible ---

TEST_F(ImportTimeLineDateWidgetTest, OnChangeChooseBtnVisible_True)
{
    m_widget->onChangeChooseBtnVisible(true);
    auto btns = m_widget->findChildren<DCommandLinkButton *>();
    bool foundVisible = false;
    for (auto *btn : btns) {
        if (btn->text() == QObject::tr("Select") && !btn->isHidden()) {
            foundVisible = true;
            break;
        }
    }
    EXPECT_TRUE(foundVisible);
}

TEST_F(ImportTimeLineDateWidgetTest, OnChangeChooseBtnVisible_False)
{
    m_widget->onChangeChooseBtnVisible(true);
    m_widget->onChangeChooseBtnVisible(false);
    auto btns = m_widget->findChildren<DCommandLinkButton *>();
    bool foundHidden = false;
    for (auto *btn : btns) {
        if (btn->text() == QObject::tr("Select") && !btn->isVisible()) {
            foundHidden = true;
            break;
        }
    }
    EXPECT_TRUE(foundHidden);
}

