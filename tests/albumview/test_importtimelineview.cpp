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
#include "importtimelineview.h"
#include "albumcreatedialog.h"
#include "test_qtestDefine.h"
#include "imginfodialog.h"
#include "imageengine/imageengineapi.h"
#include <stub-tool/cpp-stub/stub.h>
#include <stub-tool/stub-ext/stubext.h>


TEST(ImportTimeLineView, getIBaseHeight_test)
{
    qDebug() << "ImportTimeLineView getIBaseHeight_test count" << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->albumBtnClicked();
    QTest::qWait(500);

    ImportTimeLineView *impTimeline = w->m_pAlbumview->m_pImpTimeLineWidget;
    int a = 0;
    stub_ext::StubExt stu;
    stu.set_lamda(ADDR(DSlider, value), [&a](){
        return a++;
    });
    for (int i = 0; i < 10; i++) {
        impTimeline->getIBaseHeight();
    }
    QTest::qWait(500);
}


TEST(ImportTimeLineView, test_func)
{
    qDebug() << "ImportTimeLineView test_func count" << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->albumBtnClicked();
    QTest::qWait(500);

    ImportTimeLineView *impTimeline = w->m_pAlbumview->m_pImpTimeLineWidget;
    impTimeline->updateChoseText();

    emit impTimeline->pSuspensionChose->clicked();
    QTest::qWait(500);

    impTimeline->clearAndStop();
    w->allPicBtnClicked();
    w->albumBtnClicked();
    QTest::qWait(500);
}

TEST(ImportTimeLineView, on_KeyEvent_test)
{
    qDebug() << "ImportTimeLineView on_KeyEvent_test count" << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->albumBtnClicked();
    QTest::qWait(500);

    ImportTimeLineView *impTimeline = w->m_pAlbumview->m_pImpTimeLineWidget;
    impTimeline->on_KeyEvent(Qt::Key_PageDown);
    QTest::qWait(100);
    impTimeline->on_KeyEvent(Qt::Key_PageUp);
    QTest::qWait(500);
}
