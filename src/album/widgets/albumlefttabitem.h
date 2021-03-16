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
#ifndef ALBUMLEFTTABITEM_H
#define ALBUMLEFTTABITEM_H

#include <QWidget>
#include <DLabel>
#include <DLineEdit>
#include <DApplicationHelper>
#include <DIconButton>
#include "albumview/leftlistwidget.h"
#include "mountexternalbtn.h"
#include "widgets/ddlabel.h"

DWIDGET_USE_NAMESPACE

class AlbumLeftTabItem : public QWidget
{
    Q_OBJECT
public:
    AlbumLeftTabItem(QString str, QString strAlbumType = "");
    ~AlbumLeftTabItem();
    void editAlbumEdit();
    void oriAlbumStatus();
    void newAlbumStatus();
    void setExternalDevicesMountPath(QString strPath);
    QString getalbumname();

private:
    void initConnections();
    void initUI();
    void unMountBtnClicked();
signals:
    void editingFinished();
    void unMountExternalDevices(QString mountName);
public slots:
    void onCheckNameValid();
public:
    QString m_albumNameStr;
    QString m_albumTypeStr;
    int m_opeMode;
    DLineEdit *m_pLineEdit;
    DLineEdit *m_pNewLineEdit;
    QString m_mountPath;
private:
    DDlabel *m_nameLabel;
    DLabel *pImageLabel;
    MountExternalBtn *m_unMountBtn;
    LeftListWidget *m_pListWidget;
    QListWidgetItem *m_pListWidgetItem;
};

#endif // ALBUMLEFTTABITEM_H
