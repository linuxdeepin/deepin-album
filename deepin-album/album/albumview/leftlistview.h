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
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *e) Q_DECL_OVERRIDE;
signals:
    void itemClicked();
    void menuOpenImage(QString path, QStringList paths, bool isFullScreen, bool isSlideShow);
    void sigKeyDelete();
    void sigKeyF2();

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
    int m_ItemCurrentDataType;
    DMenu *m_pMenu;
    QMap<QString, QAction *> m_MenuActionMap;
    AlbumDeleteDialog *deletDialg;
};

#endif // LEFTLISTVIEW_H
