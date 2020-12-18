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
#include <QCommandLineParser>
#include <stub-tool/cpp-stub/stub.h>
#include <stub-tool/stub-ext/stubext.h>

struct CMOption {
    QString shortOption;
    QString longOption;
    QString description;
    QString valueName;
};

TEST(CommandLine, test_CommandLine)
{
    qDebug() << "CommandLine test_CommandLine count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    CommandLine::instance();
    QString path = DBManager::instance()->getAllPaths().first();
    QUrl UrlInfo1(QString path);

}

TEST(CommandLine, urltest)
{
    qDebug() << "CommandLine urltest count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    stub_ext::StubExt stu;
    stu.set_lamda(ADDR(QCommandLineParser, addOption), [](){
        return false;
    });
    CMOption option;
    CommandLine::instance()->addOption(&option);
}

TEST(CommandLine, viewImage_test)
{
    qDebug() << "CommandLine viewImage_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    QStringList paths = DBManager::instance()->getAllPaths();
    CommandLine::instance()->viewImage(paths.first(), paths);
    QTest::qWait(500);
    w->allPicBtnClicked();
    QTest::qWait(500);
}

TEST(CommandLine, processOption_test)
{
    qDebug() << "CommandLine processOption_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
//    stub_ext::StubExt stu;
//    stu.set_lamda(ADDR(QCommandLineParser, addOption), [](){
//        return false;
//    });
//    CMOption option;
    QStringList paths = DBManager::instance()->getAllPaths();
    CommandLine::instance()->processOption(paths);
    QTest::qWait(500);
    w->allPicBtnClicked();
    QTest::qWait(500);
}
