// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QScopedPointer>
#include <QMap>
#include <QVariant>

#include <DPushButton>

DWIDGET_USE_NAMESPACE

class AlbumImageButton : public DPushButton
{
    struct MusicPicPathInfo {
        QString normalPicPath;
        QString hoverPicPath;
        QString pressPicPath;
        QString checkedPicPath;
    };
    Q_OBJECT
public:
    explicit AlbumImageButton(QWidget *parent = Q_NULLPTR);
    AlbumImageButton(const QString &normalPic, const QString &hoverPic,
                     const QString &pressPic, const QString &checkedPic = QString(), QWidget *parent = nullptr);
    void setPropertyPic(const QString &propertyName, const QVariant &value, const QString &normalPic, const QString &hoverPic,
                        const QString &pressPic, const QString &checkedPic = QString());
    void setPropertyPic(const QString &normalPic, const QString &hoverPic,
                        const QString &pressPic, const QString &checkedPic = QString());
//    void setTransparent(bool flag);
//    void setAutoChecked(bool flag);
protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void enterEvent(QEvent *event) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
private:
    char                                               status;
    bool                                               autoChecked;
    MusicPicPathInfo                                   defaultPicPath;
    bool                                               transparent;
    QPair<QString, QMap<QVariant, MusicPicPathInfo> >  propertyPicPaths;
};
