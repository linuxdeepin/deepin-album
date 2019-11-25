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
#include <dgiomount.h>
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
    explicit ImageLoader(Application* parent, QStringList pathlist, QStringList pathlisttrash);

    void addImageLoader(QStringList pathlist);
    void updateImageLoader(QStringList pathlist);

    void addTrashImageLoader(QStringList trashpathlist);
    void updateTrashImageLoader(QStringList trashpathlist);

//private:
//    bool findPicturePathByPhone(QString &path);

public slots:
    void startLoading();
//    void onLoadMountImagesStart(QString mountName, QString path);

signals:
    void sigFinishiLoad();

private:
    Application* m_parent;


    QStringList m_pathlist;
    QStringList m_pathlisttrash;

    QStringList m_phoneImgPathList;
};

class Application : public DApplication {
    Q_OBJECT

public:
    Application(int& argc, char** argv);
    ~Application();

    ConfigSetter *setter = nullptr;
    SignalManager *signalM = nullptr;
    ViewerThemeManager *viewerTheme = nullptr;
    WallpaperSetter *wpSetter = nullptr;

    QMap<QString, QPixmap> m_imagemap;
    QMap<QString, QPixmap> m_imagetrashmap;
    ImageLoader* m_imageloader;
    void LoadDbImage();

//    QMap<QString, QStringList> m_phoneNameAndPathlist;
//    QMap<QString, QPixmap> m_phonePathAndImage;

signals:
    void sigstartLoad();
    void sigFinishLoad();
//    void sigLoadMountImagesStart(QString mountName, QString path);

public slots:
    void finishLoadSlot();
private:
    void initChildren();
    void initI18n();
    void initDB();

    QThread * m_LoadThread;

};

#endif  // APPLICATION_H_
