#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <QMap>
#include <DFileDialog>
#include <QTestEventList>
#include <QObject>
#include <QDialog>
#include <QStringList>
#include <DSearchEdit>

#define private public
#define protected public

#include "wallpapersetter.h"
#include "mainwindow.h"
#include "albumcreatedialog.h"
#include "test_qtestDefine.h"
#include "imginfodialog.h"
#include <stub-tool/cpp-stub/stub.h>
#include <stub-tool/stub-ext/stubext.h>


// 从菜单导入照片
TEST(WallpaperSetter, test_WallpaperSetter)
{
    TEST_CASE_NAME("test_WallpaperSetter")
    qDebug() << "WallpaperSetter test_WallpaperSetter count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    WallpaperSetter::instance();
    QTest::qWait(500);
}

// 从菜单导入照片
TEST(WallpaperSetter, setBackground_test)
{
    TEST_CASE_NAME("setBackground_test")
    qDebug() << "WallpaperSetter setBackground_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);
    QString path;
    if (DBManager::instance()->getAllPaths().length() > 0)
        path = DBManager::instance()->getAllPaths().first();
    else
        path = testPath_test + "/2e5y8y.jpg";
    WallpaperSetter::instance()->setBackground(path);
    QTest::qWait(500);
}

