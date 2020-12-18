//#include <gtest/gtest.h>
//#include <gmock/gmock-matchers.h>
//#include <QMap>
//#include <DFileDialog>
//#include <QTestEventList>
//#include <QObject>
//#include <QDialog>
//#include <QStringList>
//#include <DSearchEdit>
//#include <QFontMetrics>
//#include <QMimeData>
//#include <QUrl>

//#define private public
//#define protected public

//#include "baseutils.h"
//#include "mainwindow.h"
//#include "importtimelineview.h"
//#include "albumcreatedialog.h"
//#include "test_qtestDefine.h"
//#include "imginfodialog.h"
//#include "imageengine/imageengineapi.h"
//#include "formlabel.h"
//#include <stub-tool/cpp-stub/stub.h>
//#include <stub-tool/stub-ext/stubext.h>

//TEST(baseutils, init_insertPhotos)
//{
//    MainWindow *w = dApp->getMainWindow();
//    w->allPicBtnClicked();
//    QTest::qWait(500);

//    int (*dlgexec)() = [](){return 1;};
//    typedef int (*fptr)(QDialog*);
//    fptr fptrexec = (fptr)(&QDialog::exec);   //obtaining an address
//    Stub stub;
//    stub.set(fptrexec, dlgexec);

//    stub_ext::StubExt stu;
//    stu.set_lamda(ADDR(DFileDialog, selectedFiles), [](){
//        QStringList filelist;
//        filelist << ":/2e5y8y.jpg" << ":/2ejqyx.jpg" << ":/2k9o1m.png";
//        return filelist;
//    });
//    QList<QAction*> actions = w->actions();
//    foreach (auto act, actions) {
//        if (act->text() == QObject::tr("Import photos")) {
//            act->trigger();
//            break;
//        }
//    }
//    QTest::qWait(500);
//}

//TEST(baseutils, baseutils_test)
//{
//    MainWindow *w = dApp->getMainWindow();
//    w->allPicBtnClicked();
//    QTest::qWait(500);

//    QString path;
//    path = DBManager::instance()->getAllPaths().first();
//    utils::base::showInFileManager(path);

//    QString text = "hello,world";
//    QString alltext;
//    for (int i =0; i < 100; i++) {
//        alltext += text;
//    }

//    SimpleFormField *field = new SimpleFormField;
//    field->setAlignment(Qt::AlignLeft | Qt::AlignTop);
//    DFontSizeManager::instance()->bind(field, DFontSizeManager::T8);
//    utils::base::SpliteText(alltext, field->font(), 600);
//    QTest::qWait(100);

//    QMimeData mimeData;
//    stub_ext::StubExt stu;
//    stu.set_lamda(ADDR(QMimeData, hasUrls), [](){
//        return true;
//    });

//    QString path2 = DBManager::instance()->getAllPaths().first();
//    QString path3 = "file://" + path2;
//    QUrl url(path3);
//    QList<QUrl> urls;
//    stu.set_lamda(ADDR(QMimeData, urls), [url, &urls](){
//        urls.append(url);
//        return urls;
//    });

//    stu.set_lamda(ADDR(QUrl, toLocalFile), [path2](){
//        return path2;
//    });

//    utils::base::checkMimeData(&mimeData);
//    QTest::qWait(500);

//    stu.set_lamda(ADDR(QUrl, toLocalFile), [path2](){
//        QFileInfo info(path2);
//        QString dirPaht = info.dir().absolutePath();
//        return dirPaht;
//    });
//    utils::base::checkMimeData(&mimeData);
//    QTest::qWait(500);
//}


////TEST(baseutils, test_func)
////{
////    MainWindow *w = dApp->getMainWindow();
////    w->albumBtnClicked();
////    QTest::qWait(500);

////    ImportTimeLineView *impTimeline = w->m_pAlbumview->m_pImpTimeLineWidget;
////    impTimeline->updateChoseText();

////    emit impTimeline->pSuspensionChose->clicked();
////    QTest::qWait(500);

////    impTimeline->clearAndStop();
////    w->allPicBtnClicked();
////    w->albumBtnClicked();
////    QTest::qWait(500);
////}

////TEST(baseutils, on_KeyEvent_test)
////{
////    MainWindow *w = dApp->getMainWindow();
////    w->albumBtnClicked();
////    QTest::qWait(500);

////    ImportTimeLineView *impTimeline = w->m_pAlbumview->m_pImpTimeLineWidget;
////    impTimeline->on_KeyEvent(Qt::Key_PageDown);
////    QTest::qWait(100);
////    impTimeline->on_KeyEvent(Qt::Key_PageUp);
////    QTest::qWait(500);
////}
