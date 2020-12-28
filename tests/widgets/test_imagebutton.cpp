#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <QMap>
#include <DFileDialog>
#include <QTestEventList>
#include <QObject>
#include <QDialog>
#include <QStringList>
#include <DSearchEdit>
#include <QFileInfo>

#define private public
#define protected public

#include "mainwindow.h"
#include "albumcreatedialog.h"
#include "imagebutton.h"
#include "test_qtestDefine.h"
#include "imginfodialog.h"
#include <stub-tool/cpp-stub/stub.h>
#include <stub-tool/stub-ext/stubext.h>

TEST(imagebutton, showTooltip_test)
{
    TEST_CASE_NAME("showTooltip_test")
    qDebug() << "imagebutton showTooltip_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    stub_ext::StubExt stu;
    stu.set_lamda(ADDR(QFrame, show), [](){
        return;
    });

    QPoint point(10,10);
    ImageButton *imgbtn = new ImageButton();
    imgbtn->showTooltip(point);
    QTest::qWait(500);
}
