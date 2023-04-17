// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LEFTLISTVIEW_H
#define LEFTLISTVIEW_H

#include "dialogs/albumdeletedialog.h"
#include "albumimagebutton.h"
#include "leftlistwidget.h"

#include <DScrollArea>
#include <DWidget>
#include <DLabel>
#include <DListWidget>
#include <DMenu>

DWIDGET_USE_NAMESPACE

class LeftListView : public DScrollArea
{
    Q_OBJECT
public:
    enum MenuItemId {
        IdStartSlideShow,
        IdCreateAlbum,
        IdRenameAlbum,
        IdExport,
        IdDeleteAlbum,
    };

    explicit LeftListView(QWidget *parent = nullptr);
    QString getItemCurrentName();
    QString getItemCurrentType();
    int getItemCurrentUID();
    int getItemDataType();
    void updatePhotoListView();
    void updateAlbumItemsColor();
    void updateCustomizeListView();

private:
    void initUI();
    void initMenu();
    void initConnections();
    void showMenu(const QPoint &pos);
    void appendAction(int id, const QString &text, const QString &shortcut);
    QString getNewAlbumName();
    void resizeEvent(QResizeEvent *e) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
signals:
    void itemClicked();
    //幻灯片播放
    void sigSlideShow(QString path);
    void sigKeyDelete();
    void sigKeyF2();
    void updateCurrentItemType(int type);
    void sigLeftTabClicked();

public slots:
    void onMenuClicked(QAction *action);
    void onUpdateLeftListview();
    void onPhotoLibListViewPressed(const QModelIndex &index);
    void onCustomListViewPressed(const QModelIndex &index);
    void onMountListViewPressed(const QModelIndex &index);
    void onPhotoLibListViewCurrentItemChanged();
    void onCustomizeListViewCurrentItemChanged();
    void onMountListWidgetCurrentItemChanged();
    void onAddListBtnClicked();
    void onApplicationHelperThemeTypeChanged();
    void onGuiApplicationHelperThemeTypeChanged();
    void onMousePressIsNoValid();
    void onMouseMove();

public:

    // 照片库
    DLabel *m_pPhotoLibLabel;
    LeftListWidget *m_pPhotoLibListView;

    // 相册列表
    DLabel *m_pCustomizeLabel;
    AlbumImageButton *m_pAddListBtn;
    LeftListWidget *m_pCustomizeListView;

    // 设备
    DLabel *m_pMountLabel;
    LeftListWidget *m_pMountListWidget;
    DWidget *m_pMountWidget;

private:
    QString m_ItemCurrentName;
    QString m_ItemCurrentType;
    int m_currentUID;
    int m_ItemCurrentDataType;
    DMenu *m_pMenu;
    QMap<QString, QAction *> m_MenuActionMap;
    AlbumDeleteDialog *deletDialg;
    QScrollBar *m_bar = nullptr;
    int m_posY = 0;
};

#endif // LEFTLISTVIEW_H
