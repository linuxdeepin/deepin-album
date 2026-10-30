// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMap>
#include <QMouseEvent>
#include <QFocusEvent>
#include <QResizeEvent>
#include <QSignalSpy>
#include <QGraphicsDropShadowEffect>
#include <QScreen>
#include <QGuiApplication>
#include <QCursor>
#include <QImage>
#include <QPainter>
#include <memory>
#include <cstdlib>
#include <unistd.h>

#include <DGuiApplicationHelper>
#include <DPaletteHelper>
#include <DHiDPIHelper>
#include <DFontSizeManager>

#include "widgets/widgtes/expansionmenu.h"
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

// ── Private field access for ExpansionMenu ──
ACCESS_PRIVATE_FIELD(ExpansionMenu, FilterWidget *, mainButton)
ACCESS_PRIVATE_FIELD(ExpansionMenu, ExpansionPanel *, panel)

// ── Private function access ──
ACCESS_PRIVATE_FUN(ExpansionPanel, void(ExpansionPanel::FilteData), onButtonClicked)
ACCESS_PRIVATE_FUN(ExpansionMenu, void(ExpansionPanel::FilteData &), onCurrentItemChanged)
ACCESS_PRIVATE_FUN(ExpansionMenu, void(), onMainButtonClicked)

// ── Testable subclass to expose protected methods ──
class TestableExpansionPanel : public ExpansionPanel
{
public:
    using ExpansionPanel::ExpansionPanel;
    using ExpansionPanel::focusOutEvent;
};

class TestableFilterWidget : public FilterWidget
{
public:
    using FilterWidget::FilterWidget;
    using FilterWidget::eventFilter;
    using FilterWidget::resizeEvent;
};

class TestableFilterLabel : public FilterLabel
{
public:
    using FilterLabel::FilterLabel;
    using FilterLabel::mouseReleaseEvent;
};

// ════════════════════════════════════════════════════════════════
// FilterLabel Tests
// ════════════════════════════════════════════════════════════════

class FilterLabelTest : public ::testing::Test
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
    TestableFilterLabel *label = nullptr;

    void SetUp() override
    {
        label = new TestableFilterLabel(nullptr);
    }

    void TearDown() override
    {
        delete label;
    }
};

TEST_F(FilterLabelTest, Constructor_ObjectNameCorrect)
{
    EXPECT_EQ(label->objectName(), QStringLiteral("FilterLabel"));
}

TEST_F(FilterLabelTest, Constructor_AccessibleNameCorrect)
{
    EXPECT_EQ(label->accessibleName(), QStringLiteral("FilterLabel"));
}

TEST_F(FilterLabelTest, MouseReleaseEvent_EmitsClicked)
{
    QSignalSpy spy(label, &FilterLabel::clicked);
    ASSERT_TRUE(spy.isValid());

    QMouseEvent ev(QEvent::MouseButtonRelease, QPointF(0, 0),
                   Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    EXPECT_NO_FATAL_FAILURE(label->mouseReleaseEvent(&ev));

    EXPECT_EQ(spy.count(), 1);
}

TEST_F(FilterLabelTest, MouseReleaseEvent_MultipleClicks)
{
    QSignalSpy spy(label, &FilterLabel::clicked);
    ASSERT_TRUE(spy.isValid());

    for (int i = 0; i < 3; ++i) {
        QMouseEvent ev(QEvent::MouseButtonRelease, QPointF(0, 0),
                       Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        label->mouseReleaseEvent(&ev);
    }
    EXPECT_EQ(spy.count(), 3);
}

// ════════════════════════════════════════════════════════════════
// FilterWidget Tests
// ════════════════════════════════════════════════════════════════

class FilterWidgetTest : public ::testing::Test
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
    TestableFilterWidget *widget = nullptr;

    void SetUp() override
    {
        widget = new TestableFilterWidget(nullptr);
    }

    void TearDown() override
    {
        delete widget;
    }

    static QIcon makeTestIcon(Qt::GlobalColor color)
    {
        QPixmap pix(16, 16);
        pix.fill(color);
        return QIcon(pix);
    }
};

TEST_F(FilterWidgetTest, Constructor_ObjectNameCorrect)
{
    EXPECT_EQ(widget->objectName(), QStringLiteral("FilterWidget"));
}

TEST_F(FilterWidgetTest, Constructor_AccessibleNameCorrect)
{
    EXPECT_EQ(widget->accessibleName(), QStringLiteral("FilterWidget"));
}

TEST_F(FilterWidgetTest, Constructor_ChildLabelsCreated)
{
    ASSERT_NE(widget->m_leftLabel, nullptr);
    ASSERT_NE(widget->m_btn, nullptr);
    ASSERT_NE(widget->m_rightLabel, nullptr);
    EXPECT_EQ(widget->m_leftLabel->objectName(), QStringLiteral("FilterLabel"));
    EXPECT_EQ(widget->m_btn->objectName(), QStringLiteral("FilterLabel"));
    EXPECT_EQ(widget->m_rightLabel->objectName(), QStringLiteral("FilterLabel"));
}

TEST_F(FilterWidgetTest, Constructor_ChildLabelsInLayout)
{
    auto *lay = widget->layout();
    ASSERT_NE(lay, nullptr);
    auto *hb = qobject_cast<QHBoxLayout *>(lay);
    ASSERT_NE(hb, nullptr);
    EXPECT_GE(hb->count(), 3);
}

TEST_F(FilterWidgetTest, SetText_SetsTextOnButton)
{
    widget->setText(QStringLiteral("TestText"));
    EXPECT_EQ(widget->m_btn->text(), QStringLiteral("TestText"));
}

TEST_F(FilterWidgetTest, SetText_EmptyString)
{
    widget->setText(QStringLiteral(""));
    EXPECT_EQ(widget->m_btn->text(), QStringLiteral(""));
}

TEST_F(FilterWidgetTest, SetIcon_SetsPixmapOnLeftLabel)
{
    QIcon icon = makeTestIcon(Qt::red);
    widget->setIcon(icon);
    EXPECT_FALSE(widget->m_leftLabel->pixmap(Qt::ReturnByValue).isNull());
}

TEST_F(FilterWidgetTest, SetFilteData_DataRoundTrip)
{
    ExpansionPanel::FilteData data;
    data.text = QStringLiteral("MyFilter");
    data.type = ItemType::ItemTypePic;
    data.icon_r_path = QStringLiteral("some_icon");

    widget->setFilteData(data);

    ExpansionPanel::FilteData retrieved = widget->getFilterData();
    EXPECT_EQ(retrieved.text, data.text);
    EXPECT_EQ(retrieved.type, data.type);
    EXPECT_EQ(retrieved.icon_r_path, data.icon_r_path);
}

TEST_F(FilterWidgetTest, GetFilteType_ReturnsCorrectType)
{
    ExpansionPanel::FilteData data;
    data.type = ItemType::ItemTypeVideo;
    widget->setFilteData(data);

    EXPECT_EQ(widget->getFilteType(), ItemType::ItemTypeVideo);
}

TEST_F(FilterWidgetTest, GetFilteType_DefaultIsNull)
{
    EXPECT_EQ(widget->getFilteType(), ItemType::ItemTypeNull);
}

TEST_F(FilterWidgetTest, OnClicked_EmitsClickedSignal)
{
    QSignalSpy spy(widget, &FilterWidget::clicked);
    ASSERT_TRUE(spy.isValid());

    widget->onClicked();
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(FilterWidgetTest, ThemeTypeChanged_LightType_NoCrash)
{
    ExpansionPanel::FilteData data;
    data.text = QStringLiteral("LightTest");
    data.icon_r_path = QStringLiteral("nonexistent_icon");
    widget->setFilteData(data);

    EXPECT_NO_FATAL_FAILURE(widget->themeTypeChanged(1));
    EXPECT_EQ(widget->m_btn->text(), QStringLiteral("LightTest"));
}

TEST_F(FilterWidgetTest, ThemeTypeChanged_DarkType_NoCrash)
{
    ExpansionPanel::FilteData data;
    data.text = QStringLiteral("DarkTest");
    data.icon_r_path = QStringLiteral("nonexistent_icon");
    widget->setFilteData(data);

    EXPECT_NO_FATAL_FAILURE(widget->themeTypeChanged(2));
    EXPECT_EQ(widget->m_btn->text(), QStringLiteral("DarkTest"));
}

TEST_F(FilterWidgetTest, ThemeTypeChanged_BothBranches_NoCrash)
{
    ExpansionPanel::FilteData data;
    data.text = QStringLiteral("BothTest");
    data.icon_r_path = QStringLiteral("icon");
    widget->setFilteData(data);

    EXPECT_NO_FATAL_FAILURE(widget->themeTypeChanged(1));
    EXPECT_NO_FATAL_FAILURE(widget->themeTypeChanged(2));
    EXPECT_NO_FATAL_FAILURE(widget->themeTypeChanged(1));
}

TEST_F(FilterWidgetTest, EventFilter_MouseButtonPressOnBtn_TriggersOnClicked)
{
    QSignalSpy spy(widget, &FilterWidget::clicked);
    ASSERT_TRUE(spy.isValid());

    QMouseEvent ev(QEvent::MouseButtonPress, QPointF(0, 0),
                   Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    bool result = widget->eventFilter(widget->m_btn, &ev);

    EXPECT_TRUE(result);
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(FilterWidgetTest, EventFilter_MouseButtonPressOnRightLabel_TriggersOnClicked)
{
    QSignalSpy spy(widget, &FilterWidget::clicked);
    ASSERT_TRUE(spy.isValid());

    QMouseEvent ev(QEvent::MouseButtonPress, QPointF(0, 0),
                   Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    bool result = widget->eventFilter(widget->m_rightLabel, &ev);

    EXPECT_TRUE(result);
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(FilterWidgetTest, EventFilter_MouseMoveOnBtn_Intercepted)
{
    QMouseEvent ev(QEvent::MouseMove, QPointF(0, 0),
                   Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    bool result = widget->eventFilter(widget->m_btn, &ev);
    EXPECT_TRUE(result);
}

TEST_F(FilterWidgetTest, EventFilter_MouseMoveOnRightLabel_Intercepted)
{
    QMouseEvent ev(QEvent::MouseMove, QPointF(0, 0),
                   Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    bool result = widget->eventFilter(widget->m_rightLabel, &ev);
    EXPECT_TRUE(result);
}

TEST_F(FilterWidgetTest, EventFilter_OtherObject_NotIntercepted)
{
    QObject other;
    QMouseEvent ev(QEvent::MouseButtonPress, QPointF(0, 0),
                   Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    bool result = widget->eventFilter(&other, &ev);
    EXPECT_FALSE(result);
}

TEST_F(FilterWidgetTest, EventFilter_NonMouseEventOnBtn_NotIntercepted)
{
    QEvent ev(QEvent::None);
    bool result = widget->eventFilter(widget->m_btn, &ev);
    EXPECT_FALSE(result);
}

TEST_F(FilterWidgetTest, ResizeEvent_EmitsSigWidthChanged)
{
    QSignalSpy spy(widget, &FilterWidget::sigWidthChanged);
    ASSERT_TRUE(spy.isValid());

    widget->resize(200, 30);
    QResizeEvent ev(QSize(200, 30), QSize(100, 30));
    EXPECT_NO_FATAL_FAILURE(widget->resizeEvent(&ev));

    // resize() may also trigger it; at least the manual event should
    EXPECT_GE(spy.count(), 1);
    // Last argument should be the new width
    if (spy.count() > 0) {
        int lastWidth = spy.takeLast().at(0).toInt();
        EXPECT_EQ(lastWidth, 200);
    }
}

TEST_F(FilterWidgetTest, ResizeEvent_EmitsCorrectWidth)
{
    widget->resize(150, 30);
    QSignalSpy spy(widget, &FilterWidget::sigWidthChanged);
    ASSERT_TRUE(spy.isValid());

    QResizeEvent ev(QSize(150, 30), QSize(100, 30));
    widget->resizeEvent(&ev);

    ASSERT_GE(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toInt(), 150);
}

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
};

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

TEST_F(ExpansionPanelTest, Constructor_HasPopupFlag)
{
    EXPECT_TRUE(panel->windowFlags() & Qt::Popup);
}

TEST_F(ExpansionPanelTest, Constructor_InitialButtonCountZero)
{
    int count = access_private_field::ExpansionPanelbuttonCount(*panel);
    EXPECT_EQ(count, 0);
}

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

TEST_F(ExpansionPanelTest, AddNewButton_MultipleButtons_AllInMap)
{
    for (int i = 0; i < 3; ++i) {
        auto data = makeFilteData(QString("Button%1").arg(i));
        panel->addNewButton(data);
    }

    auto &buttons = access_private_field::ExpansionPanelbuttons(*panel);
    EXPECT_EQ(buttons.size(), 3);
    EXPECT_EQ(buttons[0]->text(), QStringLiteral("Button0"));
    EXPECT_EQ(buttons[1]->text(), QStringLiteral("Button1"));
    EXPECT_EQ(buttons[2]->text(), QStringLiteral("Button2"));
}

TEST_F(ExpansionPanelTest, OnButtonClicked_HidesPanel)
{
    panel->show();
    ASSERT_TRUE(panel->isVisible());

    auto data = makeFilteData("Test");
    EXPECT_NO_FATAL_FAILURE(
        call_private_fun::ExpansionPanelonButtonClicked(static_cast<ExpansionPanel &>(*panel), data));

    EXPECT_FALSE(panel->isVisible());
}

TEST_F(ExpansionPanelTest, OnButtonClicked_EmitsCurrentItemChanged)
{
    QSignalSpy spy(panel, &ExpansionPanel::currentItemChanged);
    ASSERT_TRUE(spy.isValid());

    auto data = makeFilteData("SignalTest");
    call_private_fun::ExpansionPanelonButtonClicked(static_cast<ExpansionPanel &>(*panel), data);

    EXPECT_EQ(spy.count(), 1);
}

TEST_F(ExpansionPanelTest, FocusOutEvent_HidesPanel)
{
    panel->show();
    ASSERT_TRUE(panel->isVisible());

    QFocusEvent ev(QEvent::FocusOut);
    EXPECT_NO_FATAL_FAILURE(panel->focusOutEvent(&ev));

    EXPECT_FALSE(panel->isVisible());
}

TEST_F(ExpansionPanelTest, ChangeTheme_LightType_NoCrash)
{
    // shadow_effect is nullptr by default; set it before calling changeTheme
    auto *effect = new QGraphicsDropShadowEffect;
    access_private_field::ExpansionPanelshadow_effect(*panel) = effect;

    EXPECT_NO_FATAL_FAILURE(
        panel->changeTheme(Dtk::Gui::DGuiApplicationHelper::LightType));

    // Clean up
    delete effect;
    access_private_field::ExpansionPanelshadow_effect(*panel) = nullptr;
}

TEST_F(ExpansionPanelTest, ChangeTheme_DarkType_NoCrash)
{
    auto *effect = new QGraphicsDropShadowEffect;
    access_private_field::ExpansionPanelshadow_effect(*panel) = effect;

    EXPECT_NO_FATAL_FAILURE(
        panel->changeTheme(Dtk::Gui::DGuiApplicationHelper::DarkType));

    delete effect;
    access_private_field::ExpansionPanelshadow_effect(*panel) = nullptr;
}

TEST_F(ExpansionPanelTest, ChangeTheme_BothBranches_NoCrash)
{
    auto *effect = new QGraphicsDropShadowEffect;
    access_private_field::ExpansionPanelshadow_effect(*panel) = effect;

    EXPECT_NO_FATAL_FAILURE(
        panel->changeTheme(Dtk::Gui::DGuiApplicationHelper::LightType));
    EXPECT_NO_FATAL_FAILURE(
        panel->changeTheme(Dtk::Gui::DGuiApplicationHelper::DarkType));

    delete effect;
    access_private_field::ExpansionPanelshadow_effect(*panel) = nullptr;
}

// ════════════════════════════════════════════════════════════════
// ExpansionMenu Tests
// ════════════════════════════════════════════════════════════════

class ExpansionMenuTest : public ::testing::Test
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
    ExpansionMenu *menu = nullptr;

    void SetUp() override
    {
        menu = new ExpansionMenu(nullptr);
    }

    void TearDown() override
    {
        delete menu;
    }

    ExpansionPanel::FilteData makeFilteData(const QString &text)
    {
        ExpansionPanel::FilteData data;
        data.text = text;
        data.type = ItemType::ItemTypePic;
        return data;
    }
};

TEST_F(ExpansionMenuTest, Constructor_CreatesPanel)
{
    auto *p = access_private_field::ExpansionMenupanel(*menu);
    ASSERT_NE(p, nullptr);
    EXPECT_FALSE(p->isVisible());
}

TEST_F(ExpansionMenuTest, Constructor_CreatesMainButton)
{
    auto *btn = access_private_field::ExpansionMenumainButton(*menu);
    ASSERT_NE(btn, nullptr);
}

TEST_F(ExpansionMenuTest, MainWidget_ReturnsMainButton)
{
    auto *btn = access_private_field::ExpansionMenumainButton(*menu);
    EXPECT_EQ(menu->mainWidget(), btn);
}

TEST_F(ExpansionMenuTest, AddNewButton_IncreasesPanelButtonCount)
{
    auto *p = access_private_field::ExpansionMenupanel(*menu);
    ASSERT_NE(p, nullptr);

    int before = access_private_field::ExpansionPanelbuttonCount(*p);

    auto data = makeFilteData("MenuButton1");
    menu->addNewButton(data);

    int after = access_private_field::ExpansionPanelbuttonCount(*p);
    EXPECT_EQ(after, before + 1);
}

TEST_F(ExpansionMenuTest, AddNewButton_MultipleButtons)
{
    auto *p = access_private_field::ExpansionMenupanel(*menu);

    for (int i = 0; i < 3; ++i) {
        auto data = makeFilteData(QString("MB%1").arg(i));
        menu->addNewButton(data);
    }

    EXPECT_EQ(access_private_field::ExpansionPanelbuttonCount(*p), 3);
}

TEST_F(ExpansionMenuTest, SetDefaultFilteData_MainButtonDataMatches)
{
    auto data = makeFilteData("DefaultFilter");
    menu->setDefaultFilteData(data);

    auto *btn = menu->mainWidget();
    ASSERT_NE(btn, nullptr);
    auto retrieved = btn->getFilterData();
    EXPECT_EQ(retrieved.text, QStringLiteral("DefaultFilter"));
    EXPECT_EQ(retrieved.type, ItemType::ItemTypePic);
}

TEST_F(ExpansionMenuTest, OnCurrentItemChanged_UpdatesMainButtonData)
{
    auto data = makeFilteData("ChangedData");
    data.type = ItemType::ItemTypeVideo;

    EXPECT_NO_FATAL_FAILURE(
        call_private_fun::ExpansionMenuonCurrentItemChanged(*menu, data));

    auto *btn = menu->mainWidget();
    ASSERT_NE(btn, nullptr);
    auto retrieved = btn->getFilterData();
    EXPECT_EQ(retrieved.text, QStringLiteral("ChangedData"));
    EXPECT_EQ(retrieved.type, ItemType::ItemTypeVideo);
}

TEST_F(ExpansionMenuTest, OnCurrentItemChanged_EmitsCurrentItemChangedFromMainButton)
{
    auto *btn = menu->mainWidget();
    ASSERT_NE(btn, nullptr);

    QSignalSpy spy(btn, &FilterWidget::currentItemChanged);
    ASSERT_TRUE(spy.isValid());

    auto data = makeFilteData("SignalTest");
    call_private_fun::ExpansionMenuonCurrentItemChanged(*menu, data);

    EXPECT_EQ(spy.count(), 1);
}

TEST_F(ExpansionMenuTest, OnMainButtonClicked_ShowsHiddenPanel)
{
    auto *p = access_private_field::ExpansionMenupanel(*menu);
    ASSERT_NE(p, nullptr);
    ASSERT_FALSE(p->isVisible());

    EXPECT_NO_FATAL_FAILURE(
        call_private_fun::ExpansionMenuonMainButtonClicked(*menu));

    EXPECT_TRUE(p->isVisible());
}

TEST_F(ExpansionMenuTest, OnMainButtonClicked_HidesVisiblePanel)
{
    auto *p = access_private_field::ExpansionMenupanel(*menu);
    ASSERT_NE(p, nullptr);

    // First click shows
    call_private_fun::ExpansionMenuonMainButtonClicked(*menu);
    ASSERT_TRUE(p->isVisible());

    // Second click hides
    EXPECT_NO_FATAL_FAILURE(
        call_private_fun::ExpansionMenuonMainButtonClicked(*menu));

    EXPECT_FALSE(p->isVisible());
}

TEST_F(ExpansionMenuTest, OnMainButtonClicked_ToggleMultipleTimes)
{
    auto *p = access_private_field::ExpansionMenupanel(*menu);

    for (int i = 0; i < 4; ++i) {
        EXPECT_NO_FATAL_FAILURE(
            call_private_fun::ExpansionMenuonMainButtonClicked(*menu));
        // Even clicks show, odd clicks hide
        if (i % 2 == 0) {
            EXPECT_TRUE(p->isVisible()) << "Failed at iteration " << i;
        } else {
            EXPECT_FALSE(p->isVisible()) << "Failed at iteration " << i;
        }
    }
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
