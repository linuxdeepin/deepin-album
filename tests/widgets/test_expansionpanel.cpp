// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QApplication>
#include <QVBoxLayout>
#include <QMap>
#include <QFocusEvent>
#include <QSignalSpy>
#include <QGraphicsDropShadowEffect>
#include <memory>
#include <cstdlib>
#include <unistd.h>

#include <DGuiApplicationHelper>
#include <DPaletteHelper>
#include <DHiDPIHelper>
#include <DFontSizeManager>

#include "widgets/widgtes/expansionpanel.h"
#include "widgets/widgtes/toolbutton.h"

#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

// ── Private field access for ExpansionPanel ──
using ToolButtonMap = QMap<int, ToolButton *>;
ACCESS_PRIVATE_FIELD(ExpansionPanel, QGraphicsDropShadowEffect *, shadow_effect)
ACCESS_PRIVATE_FIELD(ExpansionPanel, ToolButtonMap, buttons)
ACCESS_PRIVATE_FIELD(ExpansionPanel, int, buttonCount)
ACCESS_PRIVATE_FIELD(ExpansionPanel, QVBoxLayout *, layout)

// ── Private function access ──
ACCESS_PRIVATE_FUN(ExpansionPanel, void(ExpansionPanel::FilteData), onButtonClicked)

// ── Testable subclass to expose protected methods ──
class TestableExpansionPanel : public ExpansionPanel
{
public:
    using ExpansionPanel::ExpansionPanel;
    using ExpansionPanel::focusOutEvent;
};

/*
 * Branch analysis (verified via codebase-memory-mcp get_code_snippet):
 *
 * ExpansionPanel::ExpansionPanel(QWidget*)     [expansionpanel.cpp:14-40]
 *   Linear initialization — no conditional branches.
 *
 * ExpansionPanel::changeTheme(ColorType)       [expansionpanel.cpp:42-60]
 *   Branch 1: themeType == 1 (DarkType)
 *             → setMaskColor(QColor("#EBEBEB") alpha=80)
 *               shadow_effect->setColor(QColor(150,150,150))
 *   Branch 2: else (LightType, value 0)
 *             → setMaskColor(QColor("#404040"))
 *               shadow_effect->setColor(QColor("#404040"))
 *
 * ExpansionPanel::onButtonClicked(FilteData)   [expansionpanel.cpp:62-68]
 *   Linear: hide() + emit currentItemChanged(data) — no branches.
 *
 * ExpansionPanel::addNewButton(FilteData&)     [expansionpanel.cpp:70-85]
 *   Linear: new ToolButton → store in buttons[buttonCount++] → setText/
 *           setLIcon/setRIcon/setRWIcon → setFixedSize(190,34) → addWidget →
 *           connect(clicked → onButtonClicked) — no branches.
 *
 * ExpansionPanel::focusOutEvent(QFocusEvent*)  [expansionpanel.cpp:87-93]
 *   Linear: hide() + DBlurEffectWidget::focusOutEvent(e) — no branches.
 *
 * Signals (verified via MCP search_graph across project):
 *   - currentItemChanged(FilteData): emitted by onButtonClicked — tested.
 *   - signalItemGroup / signalItemgUngroup: NOT found in the MCP-indexed
 *     knowledge graph for ExpansionPanel; no method emits them. No trigger
 *     path exists to test — omitted with this note.
 */

// ════════════════════════════════════════════════════════════════
// ExpansionPanel Tests
// ════════════════════════════════════════════════════════════════

class ExpansionPanelTest : public ::testing::Test
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
    TestableExpansionPanel *panel = nullptr;

    void SetUp() override
    {
        panel = new TestableExpansionPanel();
    }

    void TearDown() override
    {
        delete panel;
    }

    ExpansionPanel::FilteData makeFilteData(const QString &text)
    {
        ExpansionPanel::FilteData data;
        data.text = text;
        data.type = ItemType::ItemTypePic;
        return data;
    }

    // Helper: install a real QGraphicsDropShadowEffect so changeTheme can
    // dereference shadow_effect without crashing.  Returns the effect for
    // assertions; caller owns cleanup.
    QGraphicsDropShadowEffect *installShadowEffect()
    {
        auto *effect = new QGraphicsDropShadowEffect;
        access_private_field::ExpansionPanelshadow_effect(*panel) = effect;
        return effect;
    }

    void removeShadowEffect(QGraphicsDropShadowEffect *effect)
    {
        delete effect;
        access_private_field::ExpansionPanelshadow_effect(*panel) = nullptr;
    }
};

// ── Constructor: instance creation & initial state ──

TEST_F(ExpansionPanelTest, Constructor_ObjectNameCorrect)
{
    EXPECT_EQ(panel->objectName(), QStringLiteral("ExpansionPanel"));
}

TEST_F(ExpansionPanelTest, Constructor_AccessibleNameCorrect)
{
    EXPECT_EQ(panel->accessibleName(), QStringLiteral("ExpansionPanel"));
}

TEST_F(ExpansionPanelTest, Constructor_HasVBoxLayout)
{
    auto *lay = access_private_field::ExpansionPanellayout(*panel);
    ASSERT_NE(lay, nullptr);
    EXPECT_TRUE(qobject_cast<QVBoxLayout *>(lay) != nullptr);
}

TEST_F(ExpansionPanelTest, Constructor_LayoutMarginsCorrect)
{
    auto *lay = access_private_field::ExpansionPanellayout(*panel);
    ASSERT_NE(lay, nullptr);
    EXPECT_EQ(lay->contentsMargins(), QMargins(0, 12, 0, 12));
}

TEST_F(ExpansionPanelTest, Constructor_HasPopupFlag)
{
    EXPECT_TRUE(panel->windowFlags() & Qt::Popup);
}

TEST_F(ExpansionPanelTest, Constructor_InitialButtonCountZero)
{
    EXPECT_EQ(access_private_field::ExpansionPanelbuttonCount(*panel), 0);
}

TEST_F(ExpansionPanelTest, Constructor_InitialButtonsMapEmpty)
{
    auto &buttons = access_private_field::ExpansionPanelbuttons(*panel);
    EXPECT_EQ(buttons.size(), 0);
}

TEST_F(ExpansionPanelTest, Constructor_BlurEnabled)
{
    EXPECT_TRUE(panel->blurEnabled());
}

// ── addNewButton: dynamic button addition, mapping, icons & text ──

TEST_F(ExpansionPanelTest, AddNewButton_IncrementsButtonCount)
{
    auto data1 = makeFilteData("Button1");
    panel->addNewButton(data1);
    EXPECT_EQ(access_private_field::ExpansionPanelbuttonCount(*panel), 1);

    auto data2 = makeFilteData("Button2");
    panel->addNewButton(data2);
    EXPECT_EQ(access_private_field::ExpansionPanelbuttonCount(*panel), 2);
}

TEST_F(ExpansionPanelTest, AddNewButton_ButtonAddedToLayout)
{
    auto data = makeFilteData("TestButton");
    panel->addNewButton(data);

    auto *lay = access_private_field::ExpansionPanellayout(*panel);
    ASSERT_NE(lay, nullptr);
    EXPECT_GE(lay->count(), 1);
}

TEST_F(ExpansionPanelTest, AddNewButton_ButtonTextMatchesData)
{
    auto data = makeFilteData("MyButton");
    panel->addNewButton(data);

    auto &buttons = access_private_field::ExpansionPanelbuttons(*panel);
    ASSERT_EQ(buttons.size(), 1);
    ASSERT_NE(buttons[0], nullptr);
    EXPECT_EQ(buttons[0]->text(), QStringLiteral("MyButton"));
}

TEST_F(ExpansionPanelTest, AddNewButton_StoresInButtonsMapWithSequentialKeys)
{
    for (int i = 0; i < 3; ++i) {
        auto data = makeFilteData(QString("Button%1").arg(i));
        panel->addNewButton(data);
    }

    auto &buttons = access_private_field::ExpansionPanelbuttons(*panel);
    EXPECT_EQ(buttons.size(), 3);
    EXPECT_TRUE(buttons.contains(0));
    EXPECT_TRUE(buttons.contains(1));
    EXPECT_TRUE(buttons.contains(2));
}

TEST_F(ExpansionPanelTest, AddNewButton_MultipleButtons_AllTextMatch)
{
    for (int i = 0; i < 3; ++i) {
        auto data = makeFilteData(QString("Button%1").arg(i));
        panel->addNewButton(data);
    }

    auto &buttons = access_private_field::ExpansionPanelbuttons(*panel);
    ASSERT_EQ(buttons.size(), 3);
    EXPECT_EQ(buttons[0]->text(), QStringLiteral("Button0"));
    EXPECT_EQ(buttons[1]->text(), QStringLiteral("Button1"));
    EXPECT_EQ(buttons[2]->text(), QStringLiteral("Button2"));
}

TEST_F(ExpansionPanelTest, AddNewButton_ButtonHasFixedSize)
{
    auto data = makeFilteData("FixedSizeButton");
    panel->addNewButton(data);

    auto &buttons = access_private_field::ExpansionPanelbuttons(*panel);
    ASSERT_EQ(buttons.size(), 1);
    ASSERT_NE(buttons[0], nullptr);
    EXPECT_EQ(buttons[0]->minimumSize(), QSize(190, 34));
    EXPECT_EQ(buttons[0]->maximumSize(), QSize(190, 34));
}

TEST_F(ExpansionPanelTest, AddNewButton_ButtonIsChildOfPanel)
{
    auto data = makeFilteData("ChildButton");
    panel->addNewButton(data);

    auto &buttons = access_private_field::ExpansionPanelbuttons(*panel);
    ASSERT_EQ(buttons.size(), 1);
    ASSERT_NE(buttons[0], nullptr);
    EXPECT_EQ(buttons[0]->parentWidget(), panel);
}

TEST_F(ExpansionPanelTest, AddNewButton_IconsSetFromFilteData)
{
    // Build a FilteData with non-null QIcons so we can verify round-trip.
    QPixmap pm(16, 16);
    pm.fill(Qt::red);

    ExpansionPanel::FilteData data;
    data.text = "IconTest";
    data.type = ItemType::ItemTypePic;
    data.icon_l_light = QIcon(pm);
    data.icon_l_dark = QIcon(pm);
    data.icon_r_light = QIcon(pm);
    data.icon_r_dark = QIcon(pm);

    panel->addNewButton(data);

    auto &buttons = access_private_field::ExpansionPanelbuttons(*panel);
    ASSERT_EQ(buttons.size(), 1);
    ASSERT_NE(buttons[0], nullptr);

    auto lIcons = buttons[0]->lIcon();
    EXPECT_FALSE(lIcons.first.isNull());
    EXPECT_FALSE(lIcons.second.isNull());

    auto rIcons = buttons[0]->rIcon();
    EXPECT_FALSE(rIcons.first.isNull());
    EXPECT_FALSE(rIcons.second.isNull());
}

TEST_F(ExpansionPanelTest, AddNewButton_ButtonClickEmitsCurrentItemChanged)
{
    // Verify the connect inside addNewButton wires ToolButton::clicked
    // to onButtonClicked (hide + emit currentItemChanged).
    QSignalSpy spy(panel, &ExpansionPanel::currentItemChanged);
    ASSERT_TRUE(spy.isValid());

    auto data = makeFilteData("ClickTest");
    panel->addNewButton(data);

    auto &buttons = access_private_field::ExpansionPanelbuttons(*panel);
    ASSERT_EQ(buttons.size(), 1);
    ASSERT_NE(buttons[0], nullptr);

    panel->show();
    ASSERT_TRUE(panel->isVisible());

    // Trigger the button — connected lambda calls onButtonClicked(data)
    EXPECT_NO_FATAL_FAILURE(buttons[0]->click());

    EXPECT_EQ(spy.count(), 1);
    EXPECT_FALSE(panel->isVisible());
}

// ── changeTheme: light / dark branch coverage ──

TEST_F(ExpansionPanelTest, ChangeTheme_DarkType_SetsShadowColor)
{
    auto *effect = installShadowEffect();

    panel->changeTheme(Dtk::Gui::DGuiApplicationHelper::DarkType);

    EXPECT_EQ(effect->color(), QColor(150, 150, 150));

    removeShadowEffect(effect);
}

TEST_F(ExpansionPanelTest, ChangeTheme_LightType_SetsShadowColor)
{
    auto *effect = installShadowEffect();

    panel->changeTheme(Dtk::Gui::DGuiApplicationHelper::LightType);

    EXPECT_EQ(effect->color(), QColor("#404040"));

    removeShadowEffect(effect);
}

TEST_F(ExpansionPanelTest, ChangeTheme_DarkType_SetsMaskColor)
{
    auto *effect = installShadowEffect();

    panel->changeTheme(Dtk::Gui::DGuiApplicationHelper::DarkType);

    QColor expected("#EBEBEB");
    expected.setAlpha(80);
    EXPECT_EQ(panel->maskColor(), expected);

    removeShadowEffect(effect);
}

TEST_F(ExpansionPanelTest, ChangeTheme_LightType_SetsMaskColor)
{
    auto *effect = installShadowEffect();

    panel->changeTheme(Dtk::Gui::DGuiApplicationHelper::LightType);

    EXPECT_EQ(panel->maskColor(), QColor("#404040"));

    removeShadowEffect(effect);
}

TEST_F(ExpansionPanelTest, ChangeTheme_BothBranches_NoCrash)
{
    auto *effect = installShadowEffect();

    EXPECT_NO_FATAL_FAILURE(panel->changeTheme(Dtk::Gui::DGuiApplicationHelper::LightType));
    EXPECT_NO_FATAL_FAILURE(panel->changeTheme(Dtk::Gui::DGuiApplicationHelper::DarkType));

    removeShadowEffect(effect);
}

// ── onButtonClicked: hide + signal emission ──

TEST_F(ExpansionPanelTest, OnButtonClicked_HidesPanel)
{
    panel->show();
    ASSERT_TRUE(panel->isVisible());

    auto data = makeFilteData("Test");
    EXPECT_NO_FATAL_FAILURE(
        call_private_fun::ExpansionPanelonButtonClicked(
            static_cast<ExpansionPanel &>(*panel), data));

    EXPECT_FALSE(panel->isVisible());
}

TEST_F(ExpansionPanelTest, OnButtonClicked_EmitsCurrentItemChanged)
{
    QSignalSpy spy(panel, &ExpansionPanel::currentItemChanged);
    ASSERT_TRUE(spy.isValid());

    auto data = makeFilteData("SignalTest");
    call_private_fun::ExpansionPanelonButtonClicked(
        static_cast<ExpansionPanel &>(*panel), data);

    EXPECT_EQ(spy.count(), 1);
}

TEST_F(ExpansionPanelTest, OnButtonClicked_EmitsCurrentItemChangedWithData)
{
    ExpansionPanel::FilteData captured;
    QObject::connect(panel, &ExpansionPanel::currentItemChanged,
                     [&captured](ExpansionPanel::FilteData d) {
                         captured = d;
                     });

    auto data = makeFilteData("DataCheck");
    data.type = ItemType::ItemTypeVideo;
    call_private_fun::ExpansionPanelonButtonClicked(
        static_cast<ExpansionPanel &>(*panel), data);

    EXPECT_EQ(captured.text, QStringLiteral("DataCheck"));
    EXPECT_EQ(captured.type, ItemType::ItemTypeVideo);
}

TEST_F(ExpansionPanelTest, OnButtonClicked_MultipleEmits)
{
    QSignalSpy spy(panel, &ExpansionPanel::currentItemChanged);
    ASSERT_TRUE(spy.isValid());

    for (int i = 0; i < 3; ++i) {
        auto data = makeFilteData(QString("Emit%1").arg(i));
        call_private_fun::ExpansionPanelonButtonClicked(
            static_cast<ExpansionPanel &>(*panel), data);
    }

    EXPECT_EQ(spy.count(), 3);
}

// ── focusOutEvent: hide behavior ──

TEST_F(ExpansionPanelTest, FocusOutEvent_HidesPanel)
{
    panel->show();
    ASSERT_TRUE(panel->isVisible());

    QFocusEvent ev(QEvent::FocusOut);
    EXPECT_NO_FATAL_FAILURE(panel->focusOutEvent(&ev));

    EXPECT_FALSE(panel->isVisible());
}

TEST_F(ExpansionPanelTest, FocusOutEvent_AlreadyHidden_StaysHidden)
{
    ASSERT_FALSE(panel->isVisible());

    QFocusEvent ev(QEvent::FocusOut);
    EXPECT_NO_FATAL_FAILURE(panel->focusOutEvent(&ev));

    EXPECT_FALSE(panel->isVisible());
}

// ── currentItemChanged signal: existence & connectivity ──

TEST_F(ExpansionPanelTest, CurrentItemChanged_SignalExists)
{
    QSignalSpy spy(panel, &ExpansionPanel::currentItemChanged);
    EXPECT_TRUE(spy.isValid());
}

// Custom main to ensure clean process exit (QApplication may have lingering timers)
int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }
    if (!QApplication::instance()) {
        static int ac = 1;
        static char arg0[] = "test";
        static char *av[] = {arg0, nullptr};
        new QApplication(ac, av);
    }
    int result = RUN_ALL_TESTS();
    _exit(result);
}
