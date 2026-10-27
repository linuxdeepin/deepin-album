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
 *   Q_ENUMS reflection ItemType:
 *     E1: 枚举已注册 (metaEnum.isValid())
 *     E2: 键名与键值一一对应 (All=0)
 *   Q_ENUMS reflection ModelType:
 *     E3: 枚举已注册
 *     E4: 键名与键值一一对应 (Normal=0, ClassificationDetail=8)
 *   Q_ENUMS reflection ThumbnailViewType:
 *     E5: 枚举已注册
 *     E6: 键名与键值一一对应 (ViewImport=0, ViewClassificationDetail=10)
 *   Q_ENUMS reflection ImageType:
 *     E7: 枚举已注册
 *     E8: 键名与键值一一对应 (DamagedImage=5)
 *   Q_ENUMS reflection MenuItemId:
 *     E9: 枚举已注册
 *     E10: 键名与键值一一对应 (IdTrashRecovery=22)
 *   Q_ENUMS reflection SwitchType:
 *     E11: 枚举已注册
 *     E12: 键名与键值一一对应 (UnknownSwitchType=-1)
 *   Q_ENUMS reflection WidgetViewType:
 *     E13: 枚举已注册
 *     E14: 键名与键值一一对应 (WidgetViewUnknown=-1)
 */

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QMetaEnum>
#include <QMetaObject>

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

// ---- Constructor / Destructor ----

TEST_F(TypesTest, Constructor_DoesNotCrash)
{
    Types *t = new Types();
    ASSERT_NE(nullptr, t);
    delete t;
}

TEST_F(TypesTest, Destructor_DoesNotCrash)
{
    Types *t = new Types();
    EXPECT_NO_THROW(delete t);
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
    const QMetaObject *meta = &Types::staticMetaObject;
    int idx = meta->indexOfEnumerator("ItemType");
    ASSERT_GE(idx, 0);
    QMetaEnum me = meta->enumerator(idx);
    EXPECT_TRUE(me.isValid());
}

TEST_F(TypesTest, QEnums_ItemType_KeyValueMapping)
{
    const QMetaObject *meta = &Types::staticMetaObject;
    int idx = meta->indexOfEnumerator("ItemType");
    ASSERT_GE(idx, 0);
    QMetaEnum me = meta->enumerator(idx);
    ASSERT_TRUE(me.isValid());
    EXPECT_EQ(me.keyToValue("All"), 0);
    EXPECT_EQ(me.keyToValue("Picture"), 1);
    EXPECT_EQ(me.keyToValue("Video"), 2);
    EXPECT_STREQ(me.valueToKey(0), "All");
}

// ---- Q_ENUMS reflection: ModelType ----

TEST_F(TypesTest, QEnums_ModelType_Registered)
{
    const QMetaObject *meta = &Types::staticMetaObject;
    int idx = meta->indexOfEnumerator("ModelType");
    ASSERT_GE(idx, 0);
    QMetaEnum me = meta->enumerator(idx);
    EXPECT_TRUE(me.isValid());
}

TEST_F(TypesTest, QEnums_ModelType_KeyValueMapping)
{
    const QMetaObject *meta = &Types::staticMetaObject;
    int idx = meta->indexOfEnumerator("ModelType");
    ASSERT_GE(idx, 0);
    QMetaEnum me = meta->enumerator(idx);
    ASSERT_TRUE(me.isValid());
    EXPECT_EQ(me.keyToValue("Normal"), 0);
    EXPECT_EQ(me.keyToValue("ClassificationDetail"), 8);
    EXPECT_STREQ(me.valueToKey(0), "Normal");
}

// ---- Q_ENUMS reflection: ThumbnailViewType ----

TEST_F(TypesTest, QEnums_ThumbnailViewType_Registered)
{
    const QMetaObject *meta = &Types::staticMetaObject;
    int idx = meta->indexOfEnumerator("ThumbnailViewType");
    ASSERT_GE(idx, 0);
    QMetaEnum me = meta->enumerator(idx);
    EXPECT_TRUE(me.isValid());
}

TEST_F(TypesTest, QEnums_ThumbnailViewType_KeyValueMapping)
{
    const QMetaObject *meta = &Types::staticMetaObject;
    int idx = meta->indexOfEnumerator("ThumbnailViewType");
    ASSERT_GE(idx, 0);
    QMetaEnum me = meta->enumerator(idx);
    ASSERT_TRUE(me.isValid());
    EXPECT_EQ(me.keyToValue("ViewImport"), 0);
    EXPECT_EQ(me.keyToValue("ViewClassificationDetail"), 10);
    EXPECT_STREQ(me.valueToKey(0), "ViewImport");
}

// ---- Q_ENUMS reflection: ImageType ----

TEST_F(TypesTest, QEnums_ImageType_Registered)
{
    const QMetaObject *meta = &Types::staticMetaObject;
    int idx = meta->indexOfEnumerator("ImageType");
    ASSERT_GE(idx, 0);
    QMetaEnum me = meta->enumerator(idx);
    EXPECT_TRUE(me.isValid());
}

TEST_F(TypesTest, QEnums_ImageType_KeyValueMapping)
{
    const QMetaObject *meta = &Types::staticMetaObject;
    int idx = meta->indexOfEnumerator("ImageType");
    ASSERT_GE(idx, 0);
    QMetaEnum me = meta->enumerator(idx);
    ASSERT_TRUE(me.isValid());
    EXPECT_EQ(me.keyToValue("DamagedImage"), 5);
    EXPECT_STREQ(me.valueToKey(5), "DamagedImage");
    EXPECT_EQ(me.keyToValue("NullImage"), 0);
}

// ---- Q_ENUMS reflection: MenuItemId ----

TEST_F(TypesTest, QEnums_MenuItemId_Registered)
{
    const QMetaObject *meta = &Types::staticMetaObject;
    int idx = meta->indexOfEnumerator("MenuItemId");
    ASSERT_GE(idx, 0);
    QMetaEnum me = meta->enumerator(idx);
    EXPECT_TRUE(me.isValid());
}

TEST_F(TypesTest, QEnums_MenuItemId_KeyValueMapping)
{
    const QMetaObject *meta = &Types::staticMetaObject;
    int idx = meta->indexOfEnumerator("MenuItemId");
    ASSERT_GE(idx, 0);
    QMetaEnum me = meta->enumerator(idx);
    ASSERT_TRUE(me.isValid());
    EXPECT_EQ(me.keyToValue("IdTrashRecovery"), 22);
    EXPECT_STREQ(me.valueToKey(22), "IdTrashRecovery");
    EXPECT_EQ(me.keyToValue("IdView"), 0);
}

// ---- Q_ENUMS reflection: SwitchType ----

TEST_F(TypesTest, QEnums_SwitchType_Registered)
{
    const QMetaObject *meta = &Types::staticMetaObject;
    int idx = meta->indexOfEnumerator("SwitchType");
    ASSERT_GE(idx, 0);
    QMetaEnum me = meta->enumerator(idx);
    EXPECT_TRUE(me.isValid());
}

TEST_F(TypesTest, QEnums_SwitchType_KeyValueMapping)
{
    const QMetaObject *meta = &Types::staticMetaObject;
    int idx = meta->indexOfEnumerator("SwitchType");
    ASSERT_GE(idx, 0);
    QMetaEnum me = meta->enumerator(idx);
    ASSERT_TRUE(me.isValid());
    EXPECT_EQ(me.keyToValue("UnknownSwitchType"), -1);
    EXPECT_STREQ(me.valueToKey(-1), "UnknownSwitchType");
}

// ---- Q_ENUMS reflection: WidgetViewType ----

TEST_F(TypesTest, QEnums_WidgetViewType_Registered)
{
    const QMetaObject *meta = &Types::staticMetaObject;
    int idx = meta->indexOfEnumerator("WidgetViewType");
    ASSERT_GE(idx, 0);
    QMetaEnum me = meta->enumerator(idx);
    EXPECT_TRUE(me.isValid());
}

TEST_F(TypesTest, QEnums_WidgetViewType_KeyValueMapping)
{
    const QMetaObject *meta = &Types::staticMetaObject;
    int idx = meta->indexOfEnumerator("WidgetViewType");
    ASSERT_GE(idx, 0);
    QMetaEnum me = meta->enumerator(idx);
    ASSERT_TRUE(me.isValid());
    EXPECT_EQ(me.keyToValue("WidgetViewUnknown"), -1);
    EXPECT_STREQ(me.valueToKey(-1), "WidgetViewUnknown");
    EXPECT_EQ(me.keyToValue("WidgetDayView"), 0);
    EXPECT_EQ(me.keyToValue("WidgetImportedView"), 1);
}

// ---- Additional Q_ENUMS reflection checks (ThumbnailType, RectScrollDirType, FileDeleteType, StackPage) ----

TEST_F(TypesTest, QEnums_ThumbnailType_Registered)
{
    const QMetaObject *meta = &Types::staticMetaObject;
    int idx = meta->indexOfEnumerator("ThumbnailType");
    ASSERT_GE(idx, 0);
    QMetaEnum me = meta->enumerator(idx);
    EXPECT_TRUE(me.isValid());
}

TEST_F(TypesTest, QEnums_RectScrollDirType_Registered)
{
    const QMetaObject *meta = &Types::staticMetaObject;
    int idx = meta->indexOfEnumerator("RectScrollDirType");
    ASSERT_GE(idx, 0);
    QMetaEnum me = meta->enumerator(idx);
    EXPECT_TRUE(me.isValid());
}

TEST_F(TypesTest, QEnums_FileDeleteType_Registered)
{
    const QMetaObject *meta = &Types::staticMetaObject;
    int idx = meta->indexOfEnumerator("FileDeleteType");
    ASSERT_GE(idx, 0);
    QMetaEnum me = meta->enumerator(idx);
    EXPECT_TRUE(me.isValid());
}

TEST_F(TypesTest, QEnums_StackPage_Registered)
{
    const QMetaObject *meta = &Types::staticMetaObject;
    int idx = meta->indexOfEnumerator("StackPage");
    ASSERT_GE(idx, 0);
    QMetaEnum me = meta->enumerator(idx);
    EXPECT_TRUE(me.isValid());
}
