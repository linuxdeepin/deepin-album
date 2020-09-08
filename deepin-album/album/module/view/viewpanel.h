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
#ifndef VIEWPANEL_H
#define VIEWPANEL_H

#include "module/modulepanel.h"
#include "dbmanager/dbmanager.h"
#include "controller/viewerthememanager.h"
#include "danchors.h"
#include "thumbnailwidget.h"
#include "lockwidget.h"
#include "contents/ttbcontent.h"

#include <QFileInfo>
#include <QJsonObject>
#include <QDirIterator>
#include <DMenu>
#include <DDesktopServices>

DWIDGET_USE_NAMESPACE

class ImageButton;
class ImageView;
class ImageWidget;
class NavigationWidget;
class QFileSystemWatcher;
class QLabel;
class QStackedWidget;
class SlideEffectPlayer;

class ViewPanel : public ModulePanel, public ImageEngineObject
{
    Q_OBJECT
public:
    explicit ViewPanel(QWidget *parent = nullptr);
    ~ViewPanel() Q_DECL_OVERRIDE;

    QString moduleName() Q_DECL_OVERRIDE;
    QWidget *toolbarBottomContent() Q_DECL_OVERRIDE;
    QWidget *toolbarTopLeftContent() Q_DECL_OVERRIDE;
    QWidget *bottomTopLeftContent() ;
    QWidget *toolbarTopMiddleContent() Q_DECL_OVERRIDE;
//    QWidget *extensionPanelContent() Q_DECL_OVERRIDE;
    const SignalManager::ViewInfo viewInfo() const;
    int getPicCount()
    {
        if (!m_ttbc) {
            return -1;
        }
        return m_ttbc->itemLoadedSize();
    }
signals:
    void updateCollectButton();
//    void imageChanged(const QString &path, DBImgInfoList infos);
    //------------------
    void imageChanged(const QString &path);
    void ttbcDeleteImage();
    //------------------
    void viewImageFrom(QString dir);
    void mouseMoved();
    void updateTopLeftWidthChanged(int width);
    void updateTopLeftContentImage(const QString &path);
    void imgloader();

    void sigResize();

protected:
//    void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;
//    void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;
    bool eventFilter(QObject *obj, QEvent *e) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *e) Q_DECL_OVERRIDE;
    void timerEvent(QTimerEvent *e) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *e) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *e)Q_DECL_OVERRIDE;

private:
    //------------------
    void loadFilesFromLocal(QStringList files);
    void loadFilesFromLocal(DBImgInfoList files);
    bool imageLocalLoaded(QStringList &filelist) Q_DECL_OVERRIDE;
    bool imageFromDBLoaded(QStringList &filelist) Q_DECL_OVERRIDE {
        Q_UNUSED(filelist)
        return false;
    }
    bool imageLoaded(QString filepath) Q_DECL_OVERRIDE {
        Q_UNUSED(filepath)
        return false;
    }
    //------------------
    void initConnect();
    void initFileSystemWatcher();
    void initPopupMenu();
    void initShortcut();
    void initStack();
    void initViewContent();
    void popupDelDialog(const QString path);
    void popupPrintDialog(const QString path);

    // Floating component
    void initFloatingComponent();
    void initSwitchButtons();
    void initScaleLabel();
    void initNavigation();

    // Menu control
    void appendAction(int id, const QString &text, const QString &shortcut = "");
    void appendAction_darkmenu(int id, const QString &text, const QString &shortcut = "");
    DMenu *createAblumMenu();
#ifndef LITE_DIV
    DMenu *createAlbumMenu();
#endif
    void onMenuItemClicked(QAction *action);
    void updateMenuContent();

    // View control
    void onViewImage(const  QStringList &vinfo);
//    void onViewImage(const SignalManager::ViewInfo &vinfo);
    void openImage(const QString &path, bool inDB = true, bool bjudge = true);
    void removeCurrentImage();
    void rotateImage(bool clockWise);
    bool showNext();
    bool showPrevious();
    bool showImage(int index, int addIndex);

    // Geometry
    void toggleFullScreen();
    void showNormal();
    void showFullScreen();

    void viewOnNewProcess(const QStringList &paths);
    void backToLastPanel();

//    int imageIndex(const QString &path);
    QFileInfoList getFileInfos(const QString &path);
//    DBImgInfoList getImageInfos(const QFileInfoList &infos);
//    const QStringList paths() const;

private slots:
    void onThemeChanged(ViewerThemeManager::AppTheme theme);

    //------------------
    void feedBackCurrentIndex(int index, QString path);
    //------------------

//    void updateLocalImages();

private:
    int m_hideCursorTid;
    int m_iSlideShowTimerId;
    bool m_isInfoShowed;
    bool m_isMaximized = false;

    bool m_printDialogVisible = false;
    int m_topLeftContentWidth = 0;
    ImageView *m_viewB;
    ThumbnailWidget *m_emptyWidget = nullptr;
    QMenu *m_menu;
    QStackedWidget *m_stack;
    LockWidget *m_lockWidget;

    // Floating component
    DAnchors<NavigationWidget> m_nav;

    SignalManager::ViewInfo m_vinfo;
//    DBImgInfoList m_infos;
//    DBImgInfoList::ConstIterator m_current =NULL;

    TTBContent *m_ttbc;
    int m_current = -1;
#ifdef LITE_DIV
    QScopedPointer<QDirIterator> m_imageDirIterator;

    void eatImageDirIterator();
#endif
    QString m_currentImageLastDir = "";
//    QString m_viewType;

    //------------------
    QStringList m_filepathlist;
    QString m_currentpath = "";
    //------------------

    QTimer *m_deletetimer;  //删除图片定时器
    bool   m_bFirstFullScreen;  //是否全屏进入
};
#endif // VIEWPANEL_H
