// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
    AlbumLeftTabItem(QString str, int UID, QString strAlbumType = "");
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
    void unMountExternalDevices(const QString &mountName);
public slots:
    void onCheckNameValid();
public:
    QString m_albumNameStr;
    QString m_albumTypeStr;
    int m_UID;
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
