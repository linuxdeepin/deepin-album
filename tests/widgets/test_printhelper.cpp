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
#include <dprintpreviewdialog.h>
#include <dprintpreviewwidget.h>
#include <QMap>
#include <DFileDialog>
#include <QTestEventList>
#include <QObject>
#include <QDialog>
#include <QStringList>
#include <DSearchEdit>

#define private public
#define protected public

#include "mainwindow.h"
#include "albumcreatedialog.h"
#include "test_qtestDefine.h"
#include "imginfodialog.h"
#include "printhelper.h"
//#include "printoptionspage.h"
#include "utils/unionimage.h"
#include <stub-tool/cpp-stub/stub.h>
#include <stub-tool/stub-ext/stubext.h>

TEST(PrintHelper, PrintHelper_test)
{
    qDebug() << "PrintHelper PrintHelper_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    int (*dlgexec)() = [](){return 1;};
    typedef int (*fptr)(QDialog*);
    fptr fptrexec = (fptr)(&QDialog::exec);   //obtaining an address
    Stub stub;
    stub.set(fptrexec, dlgexec);

    QStringList paths = DBManager::instance()->getAllPaths();
    QWidget *p = nullptr;

    PrintHelper::showPrintDialog(paths, p);
}


