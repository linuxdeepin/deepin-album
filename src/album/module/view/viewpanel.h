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

    QString moduleName() override;
//    QWidget *toolbarBottomContent() override;
//    QWidget *toolbarTopLeftContent() override;
    void bottomTopLeftContent() ;
//    QWidget *toolbarTopMiddleContent() override;
    int getPicCount()
    {
        if (m_ttbc == nullptr) {
            return -1;
        }
        return m_ttbc->itemLoadedSize();
    }
//    ImageView *getImageView();


signals:
    void updateCollectButton();
    void imageChanged(const QString &path);
    void ttbcDeleteImage();
    void viewImageFrom(QString dir);
    void mouseMoved();
    void updateTopLeftWidthChanged(int width);
    void imgloader();
    void sigResize();

public slots:
    void onDeleteByMenu();
    void onGotoPanel(ModulePanel *p);
    void onShowExtensionPanel();
    void onHideExtensionPanel();
    void onHideImageView();
    bool onSigViewImage(const SignalManager::ViewInfo &info);
    void onMouseHoverMoved();
    void onESCKeyActivated();
//    void onImagesInserted();
//    void onViewImageNoNeedReload(int &fileindex);
    void onRotateClockwise();
    void onRotateCounterClockwise();
    void onRemoved();
    void onResetTransform(bool fitWindow);
    void onDoubleClicked();
    void onViewBClicked();
    void onViewBImageChanged(const QString &path);
    void onFileDelete();

protected:
    bool eventFilter(QObject *obj, QEvent *e) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *e) Q_DECL_OVERRIDE;
    void timerEvent(QTimerEvent *e) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *e) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *e)Q_DECL_OVERRIDE;

private:
    //------------------
    void loadFilesFromLocal(QStringList files);
    bool imageLocalLoaded(QStringList &filelist) override;
    bool imageFromDBLoaded(QStringList &filelist) override
    {
        Q_UNUSED(filelist)
        return false;
    }
    bool imageLoaded(QString filepath) override
    {
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
//    void popupPrintDialog(const QString &path);

    // Floating component
    void initFloatingComponent();
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
    void openImage(const QString &path, bool bjudge = true);
    void removeCurrentImage();
    void rotateImage(bool clockWise);
    // Geometry
    void toggleFullScreen();
    void showNormal();
    void showFullScreen();
    void backToLastPanel();

private slots:
//    void onThemeChanged(ViewerThemeManager::AppTheme theme);
    void feedBackCurrentIndex(int index, const QString &path);

private:
    int m_hideCursorTid;
    int m_iSlideShowTimerId;
    bool m_isInfoShowed;
    bool m_isMaximized = false;

    bool m_printDialogVisible = false;
    int m_topLeftContentWidth = 0;
    ImageView *m_viewB = nullptr;
    ThumbnailWidget *m_emptyWidget = nullptr;
    QMenu *m_menu = nullptr;
    QStackedWidget *m_stack = nullptr;
    LockWidget *m_lockWidget = nullptr;

    // Floating component
    DAnchors<NavigationWidget> m_nav;
    SignalManager::ViewInfo m_vinfo;
    TTBContent *m_ttbc = nullptr;
    int m_current = -1;
#ifdef LITE_DIV
    QScopedPointer<QDirIterator> m_imageDirIterator;
    void eatImageDirIterator();
#endif
    QString m_currentImageLastDir = "";
    //------------------
    QStringList m_filepathlist;
    QString m_currentpath = "";
    //------------------
    QTimer *m_deletetimer = nullptr;  //删除图片定时器
    bool   m_bFirstFullScreen;  //是否全屏进入

    //平板需求，工具栏显隐控制
    bool m_showorhide = true;
};
#endif // VIEWPANEL_H
