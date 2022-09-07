// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef WALLPAPERSETTER_H
#define WALLPAPERSETTER_H

#include <QObject>

class WallpaperSetter : public QObject
{
    Q_OBJECT
public:
    static WallpaperSetter *instance();
    //void setWallpaper(const QString &path);
    bool setBackground(const QString &pictureFilePath);
private:
    enum DE {
        Deepin,
        GNOME,
        GNOMEShell,
        KDE,
        LXDE,
        Xfce,
        MATE,
        OthersDE
    };

    explicit WallpaperSetter(QObject *parent = nullptr);

private:
    static WallpaperSetter *m_setter;
};

#endif // WALLPAPERSETTER_H
