/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     ZhangYong <zhangyong@uniontech.com>
 *
 * Maintainer: ZhangYong <ZhangYong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <DApplication>
#include <QThread>
#include <QSharedMemory>
#include <dgiomount.h>
#include "dbmanager/dbmanager.h"

class MainWindow;
class Application;
class ConfigSetter;
class DatabaseManager;
class DBManager;
class Exporter;
class Importer;
class ScanPathsDialog;
class SignalManager;
class WallpaperSetter;
class ViewerThemeManager;
#if defined(dApp)
#undef dApp
#endif
//#define dApp (static_cast<Application *>(QCoreApplication::instance()))
#define dApp (Application::getApp())

DWIDGET_USE_NAMESPACE

class ImageLoader : public QObject
{
    Q_OBJECT
public:
    explicit ImageLoader(/*Application *parent, QStringList pathlist, QStringList pathlisttrash*/);

    void ImportImageLoader(const DBImgInfoList &dbInfos);
    void updateImageLoader(const QStringList &pathlist, const QList<QImage> &images = QList<QImage>());

signals:
    void sigFinishiLoad();

private:
//    Application *m_parent;
//    QStringList m_pathlist;
//    QStringList m_pathlisttrash;
};

//使用DTK推荐的方式实现退出效果，以尝试解决崩溃日志问题
//由于下方Application中和DApplication的关系是has a，因此直接重新定义一个基于DApplication的类，而不是改造Application
class CustomDApplication : public DApplication
{
public:
    CustomDApplication(int &argc, char *argv[]);
    void setMainWindow(MainWindow *newWindow);
protected:
    void handleQuitAction() override;

private:
    MainWindow *window = nullptr;
};

class Application : public QObject
{
    Q_OBJECT

public:
    Application(/*int &argc, char **argv*/);
    ~Application();

    DApplication *getDAppNew();
    static Application *getApp();
    static void setApp(DApplication *);

    ConfigSetter *setter = nullptr;
    SignalManager *signalM = nullptr;
    ViewerThemeManager *viewerTheme = nullptr;
    WallpaperSetter *wpSetter = nullptr;

    ImageLoader *m_imageloader;

    static bool isWaylandPlatform();
signals:
    void sigstartLoad();

public slots:

private:
    void initChildren();

    QThread *m_LoadThread;
public :
    //LMH0420设置单例程序
    void setupsinglecase();
    //LMH0420是否已有相同程序运行
    bool isRunning();
    //打印信息
    bool sendMessage(const QString &message);

    //test
    void setMainWindow(MainWindow *window);
    MainWindow *getMainWindow();
public slots:
    void checkForMessage();
signals:
    void messageAvailable(QString message);
private:
    bool _isRunning;
    QSharedMemory sharedMemory;
    MainWindow *m_mainwindow;
    static DApplication *dAppNew;
    static Application *dApp1;
};

#endif  // APPLICATION_H_
