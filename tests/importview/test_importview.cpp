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

#include "mainwindow.h"
#include "albumcreatedialog.h"
#include "test_qtestDefine.h"
#include "imginfodialog.h"
#include "imageengine/imageengineapi.h"
#include <stub-tool/cpp-stub/stub.h>
#include <stub-tool/stub-ext/stubext.h>


TEST(ImportView, test_onImprotBtnClicked)
{
    qDebug() << "ImportView test_onImprotBtnClicked count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    int (*dlgexec)() = [](){return 1;};
    typedef int (*fptr)(QDialog*);
    fptr fptrexec = (fptr)(&QDialog::exec);   //obtaining an address
    Stub stub;
    stub.set(fptrexec, dlgexec);

    stub_ext::StubExt stu;
    stu.set_lamda(ADDR(DFileDialog, selectedFiles), [](){
        QStringList filelist;
        filelist << ":/2e5y8y.jpg" << ":/2ejqyx.jpg" << ":/2k9o1m.png";
        return filelist;
    });
    stu.set_lamda(ADDR(ImageEngineApi, ImportImagesFromFileList), [](){
        QStringList filelist;
        filelist << ":/2e5y8y.jpg" << ":/2ejqyx.jpg" << ":/2k9o1m.png";
        return true;
    });

    QStringList paths;
    paths << ":/3kp6yv.jpg";
    w->m_pAllPicView->m_pImportView->onImprotBtnClicked(true, paths);
    QTest::qWait(500);
}


TEST(ImportView, test_onImprotBtnClicked_empty)
{
    qDebug() << "ImportView test_onImprotBtnClicked_empty count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    int (*dlgexec)() = [](){return 1;};
    typedef int (*fptr)(QDialog*);
    fptr fptrexec = (fptr)(&QDialog::exec);   //obtaining an address
    Stub stub;
    stub.set(fptrexec, dlgexec);

    stub_ext::StubExt stu;
    stu.set_lamda(ADDR(DFileDialog, selectedFiles), [](){
        QStringList filelist;
        return filelist;
    });
    stu.set_lamda(ADDR(ImageEngineApi, ImportImagesFromFileList), [](){
        QStringList filelist;
        filelist << ":/2e5y8y.jpg" << ":/2ejqyx.jpg" << ":/2k9o1m.png";
        return true;
    });

    QStringList paths;
    w->m_pAllPicView->m_pImportView->onImprotBtnClicked(false, paths);
    QTest::qWait(500);
}
