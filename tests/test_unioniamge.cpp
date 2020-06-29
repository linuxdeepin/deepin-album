
#include "gtest/gtest.h"
#include "../album/utils/unionimage.h"
#include <QImage>
#include <QPixmap>

TEST(UnionMovieImage, setFileName)
{
    using namespace  UnionImage_NameSpace;
    UnionMovieImage m_movie;
    m_movie.setFileName("/home/ut-djh/Pictures/截图录屏_选择区域_20200520193705.gif");
    QImage testimage;
    testimage = m_movie++;
    ASSERT_FALSE(testimage.isNull());
}
