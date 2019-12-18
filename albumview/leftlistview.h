#ifndef LEFTLISTVIEW_H
#define LEFTLISTVIEW_H

#include "albumimagebutton.h"
#include "leftlistwidget.h"

#include <DWidget>
#include <DLabel>
#include <DListWidget>
#include <DMenu>

DWIDGET_USE_NAMESPACE

class LeftListView : public DWidget
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

    LeftListView(DWidget* parent);
    QString getItemCurrentName();
    QString getItemCurrentType();
    void updatePhotoListView();
    void updateAlbumItemsColor();
    void updateCustomizeListView();
    void moveMountListWidget();

private:
    void initUI();
    void initMenu();
    void initConnections();
    void updateMountListView();
    void showMenu(const QPoint &pos);
    void appendAction(int id, const QString &text, const QString &shortcut);
    QString getNewAlbumName();
    void keyPressEvent(QKeyEvent *e) override;

signals:
    void itemClicked();
    void menuOpenImage(QString path, QStringList paths, bool isFullScreen, bool isSlideShow);
    void sigKeyDelete();
    void sigKeyF2();

private slots:
    void onMenuClicked(QAction *action);

public:
    // 照片库
    DLabel* m_pPhotoLibLabel;
    LeftListWidget* m_pPhotoLibListView;

    // 相册列表
    DLabel* m_pCustomizeLabel;
    AlbumImageButton* m_pAddListBtn;
    LeftListWidget* m_pCustomizeListView;

    // 设备
    DLabel* m_pMountLabel;
    LeftListWidget* m_pMountListView;

private:
    QString m_ItemCurrentName;
    QString m_ItemCurrentType;
    DMenu *m_pMenu;
    QMap<QString, QAction *> m_MenuActionMap;
    DWidget *m_pMountWidget;
};

#endif // LEFTLISTVIEW_H
