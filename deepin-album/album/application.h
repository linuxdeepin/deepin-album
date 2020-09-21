/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
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
#define dApp (static_cast<Application *>(QCoreApplication::instance()))

DWIDGET_USE_NAMESPACE

class ImageLoader : public QObject
{
    Q_OBJECT
public:
    explicit ImageLoader(Application *parent, QStringList pathlist, QStringList pathlisttrash);

    void ImportImageLoader(DBImgInfoList dbInfos, QString albumname = nullptr);
    void updateImageLoader(QStringList pathlist);

signals:
    void sigFinishiLoad();

private:
    Application *m_parent;
    QStringList m_pathlist;
    QStringList m_pathlisttrash;
};

class Application : public DApplication
{
    Q_OBJECT

public:
    Application(int &argc, char **argv);
    ~Application();

    ConfigSetter *setter = nullptr;
    SignalManager *signalM = nullptr;
    ViewerThemeManager *viewerTheme = nullptr;
    WallpaperSetter *wpSetter = nullptr;

//    QMap<QString, QPixmap> m_imagemap;
//    QMap<QString, QPixmap> m_imagetrashmap;
    ImageLoader *m_imageloader;
//    void LoadDbImage();

//    QMap<QString, QStringList> m_phoneNameAndPathlist;
//    QMap<QString, QPixmap> m_phonePathAndImage;

    static bool isWaylandPlatform();
signals:
    void sigstartLoad();
    void sigFinishLoad();
//    void sigLoadMountImagesStart(QString mountName, QString path);

public slots:
    void finishLoadSlot();
private:
    void initChildren();
    void initI18n();

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
};

#endif  // APPLICATION_H_
