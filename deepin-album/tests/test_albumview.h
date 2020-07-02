#ifndef TEST_ALBUMVIEW_H
#define TEST_ALBUMVIEW_H

#include <QtTest/QTest>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "../album/albumview/albumview.h"

class test_albumview : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        albumview = new AlbumView();
    }
    virtual void TearDown()
    {
        delete albumview;
    }
    AlbumView * albumview;
};

#endif // TEST_ALBUMVIEW_H
