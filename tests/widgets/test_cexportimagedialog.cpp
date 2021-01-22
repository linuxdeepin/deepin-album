#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include <QMap>
#include <QTestEventList>
#include <QObject>
#include <QDialog>
#include <QStringList>
#include <QFileInfo>

#include <DSearchEdit>
#include <DFileDialog>

#define private public
#define protected public

#include "mainwindow.h"
#include "albumcreatedialog.h"
#include "test_qtestDefine.h"
#include "imginfodialog.h"
#include "ac-desktop-define.h"

#include <stub-tool/cpp-stub/stub.h>
#include <stub-tool/stub-ext/stubext.h>

TEST(CExportImageDialog, showQuestionDialog_test)
{
    TEST_CASE_NAME("showQuestionDialog_test")
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    CExportImageDialog *expdlg0 = new CExportImageDialog;

    int (*dlgexec)() = []() {return 1;};
    typedef int (*fptr)(QDialog *);
    fptr fptrexec = (fptr)(&QDialog::exec);   //obtaining an address
    Stub stub;
    stub.set(fptrexec, dlgexec);

    QString path;
    if (DBManager::instance()->getAllPaths().length() > 0)
        path = DBManager::instance()->getAllPaths().first();
    else
        path = testPath_test + "/2e5y8y.jpg";
    QString srcpath = path;
    expdlg0->showQuestionDialog(path, srcpath);
}

//
TEST(CExportImageDialog, showDirChoseDialog_test)
{
    TEST_CASE_NAME("showDirChoseDialog_test")
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    int (*dlgexec)() = []() {return 1;};
    typedef int (*fptr)(QDialog *);
    fptr fptrexec = (fptr)(&QDialog::exec);   //obtaining an address
    Stub stub;
    stub.set(fptrexec, dlgexec);

    stub_ext::StubExt stu;
    stu.set_lamda(ADDR(DFileDialog, selectedFiles), []() {
        QStringList filelist;
        filelist << ":/2e5y8y.jpg" << ":/2ejqyx.jpg" << ":/2k9o1m.png";
        return filelist;
    });

    CExportImageDialog *expdlg1 = new CExportImageDialog;

    expdlg1->showDirChoseDialog();
}

TEST(CExportImageDialog, showEmptyWarningDialog_test)
{
    TEST_CASE_NAME("showEmptyWarningDialog_test")
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    CExportImageDialog *expdlg2 = new CExportImageDialog;

    expdlg2->showEmptyWarningDialog();
}
TEST(CExportImageDialog, CExportImageDialog_func)
{
    TEST_CASE_NAME("CExportImageDialog_func")
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    CExportImageDialog *expdlg3 = new CExportImageDialog;

    expdlg3->slotOnQualityChanged(1);

    int index = 0;
    QString text = "hello";
    stub_ext::StubExt stu;
    stu.set_lamda(ADDR(CExportImageDialog, doSave), []() {
        return true;
    });

    expdlg3->slotOnQuestionDialogButtonClick(index, text);
}

TEST(CExportImageDialog, slotOnDialogButtonClick_test)
{
    TEST_CASE_NAME("slotOnDialogButtonClick_test")
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    CExportImageDialog *expdlg4 = new CExportImageDialog;

    int index = 1;
    QString text = "hello";
    stub_ext::StubExt stu;
    stu.set_lamda(ADDR(CExportImageDialog, showDirChoseDialog), []() {
        return;
    });

    expdlg4->slotOnDialogButtonClick(index, text);
    QTest::qWait(50);
    expdlg4->setGifType(text);
    QTest::qWait(50);
    expdlg4->removeGifType();
    QTest::qWait(50);

    for (int i = 0; i < 8; i++) {
        expdlg4->slotOnSavePathChange(i);
    }
    QTest::qWait(500);
}

TEST(CExportImageDialog, doSave_test)
{
    TEST_CASE_NAME("doSave_test")
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    CExportImageDialog *expdlg = new CExportImageDialog;

    stub_ext::StubExt stu;
    stu.set_lamda(ADDR(QComboBox, currentText), []() {
        return "gif";
    });

    expdlg->doSave();
}
