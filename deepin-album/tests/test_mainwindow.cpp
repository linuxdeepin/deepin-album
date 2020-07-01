#include <QtTest/QTest>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "mainwindow.h"
#include "application.h"



class MainWindowTest: public ::testing::Test

{
    virtual void SetUp()
    {
        mainwindow = new MainWindow();
    }
    virtual void TearDown()
    {
        delete mainwindow;
    }
    MainWindow *mainwindow;
};


TEST_F(MainWindowTest, SetUp)
{

}
