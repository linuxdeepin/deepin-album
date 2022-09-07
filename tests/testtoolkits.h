// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

//工具箱里面放一些常用的公共函数

#include <QString>
#include <QPoint>
#include <QObject>
#include <QApplication>
#include "application.h"
#include <QtConcurrent>
#include <QTestEvent>
#include "mainwindow.h"

class QMenu;
class QWidget;

//主界面切换
void clickToAllPictureView();
void clickToTimelineView();
void clickToAlbumView();

//双击标题栏
void dClickTitleBar();

//相册页面操作（会确认并尝试强制切换到相册页面以保证操作生效）

//左侧侧边栏操作
void clickToImportPage();
void clickToDeletePage();
void clickToFavoritePage();
void clickNewAlbumInAlbumPage(const QString &albumName);
void clickToCustomAlbum(int index); //需要先创建自定义相册

//菜单操作

//弹出菜单
QMenu *runContextMenu(QWidget *w, const QPoint &pos);

//执行动作
void runActionFromMenu(QMenu *menu, const QString &actionName);

//检查数据库
bool checkIfInAlbum(const QString &path, int UID);

//获取指针背后的数据类型（即从T*中把T提出来）
template <typename T>
struct PointerTypeGetter {
    using type = T;
};

template <typename T>
struct PointerTypeGetter<T *> {
    using type = T;
};

class asynchronousObject : public QObject
{
    Q_OBJECT
public:
    explicit asynchronousObject(QObject *parent = nullptr);
public slots:
    void asynchronousRunActionFromMenu(QMenu *menu, QString actionName);
private:

};

//输入：对话框对象、对话框运行时要执行的操作
template<typename T, typename U>
void stubDialog(T &&activeFun, U &&processFun)
{
    qDebug() << __FUNCTION__ << "---";
    activeFun();//启动函数

    QEventLoop loop;
    QtConcurrent::run([ =, &loop]() {
        (void)QTest::qWaitFor([ =, &loop]() {
            return (loop.isRunning());
        });
        (void)QTest::qWaitFor([ = ]() {
            return (qApp->activeModalWidget() != nullptr && qApp->activeModalWidget() != dApp->getMainWindow());
        });
        if (qApp->activeModalWidget() != nullptr) {
            QThread::msleep(200);
            processFun(); //要执行的操作在这里
            QThread::msleep(200);
            QMetaObject::invokeMethod(&loop, "quit");
        } else {
            QMetaObject::invokeMethod(&loop, "quit");
        }
    });
    loop.exec();
}
