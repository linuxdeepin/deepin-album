// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * 用例统计表
 * | Types::Types (constructor) | high | 1 | 1 | 1 |
 * | Types::~Types (destructor) | high | 1 | 1 | 1 |
 * | ItemType enum values | high | 2 | 2 | 2 |
 * | ItemRole enum values | high | 2 | 2 | 2 |
 * | SwitchType enum values | high | 2 | 2 | 2 |
 * | WidgetViewType enum values | high | 2 | 2 | 2 |
 * | Q_ENUMS reflection (ItemType) | high | 2 | 2 | 2 |
 * | Q_ENUMS reflection (ModelType) | high | 2 | 2 | 2 |
 * | Q_ENUMS reflection (ThumbnailViewType) | high | 2 | 2 | 2 |
 * | Q_ENUMS reflection (ImageType) | high | 2 | 2 | 2 |
 * | Q_ENUMS reflection (MenuItemId) | high | 2 | 2 | 2 |
 * | Q_ENUMS reflection (SwitchType) | high | 2 | 2 | 2 |
 * | Q_ENUMS reflection (WidgetViewType) | high | 2 | 2 | 2 |
 *
 * 分支列表:
 *   Constructor:
 *     C1: 构造对象不崩溃且非空
 *   Destructor:
 *     D1: 析构对象不崩溃
 *   ItemType values:
 *     V1: All == 0
 *     V2: Picture == 1, Video == 2 (递增)
 *   ItemRole values:
 *     R1: ImageUrlRole == Qt::UserRole + 1
 *     R2: FrameIndexRole == Qt::UserRole + 2, ImageAngleRole == Qt::UserRole + 3 (递增)
 *   SwitchType values:
 *     S1: UnknownSwitchType == -1
 *     S2: HardCut == 0, 后续递增
 *   WidgetViewType values:
 *     W1: WidgetViewUnknown == -1
 *     W2: WidgetDayView == 0, WidgetImportedView == 1
 *   Q_ENUMS reflection (各枚举, 经由 assertEnumRegistered / assertEnumKeyValue 辅助函数):
 *     E1: 枚举已注册 (metaEnum.isValid())
 *     E2: 键名与键值一一对应 (keyToValue / valueToKey 往返一致)
 */

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QMetaEnum>
#include <QMetaObject>
#include <memory>

#include "types.h"

class TypesTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test_types";
            static char *argv[] = {arg0, nullptr};
            new QGuiApplication(argc, argv);
        }
    }
};

// ---- QMetaEnum reflection helpers ----

// Assert the named enum is registered on Types' staticMetaObject
static void assertEnumRegistered(const char *name)
{
    const QMetaObject *meta = &Types::staticMetaObject;
    int idx = meta->indexOfEnumerator(name);
    ASSERT_GE(idx, 0);
    QMetaEnum me = meta->enumerator(idx);
    EXPECT_TRUE(me.isValid());
}

// Assert key maps to value and value maps back to key for the named enum
static void assertEnumKeyValue(const char *name, const char *key, int value)
{
    const QMetaObject *meta = &Types::staticMetaObject;
    int idx = meta->indexOfEnumerator(name);
    ASSERT_GE(idx, 0);
    QMetaEnum me = meta->enumerator(idx);
    ASSERT_TRUE(me.isValid());
    EXPECT_EQ(me.keyToValue(key), value);
    EXPECT_STREQ(me.valueToKey(value), key);
}

// ---- Constructor / Destructor ----

TEST_F(TypesTest, Constructor_DoesNotCrash)
{
    std::unique_ptr<Types> t = std::make_unique<Types>();
    ASSERT_NE(nullptr, t.get());
}

TEST_F(TypesTest, Destructor_DoesNotCrash)
{
    std::unique_ptr<Types> t = std::make_unique<Types>();
    EXPECT_NO_THROW(t.reset());
}

// ---- ItemType enum values ----

TEST_F(TypesTest, ItemType_All_IsZero)
{
    EXPECT_EQ(static_cast<int>(Types::All), 0);
}

TEST_F(TypesTest, ItemType_IncrementingValues)
{
    EXPECT_EQ(static_cast<int>(Types::Picture), 1);
    EXPECT_EQ(static_cast<int>(Types::Video), 2);
}

// ---- ItemRole enum values ----

TEST_F(TypesTest, ItemRole_ImageUrlRole_IsUserRolePlusOne)
{
    EXPECT_EQ(static_cast<int>(Types::ImageUrlRole), Qt::UserRole + 1);
}

TEST_F(TypesTest, ItemRole_IncrementingValues)
{
    EXPECT_EQ(static_cast<int>(Types::FrameIndexRole), Qt::UserRole + 2);
    EXPECT_EQ(static_cast<int>(Types::ImageAngleRole), Qt::UserRole + 3);
}

// ---- SwitchType enum values ----

TEST_F(TypesTest, SwitchType_UnknownSwitchType_IsMinusOne)
{
    EXPECT_EQ(static_cast<int>(Types::UnknownSwitchType), -1);
}

TEST_F(TypesTest, SwitchType_IncrementingValues)
{
    EXPECT_EQ(static_cast<int>(Types::HardCut), 0);
    EXPECT_EQ(static_cast<int>(Types::FlipScroll), 1);
    EXPECT_EQ(static_cast<int>(Types::FadeInOut), 2);
    EXPECT_EQ(static_cast<int>(Types::ScaleInOUt), 3);
}

// ---- WidgetViewType enum values ----

TEST_F(TypesTest, WidgetViewType_WidgetViewUnknown_IsMinusOne)
{
    EXPECT_EQ(static_cast<int>(Types::WidgetViewUnknown), -1);
}

TEST_F(TypesTest, WidgetViewType_IncrementingValues)
{
    EXPECT_EQ(static_cast<int>(Types::WidgetDayView), 0);
    EXPECT_EQ(static_cast<int>(Types::WidgetImportedView), 1);
}

// ---- Q_ENUMS reflection: ItemType ----

TEST_F(TypesTest, QEnums_ItemType_Registered)
{
    assertEnumRegistered("ItemType");
}

TEST_F(TypesTest, QEnums_ItemType_KeyValueMapping)
{
    assertEnumKeyValue("ItemType", "All", 0);
    assertEnumKeyValue("ItemType", "Picture", 1);
    assertEnumKeyValue("ItemType", "Video", 2);
}

// ---- Q_ENUMS reflection: ModelType ----

TEST_F(TypesTest, QEnums_ModelType_Registered)
{
    assertEnumRegistered("ModelType");
}

TEST_F(TypesTest, QEnums_ModelType_KeyValueMapping)
{
    assertEnumKeyValue("ModelType", "Normal", 0);
    assertEnumKeyValue("ModelType", "ClassificationDetail", 8);
}

// ---- Q_ENUMS reflection: ThumbnailViewType ----

TEST_F(TypesTest, QEnums_ThumbnailViewType_Registered)
{
    assertEnumRegistered("ThumbnailViewType");
}

TEST_F(TypesTest, QEnums_ThumbnailViewType_KeyValueMapping)
{
    assertEnumKeyValue("ThumbnailViewType", "ViewImport", 0);
    assertEnumKeyValue("ThumbnailViewType", "ViewClassificationDetail", 10);
}

// ---- Q_ENUMS reflection: ImageType ----

TEST_F(TypesTest, QEnums_ImageType_Registered)
{
    assertEnumRegistered("ImageType");
}

TEST_F(TypesTest, QEnums_ImageType_KeyValueMapping)
{
    assertEnumKeyValue("ImageType", "NullImage", 0);
    assertEnumKeyValue("ImageType", "DamagedImage", 5);
}

// ---- Q_ENUMS reflection: MenuItemId ----

TEST_F(TypesTest, QEnums_MenuItemId_Registered)
{
    assertEnumRegistered("MenuItemId");
}

TEST_F(TypesTest, QEnums_MenuItemId_KeyValueMapping)
{
    assertEnumKeyValue("MenuItemId", "IdView", 0);
    assertEnumKeyValue("MenuItemId", "IdTrashRecovery", 22);
}

// ---- Q_ENUMS reflection: SwitchType ----

TEST_F(TypesTest, QEnums_SwitchType_Registered)
{
    assertEnumRegistered("SwitchType");
}

TEST_F(TypesTest, QEnums_SwitchType_KeyValueMapping)
{
    assertEnumKeyValue("SwitchType", "UnknownSwitchType", -1);
    assertEnumKeyValue("SwitchType", "ScaleInOUt", 3);
}

// ---- Q_ENUMS reflection: WidgetViewType ----

TEST_F(TypesTest, QEnums_WidgetViewType_Registered)
{
    assertEnumRegistered("WidgetViewType");
}

TEST_F(TypesTest, QEnums_WidgetViewType_KeyValueMapping)
{
    assertEnumKeyValue("WidgetViewType", "WidgetViewUnknown", -1);
    assertEnumKeyValue("WidgetViewType", "WidgetDayView", 0);
    assertEnumKeyValue("WidgetViewType", "WidgetImportedView", 1);
}

// ---- Q_ENUMS reflection: ThumbnailType ----

TEST_F(TypesTest, QEnums_ThumbnailType_Registered)
{
    assertEnumRegistered("ThumbnailType");
}

TEST_F(TypesTest, QEnums_ThumbnailType_KeyValueMapping)
{
    assertEnumKeyValue("ThumbnailType", "ThumbnailNormal", 0);
    assertEnumKeyValue("ThumbnailType", "ThumbnailDevice", 6);
}

// ---- Q_ENUMS reflection: RectScrollDirType ----

TEST_F(TypesTest, QEnums_RectScrollDirType_Registered)
{
    assertEnumRegistered("RectScrollDirType");
}

TEST_F(TypesTest, QEnums_RectScrollDirType_KeyValueMapping)
{
    assertEnumKeyValue("RectScrollDirType", "NoType", 0);
    assertEnumKeyValue("RectScrollDirType", "ToTop", 1);
    assertEnumKeyValue("RectScrollDirType", "ToBottom", 2);
}

// ---- Q_ENUMS reflection: FileDeleteType ----

TEST_F(TypesTest, QEnums_FileDeleteType_Registered)
{
    assertEnumRegistered("FileDeleteType");
}

TEST_F(TypesTest, QEnums_FileDeleteType_KeyValueMapping)
{
    assertEnumKeyValue("FileDeleteType", "TrashNormal", 0);
    assertEnumKeyValue("FileDeleteType", "TrashSel", 1);
    assertEnumKeyValue("FileDeleteType", "TrashAll", 2);
}

// ---- Q_ENUMS reflection: StackPage ----

TEST_F(TypesTest, QEnums_StackPage_Registered)
{
    assertEnumRegistered("StackPage");
}

TEST_F(TypesTest, QEnums_StackPage_KeyValueMapping)
{
    assertEnumKeyValue("StackPage", "OpenImagePage", 0);
    assertEnumKeyValue("StackPage", "ImageViewPage", 1);
    assertEnumKeyValue("StackPage", "SliderShowPage", 2);
}
