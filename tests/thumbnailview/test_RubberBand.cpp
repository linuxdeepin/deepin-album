// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QApplication>
#include <QQuickWindow>
#include <QQuickItem>
#include <QImage>
#include <QPainter>
#include <QStyleOptionRubberBand>
#include <memory>

#include "thumbnailview/rubberband.h"

#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

ACCESS_PRIVATE_FIELD(RubberBand, QRectF, m_geometry)

class RubberBandTest : public ::testing::Test
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
    std::unique_ptr<QQuickWindow> m_window;
    std::unique_ptr<RubberBand> m_rubberBand;

    void SetUp() override
    {
        stub.clear();
        m_window = std::make_unique<QQuickWindow>();
        m_rubberBand = std::make_unique<RubberBand>(m_window->contentItem());
    }

    void TearDown() override
    {
        m_rubberBand.reset();
        m_window.reset();
        stub.clear();
    }
};

// --- Constructor / Destructor ---

TEST_F(RubberBandTest, ConstructorDestructor_NoCrash)
{
    SUCCEED();
}

// --- intersects() ---

TEST_F(RubberBandTest, Intersects_InitialEmptyGeometry_ReturnsFalse)
{
    QRectF emptyRect;
    EXPECT_FALSE(m_rubberBand->intersects(QRectF(0, 0, 100, 100)));
}

TEST_F(RubberBandTest, Intersects_AfterGeometryChange_Intersecting_ReturnsTrue)
{
    // Trigger geometryChange by setting size/position
    m_rubberBand->setX(0);
    m_rubberBand->setY(0);
    m_rubberBand->setWidth(100);
    m_rubberBand->setHeight(100);

    // Allow Qt to process geometry changes
    QCoreApplication::processEvents();

    QRectF testRect(50, 50, 100, 100);
    EXPECT_TRUE(m_rubberBand->intersects(testRect));
}

TEST_F(RubberBandTest, Intersects_AfterGeometryChange_NonIntersecting_ReturnsFalse)
{
    m_rubberBand->setX(0);
    m_rubberBand->setY(0);
    m_rubberBand->setWidth(50);
    m_rubberBand->setHeight(50);

    QCoreApplication::processEvents();

    QRectF testRect(200, 200, 100, 100);
    EXPECT_FALSE(m_rubberBand->intersects(testRect));
}

TEST_F(RubberBandTest, Intersects_DirectPrivateFieldSet)
{
    // Set m_geometry directly via ACCESS_PRIVATE_FIELD
    auto &geom = access_private_field::RubberBandm_geometry(*m_rubberBand);
    geom = QRectF(0, 0, 100, 100);

    EXPECT_TRUE(m_rubberBand->intersects(QRectF(50, 50, 100, 100)));
    EXPECT_FALSE(m_rubberBand->intersects(QRectF(200, 200, 50, 50)));
}

// --- paint() ---

TEST_F(RubberBandTest, Paint_Offscreen_NoCrash)
{
    // Set a size so paint has a non-empty rect
    m_rubberBand->setWidth(100);
    m_rubberBand->setHeight(100);
    m_rubberBand->setX(0);
    m_rubberBand->setY(0);

    QCoreApplication::processEvents();

    // Create an offscreen image and painter
    QImage image(100, 100, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);

    // Call paint — should not crash
    m_rubberBand->paint(&painter);
    painter.end();

    SUCCEED();
}

TEST_F(RubberBandTest, Paint_Offscreen_NonEmptyResult)
{
    m_rubberBand->setWidth(100);
    m_rubberBand->setHeight(100);
    m_rubberBand->setX(0);
    m_rubberBand->setY(0);

    QCoreApplication::processEvents();

    QImage image(100, 100, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    m_rubberBand->paint(&painter);
    painter.end();

    // Check that the image is not entirely transparent
    bool hasContent = false;
    for (int y = 0; y < image.height() && !hasContent; ++y) {
        for (int x = 0; x < image.width() && !hasContent; ++x) {
            if (image.pixelColor(x, y).alpha() > 0) {
                hasContent = true;
            }
        }
    }
    EXPECT_TRUE(hasContent);
}

