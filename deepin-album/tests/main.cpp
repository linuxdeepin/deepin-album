#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "albumview.h"

int main(int argc, char *argv[])
{
    Application app(argc, argv);
    testing::InitGoogleTest(&argc, argv);
    int RUN_ALL_TESTS_RES = RUN_ALL_TESTS();
    Q_UNUSED(RUN_ALL_TESTS_RES);


    //AlbumView *albumview = new AlbumView;
    //albumview->show();

    return app.exec();
}
