#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrintPreviewWidget>
#include <QPrinter>
#include <QPainter>
#include <QToolBar>
#include <QCoreApplication>
#include <QImageReader>
#include <QDebug>
#include <QMap>
#include <QTestEventList>
#include <QObject>
#include <QDialog>
#include <QStringList>

#include <DSearchEdit>
#include <DFileDialog>

#include <dprintpreviewdialog.h>
#include <dprintpreviewwidget.h>

#define private public
#define protected public

#include "mainwindow.h"
#include "albumcreatedialog.h"
#include "test_qtestDefine.h"
#include "imginfodialog.h"
#include "printhelper.h"
#include "ac-desktop-define.h"
#include "utils/unionimage.h"

#include <stub-tool/cpp-stub/stub.h>
#include <stub-tool/stub-ext/stubext.h>

TEST(PrintHelper, PrintHelper_test)
{
    TEST_CASE_NAME("PrintHelper_test")
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    int (*dlgexec)() = []() {return 1;};
    typedef int (*fptr)(QDialog *);
    fptr fptrexec = (fptr)(&QDialog::exec);   //obtaining an address
    Stub stub;
    stub.set(fptrexec, dlgexec);

    QStringList paths = DBManager::instance()->getAllPaths();
    QWidget *p = nullptr;

    PrintHelper *p1 = new PrintHelper;
    if (paths.size() < 1)
        paths << testPath_test + "2k9o1m.png";
    PrintHelper::getIntance()->showPrintDialog(paths, p);
}


