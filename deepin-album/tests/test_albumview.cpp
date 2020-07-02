#include "test_albumview.h"

TEST_F(test_albumview, testIni)
{
    if (nullptr == albumview) {
        albumview = new AlbumView;
    }
    ASSERT_TRUE(albumview->m_pRightThumbnailList);
}

TEST_F(test_albumview, testCreatAlbumBtn)
{
    QTest::mouseClick(albumview->m_pLeftListView->m_pAddListBtn, Qt::LeftButton);

}
