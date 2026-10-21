// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "globalstatus.h"
#include "ddlog.h"
#include "filecontrol.h"
#include "albumControl.h"

static const int sc_MinHeight = 300;           // 窗口最小高度
static const int sc_MinWidth = 658;            // 窗口最小宽度
static const int sc_MinHideHeight = 425;       // 调整窗口高度小于425px时，隐藏工具栏和标题栏
static const int sc_FloatMargin = 65;          // 浮动按钮边距
static const int sc_TitleHeight = 50;          // 标题栏栏高度
static const int sc_ThumbnailViewHeight = 70;  // 底部工具栏高度 70px
static const int sc_ShowBottomY = 80;  // 底部工具栏显示时距离底部的高度 80px (工具栏高度 70px + 边距 10px)
static const int sc_SwitchImageHotspotWidth = 100;  // 左右切换图片按钮的热区宽度 100px
static const int sc_ActionMargin = 9;               // 应用图标距离顶栏
static const int sc_RightMenuItemHeight = 32;       // 右键菜单item的高度

static const double sc_AnimationDefaultDuration = 366;  // 默认动画时长
static const int sc_PathViewItemCount = 3;              // 默认 PathView 在路径中的 Item 计数

// 相册相关状态变量
static const int sc_NeedHideSideBarWidth = 783;      // 需要隐藏侧边栏的时，主界面宽度
static const int sc_SideBarWidth = 200;              // 侧边栏宽度
static const int sc_StatusBarHeight = 30;            // 状态栏高度
static const int sc_CollectionTopMargin = 25;        // 合集年月视图上边距
static const int sc_ThumbnailViewTitleHieght = 85;   // 缩略图视图区域标题显示区域高度
static const int sc_VerticalScrollBarWidth = 15;     // 垂直滚动条宽度
static const int sc_RectSelScrollStep = 30;          // 框选滚动步进
static const int sc_ThumbnailListRightMargin = 10;   // 框选滚动步进
static const int sc_ThumbnialListCellSpace = 4;      // 框选滚动步进

GlobalStatus *GlobalStatus::instance()
{
    // qCDebug(albumLog) << "GlobalStatus::instance - Function entry/exit";
    static GlobalStatus s_instance;
    return &s_instance;
}

/**
   @class GlobalStatus
   @brief QML单例类，维护全局状态，同步不同组件间的状态信息
   @details 相较于使用脚本配置的 program Singletion , Qt 更推崇使用 QObject 注册单例
   @link https://doc.qt.io/qt-6/qtquick-performance.html#use-singleton-types-instead-of-pragma-library-scripts
 */

GlobalStatus::GlobalStatus(QObject *parent)
    : QObject(parent)
{
    qCDebug(albumLog) << "Initializing GlobalStatus";
    initConnect();
}

GlobalStatus::~GlobalStatus() {
    // qCDebug(albumLog) << "Destroying GlobalStatus";
    // 在程序退出的过程中
    // 由于析构过成功会触发destoryed信号，从而导致qml上使用GlobalStatus对象的地方触发重新绑定，
    // 但此时本对象已经析构，qml上会使用一些已经析构掉的对象，导致崩溃
    disconnect(this, nullptr, nullptr, nullptr);
    // qCDebug(albumLog) << "GlobalStatus::~GlobalStatus - Function exit";
}

/**
   @return 返回是否全屏显示图片
 */
bool GlobalStatus::showFullScreen() const
{
    qCDebug(albumLog) << "GlobalStatus::showFullScreen - Function entry, returning:" << storeshowFullScreen;
    return storeshowFullScreen;
}

/**
   @brief 设置全屏显示图片
 */
void GlobalStatus::setShowFullScreen(bool value)
{
    qCDebug(albumLog) << "GlobalStatus::setShowFullScreen - Function entry, value:" << value;
    if (value != storeshowFullScreen) {
        qCDebug(albumLog) << "Setting full screen from" << storeshowFullScreen << "to" << value;
        storeshowFullScreen = value;
        Q_EMIT showFullScreenChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setShowFullScreen - Function exit";
}

/**
   @return 返回是否允许显示导航窗口
 */
bool GlobalStatus::enableNavigation() const
{
    qCDebug(albumLog) << "GlobalStatus::enableNavigation - Function entry, returning:" << storeenableNavigation;
    return storeenableNavigation;
}

/**
   @brief 设置是否允许显示导航窗口
 */
void GlobalStatus::setEnableNavigation(bool value)
{
    qCDebug(albumLog) << "GlobalStatus::setEnableNavigation - Function entry, value:" << value;
    if (value != storeenableNavigation) {
        qCDebug(albumLog) << "Setting navigation enabled from" << storeenableNavigation << "to" << value;
        storeenableNavigation = value;
        Q_EMIT enableNavigationChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setEnableNavigation - Function exit";
}

/**
   @return 返回是否显示右键菜单
 */
bool GlobalStatus::showRightMenu() const
{
    qCDebug(albumLog) << "GlobalStatus::showRightMenu - Function entry, returning:" << storeshowRightMenu;
    return storeshowRightMenu;
}

/**
   @brief 设置是否显示右键菜单
 */
void GlobalStatus::setShowRightMenu(bool value)
{
    qCDebug(albumLog) << "GlobalStatus::setShowRightMenu - Function entry, value:" << value;
    if (value != storeshowRightMenu) {
        qCDebug(albumLog) << "Setting right menu visibility from" << storeshowRightMenu << "to" << value;
        storeshowRightMenu = value;
        Q_EMIT showRightMenuChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setShowRightMenu - Function exit";
}

/**
   @return 当前是否弹窗显示详细图像信息
 */
bool GlobalStatus::showImageInfo() const
{
    qCDebug(albumLog) << "GlobalStatus::showImageInfo - Function entry, returning:" << storeshowImageInfo;
    return storeshowImageInfo;
}

/**
   @brief 设置是否弹窗显示详细图像信息
 */
void GlobalStatus::setShowImageInfo(bool value)
{
    qCDebug(albumLog) << "GlobalStatus::setShowImageInfo - Function entry, value:" << value;
    if (value != storeshowImageInfo) {
        qCDebug(albumLog) << "Setting image info visibility from" << storeshowImageInfo << "to" << value;
        storeshowImageInfo = value;
        Q_EMIT showImageInfoChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setShowImageInfo - Function exit";
}

/**
   @return 返回滑动视图是否响应操作
 */
bool GlobalStatus::viewInteractive() const
{
    qCDebug(albumLog) << "GlobalStatus::viewInteractive - Function entry, returning:" << storeviewInteractive;
    return storeviewInteractive;
}

/**
   @brief 设置滑动视图是否响应操作
 */
void GlobalStatus::setViewInteractive(bool value)
{
    qCDebug(albumLog) << "GlobalStatus::setViewInteractive - Function entry, value:" << value;
    if (value != storeviewInteractive) {
        qCDebug(albumLog) << "Setting view interactive from" << storeviewInteractive << "to" << value;
        storeviewInteractive = value;
        Q_EMIT viewInteractiveChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setViewInteractive - Function exit";
}

/**
   @return 返回滑动视图是否处于轻弹状态
 */
bool GlobalStatus::viewFlicking() const
{
    qCDebug(albumLog) << "GlobalStatus::viewFlicking - Function entry, returning:" << storeviewFlicking;
    return storeviewFlicking;
}

/**
   @brief 设置当前滑动视图是否处于轻弹状态
 */
void GlobalStatus::setViewFlicking(bool value)
{
    qCDebug(albumLog) << "GlobalStatus::setViewFlicking - Function entry, value:" << value;
    if (value != storeviewFlicking) {
        qCDebug(albumLog) << "Setting view flicking from" << storeviewFlicking << "to" << value;
        storeviewFlicking = value;
        Q_EMIT viewFlickingChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setViewFlicking - Function exit";
}

/**
   @return 返回当前是否允许标题栏、底栏动画效果
 */
bool GlobalStatus::animationBlock() const
{
    // qCDebug(albumLog) << "GlobalStatus::animationBlock - Function entry, returning:" << storeanimationBlock;
    return storeanimationBlock;
}

/**
   @brief 设置当前允许标题栏、底栏动画效果的标志值为 \a value
 */
void GlobalStatus::setAnimationBlock(bool value)
{
    qCDebug(albumLog) << "GlobalStatus::setAnimationBlock - Function entry, value:" << value;
    if (value != storeanimationBlock) {
        qCDebug(albumLog) << "Setting animation block from" << storeanimationBlock << "to" << value;
        storeanimationBlock = value;
        Q_EMIT animationBlockChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setAnimationBlock - Function exit";
}

/**
   @return 返回当前是否允许全屏展示动画
 */
bool GlobalStatus::fullScreenAnimating() const
{
    qCDebug(albumLog) << "GlobalStatus::fullScreenAnimating - Function entry, returning:" << storefullScreenAnimating;
    return storefullScreenAnimating;
}

/**
   @brief 设置当前是否允许全屏展示动画的标志值为 \a value
 */
void GlobalStatus::setFullScreenAnimating(bool value)
{
    qCDebug(albumLog) << "GlobalStatus::setFullScreenAnimating - Function entry, value:" << value;
    if (value != storefullScreenAnimating) {
        qCDebug(albumLog) << "Setting full screen animating from" << storefullScreenAnimating << "to" << value;
        storefullScreenAnimating = value;
        Q_EMIT fullScreenAnimatingChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setFullScreenAnimating - Function exit";
}

/**
   @return 返回当前缩略图列表允许显示的宽度
 */
int GlobalStatus::thumbnailVaildWidth() const
{
    qCDebug(albumLog) << "GlobalStatus::thumbnailVaildWidth - Function entry, returning:" << storethumbnailVaildWidth;
    return storethumbnailVaildWidth;
}

/**
   @brief 设置当前缩略图列表允许显示的宽度为 \a value
 */
void GlobalStatus::setThumbnailVaildWidth(int value)
{
    qCDebug(albumLog) << "GlobalStatus::setThumbnailVaildWidth - Function entry, value:" << value;
    if (value != storethumbnailVaildWidth) {
        qCDebug(albumLog) << "Setting thumbnail valid width from" << storethumbnailVaildWidth << "to" << value;
        storethumbnailVaildWidth = value;
        Q_EMIT thumbnailVaildWidthChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setThumbnailVaildWidth - Function exit";
}

/**
   @return 返回当前显示的界面索引
 */
Types::StackPage GlobalStatus::stackPage() const
{
    // qCDebug(albumLog) << "GlobalStatus::stackPage - Function entry, returning:" << storestackPage;
    return storestackPage;
}

/**
   @brief 设置当前显示的界面索引为 \a value ，将切换显示的界面类型
 */
void GlobalStatus::setStackPage(Types::StackPage value)
{
    qCDebug(albumLog) << "GlobalStatus::setStackPage - Function entry, value:" << value;
    if (value != storestackPage) {
        qCDebug(albumLog) << "Setting stack page from" << storestackPage << "to" << value;
        storestackPage = value;
        Q_EMIT stackPageChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setStackPage - Function exit";
}

bool GlobalStatus::showExportDialog() const
{
    qCDebug(albumLog) << "GlobalStatus::showExportDialog - Function entry, returning:" << storeshowExportDialog;
    return storeshowExportDialog;
}

void GlobalStatus::setShowExportDialog(bool value)
{
    qCDebug(albumLog) << "GlobalStatus::setShowExportDialog - Function entry, value:" << value;
    if (value != storeshowExportDialog) {
        qCDebug(albumLog) << "Setting export dialog visibility from" << storeshowExportDialog << "to" << value;
        storeshowExportDialog = value;
        Q_EMIT showExportDialogChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setShowExportDialog - Function exit";
}

int GlobalStatus::minHeight() const
{
    // qCDebug(albumLog) << "GlobalStatus::minHeight - Function entry, returning:" << sc_MinHeight;
    return sc_MinHeight;
}

int GlobalStatus::minWidth() const
{
    // qCDebug(albumLog) << "GlobalStatus::minWidth - Function entry, returning:" << sc_MinWidth;
    return sc_MinWidth;
}

int GlobalStatus::minHideHeight() const
{
    // qCDebug(albumLog) << "GlobalStatus::minHideHeight - Function entry, returning:" << sc_MinHideHeight;
    return sc_MinHideHeight;
}

int GlobalStatus::floatMargin() const
{
    // qCDebug(albumLog) << "GlobalStatus::floatMargin - Function entry, returning:" << sc_FloatMargin;
    return sc_FloatMargin;
}

int GlobalStatus::titleHeight() const
{
    // qCDebug(albumLog) << "GlobalStatus::titleHeight - Function entry, returning:" << sc_TitleHeight;
    return sc_TitleHeight;
}

int GlobalStatus::thumbnailViewHeight() const
{
    // qCDebug(albumLog) << "GlobalStatus::thumbnailViewHeight - Function entry, returning:" << sc_ThumbnailViewHeight;
    return sc_ThumbnailViewHeight;
}

int GlobalStatus::showBottomY() const
{
    // qCDebug(albumLog) << "GlobalStatus::showBottomY - Function entry, returning:" << sc_ShowBottomY;
    return sc_ShowBottomY;
}

int GlobalStatus::switchImageHotspotWidth() const
{
    // qCDebug(albumLog) << "GlobalStatus::switchImageHotspotWidth - Function entry, returning:" << sc_SwitchImageHotspotWidth;
    return sc_SwitchImageHotspotWidth;
}

int GlobalStatus::actionMargin() const
{
    // qCDebug(albumLog) << "GlobalStatus::actionMargin - Function entry, returning:" << sc_ActionMargin;
    return sc_ActionMargin;
}

int GlobalStatus::rightMenuItemHeight() const
{
    // qCDebug(albumLog) << "GlobalStatus::rightMenuItemHeight - Function entry, returning:" << sc_RightMenuItemHeight;
    return sc_RightMenuItemHeight;
}

double GlobalStatus::animationDefaultDuration() const
{
    // qCDebug(albumLog) << "GlobalStatus::animationDefaultDuration - Function entry, returning:" << sc_AnimationDefaultDuration;
    return sc_AnimationDefaultDuration;
}

/**
   @brief 默认 PathView 在路径中的 Item 计数
   @note 会影响 PathView 相关的动画效果计算，修改此值需慎重考虑
 */
int GlobalStatus::pathViewItemCount() const
{
    // qCDebug(albumLog) << "GlobalStatus::pathViewItemCount - Function entry, returning:" << sc_PathViewItemCount;
    return sc_PathViewItemCount;
}

void GlobalStatus::setFileControl(FileControl *fc)
{
    qCDebug(albumLog) << "Setting file control";
    m_fileControl = fc;

    if (!m_fileControl) {
        qCWarning(albumLog) << "File control is null";
        return;
    }

    bool bRet = false;
    m_nAnimationDuration = m_fileControl->getConfigValue("", "animationDuration", 400).toInt(&bRet);
    if (!bRet) {
        qCWarning(albumLog) << "Failed to get animation duration, using default: 400";
        m_nAnimationDuration = 400;
    }
    
    m_nLargeImagePreviewAnimationDuration = m_fileControl->getConfigValue("", "largeImagePreviewAnimationDuration", 800).toInt(&bRet);
    if (!bRet) {
        qCWarning(albumLog) << "Failed to get large image preview animation duration, using default: 800";
        m_nLargeImagePreviewAnimationDuration = 800;
    }
    
    m_bEnableSidebarAnimation = m_fileControl->getConfigValue("", "enableSidebarAnimation", 0).toInt(&bRet);
    if (!bRet) {
        qCWarning(albumLog) << "Failed to get sidebar animation setting, using default: false";
        m_bEnableSidebarAnimation = false;
    }
    qCDebug(albumLog) << "GlobalStatus::setFileControl - Function exit";
}

int GlobalStatus::sideBarWidth() const
{
    // qCDebug(albumLog) << "GlobalStatus::sideBarWidth - Function entry, returning:" << sc_SideBarWidth;
    return sc_SideBarWidth;
}

int GlobalStatus::statusBarHeight() const
{
    // qCDebug(albumLog) << "GlobalStatus::statusBarHeight - Function entry, returning:" << sc_StatusBarHeight;
    return sc_StatusBarHeight;
}

int GlobalStatus::collectionTopMargin() const
{
    // qCDebug(albumLog) << "GlobalStatus::collectionTopMargin - Function entry, returning:" << sc_CollectionTopMargin;
    return sc_CollectionTopMargin;
}

int GlobalStatus::thumbnailViewTitleHieght() const
{
    // qCDebug(albumLog) << "GlobalStatus::thumbnailViewTitleHieght - Function entry, returning:" << sc_ThumbnailViewTitleHieght;
    return sc_ThumbnailViewTitleHieght;
}

int GlobalStatus::verticalScrollBarWidth() const
{
    // qCDebug(albumLog) << "GlobalStatus::verticalScrollBarWidth - Function entry, returning:" << sc_VerticalScrollBarWidth;
    return sc_VerticalScrollBarWidth;
}

int GlobalStatus::rectSelScrollStep() const
{
    // qCDebug(albumLog) << "GlobalStatus::rectSelScrollStep - Function entry, returning:" << sc_RectSelScrollStep;
    return sc_RectSelScrollStep;
}

int GlobalStatus::thumbnailListRightMargin() const
{
    // qCDebug(albumLog) << "GlobalStatus::thumbnailListRightMargin - Function entry, returning:" << sc_ThumbnailListRightMargin;
    return sc_ThumbnailListRightMargin;
}

int GlobalStatus::thumbnialListCellSpace() const
{
    // qCDebug(albumLog) << "GlobalStatus::thumbnialListCellSpace - Function entry, returning:" << sc_ThumbnialListCellSpace;
    return sc_ThumbnialListCellSpace;
}

int GlobalStatus::needHideSideBarWidth() const
{
    // qCDebug(albumLog) << "GlobalStatus::needHideSideBarWidth - Function entry, returning:" << sc_NeedHideSideBarWidth;
    return sc_NeedHideSideBarWidth;
}

int GlobalStatus::animationDuration() const
{
    // qCDebug(albumLog) << "GlobalStatus::animationDuration - Function entry, returning:" << m_nAnimationDuration;
    return m_nAnimationDuration;
}

int GlobalStatus::largeImagePreviewAnimationDuration() const
{
    // qCDebug(albumLog) << "GlobalStatus::largeImagePreviewAnimationDuration - Function entry, returning:" << m_nLargeImagePreviewAnimationDuration;
    return m_nLargeImagePreviewAnimationDuration;
}

bool GlobalStatus::sidebarAnimationEnabled() const
{
    // qCDebug(albumLog) << "GlobalStatus::sidebarAnimationEnabled - Function entry, returning:" << m_bEnableSidebarAnimation;
    return m_bEnableSidebarAnimation;
}

qreal GlobalStatus::sideBarX() const
{
    // qCDebug(albumLog) << "GlobalStatus::sideBarX - Function entry, returning:" << m_sideBar_X;
    return m_sideBar_X;
}

void GlobalStatus::setSideBarX(const qreal& value)
{
    qCDebug(albumLog) << "GlobalStatus::setSideBarX - Function entry, value:" << value;
    if (!qFuzzyCompare(m_sideBar_X, value)) {
        qCDebug(albumLog) << "Setting sidebar X from" << m_sideBar_X << "to" << value;
        m_sideBar_X = value;
        Q_EMIT sideBarXChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setSideBarX - Function exit";
}

QVariantList GlobalStatus::selectedPaths() const
{
    // qCDebug(albumLog) << "GlobalStatus::selectedPaths - Function entry, returning:" << m_selectedPaths;
    return m_selectedPaths;
}

void GlobalStatus::setSelectedPaths(const QVariantList& value)
{
    qCDebug(albumLog) << "GlobalStatus::setSelectedPaths - Function entry, value:" << value;
    if (m_selectedPaths != value) {
        qCDebug(albumLog) << "Setting selected paths, count:" << value.size();
        m_selectedPaths = value;
        Q_EMIT selectedPathsChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setSelectedPaths - Function exit";
}

bool GlobalStatus::bRefreshFavoriteIconFlag() const
{
    // qCDebug(albumLog) << "GlobalStatus::bRefreshFavoriteIconFlag - Function entry, returning:" << m_bRefreshFavoriteIconFlag;
    return m_bRefreshFavoriteIconFlag;
}

void GlobalStatus::setBRefreshFavoriteIconFlag(const bool& value)
{
    qCDebug(albumLog) << "GlobalStatus::setBRefreshFavoriteIconFlag - Function entry, value:" << value;
    if (m_bRefreshFavoriteIconFlag != value) {
        qCDebug(albumLog) << "Setting refresh favorite icon flag from" << m_bRefreshFavoriteIconFlag << "to" << value;
        m_bRefreshFavoriteIconFlag = value;
        Q_EMIT bRefreshFavoriteIconFlagChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setBRefreshFavoriteIconFlag - Function exit";
}

bool GlobalStatus::refreshRangeBtnState() const
{
    // qCDebug(albumLog) << "GlobalStatus::refreshRangeBtnState - Function entry, returning:" << m_bRefreshRangeBtnState;
    return m_bRefreshRangeBtnState;
}

void GlobalStatus::setRefreshRangeBtnState(const bool& value)
{
    qCDebug(albumLog) << "GlobalStatus::setRefreshRangeBtnState - Function entry, value:" << value;
    if (m_bRefreshRangeBtnState != value) {
        qCDebug(albumLog) << "Setting refresh range button state from" << m_bRefreshRangeBtnState << "to" << value;
        m_bRefreshRangeBtnState = value;
        Q_EMIT refreshRangeBtnStateChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setRefreshRangeBtnState - Function exit";
}

Types::ThumbnailViewType GlobalStatus::currentViewType() const
{
    // qCDebug(albumLog) << "GlobalStatus::currentViewType - Function entry, returning:" << m_currentViewType;
    return m_currentViewType;
}

void GlobalStatus::setCurrentViewType(const Types::ThumbnailViewType &value)
{
    qCDebug(albumLog) << "GlobalStatus::setCurrentViewType - Function entry, value:" << value;
    if (m_currentViewType != value) {
        qCDebug(albumLog) << "Setting current view type from" << m_currentViewType << "to" << value;
        m_currentViewType = value;

        setEnableRatioAnimation(false);
        setBackingToMainAlbumView(false);
        // 若相册数据库没有图片资源，则调整显示"没有图片"提示视图
        if (AlbumControl::instance()->getAllCount() <= 0) {
            qCDebug(albumLog) << "No images in album database, adjusting view type";
            switch (value) {
            case Types::ViewImport:
            case Types::ViewNoPicture:
            case Types::ViewCollecttion:
                break;
            case Types::ViewSearchResult:
                m_currentViewType = Types::ViewNoPicture;
                qCDebug(albumLog) << "Changing to no picture view for search result";
                break;
            default:
                break;
            }
        }
        Q_EMIT currentViewTypeChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setCurrentViewType - Function exit";
}

int GlobalStatus::currentCollecttionViewIndex() const
{
    // qCDebug(albumLog) << "GlobalStatus::currentCollecttionViewIndex - Function entry, returning:" << m_currentCollecttionViewIndex;
    return m_currentCollecttionViewIndex;
}

void GlobalStatus::setCurrentCollecttionViewIndex(const int &value)
{
    qCDebug(albumLog) << "GlobalStatus::setCurrentCollecttionViewIndex - Function entry, value:" << value;
    if (m_currentCollecttionViewIndex != value) {
        qCDebug(albumLog) << "Setting collection view index from" << m_currentCollecttionViewIndex << "to" << value;
        m_currentCollecttionViewIndex = value;
        setEnableRatioAnimation(false);
        setBackingToMainAlbumView(false);
        Q_EMIT currentCollecttionViewIndexChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setCurrentCollecttionViewIndex - Function exit";
}

Types::SwitchType GlobalStatus::currentSwitchType() const
{
    // qCDebug(albumLog) << "GlobalStatus::currentSwitchType - Function entry, returning:" << m_currentSwitchType;
    return m_currentSwitchType;
}

void GlobalStatus::setCurrentSwitchType(const int &value)
{
    qCDebug(albumLog) << "GlobalStatus::setCurrentSwitchType - Function entry, value:" << value;
    if (m_currentSwitchType != value) {
        qCDebug(albumLog) << "Setting switch type from" << m_currentSwitchType << "to" << value;
        m_currentSwitchType = static_cast<Types::SwitchType>(value);
        Q_EMIT currentSwitchTypeChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setCurrentSwitchType - Function exit";
}

int GlobalStatus::currentCustomAlbumUId() const
{
    // qCDebug(albumLog) << "GlobalStatus::currentCustomAlbumUId - Function entry, returning:" << m_currentCustomAlbumUId;
    return m_currentCustomAlbumUId;
}

void GlobalStatus::setCurrentCustomAlbumUId(const int &value)
{
    qCDebug(albumLog) << "GlobalStatus::setCurrentCustomAlbumUId - Function entry, value:" << value;
    if (m_currentCustomAlbumUId != value) {
        qCDebug(albumLog) << "Setting custom album UID from" << m_currentCustomAlbumUId << "to" << value;
        setBackingToMainAlbumView(false);
        m_currentCustomAlbumUId = value;
        Q_EMIT currentCustomAlbumUIdChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setCurrentCustomAlbumUId - Function exit";
}

int GlobalStatus::stackControlCurrent() const
{
    // qCDebug(albumLog) << "GlobalStatus::stackControlCurrent - Function entry, returning:" << m_stackControlCurrent;
    return m_stackControlCurrent;
}

void GlobalStatus::setStackControlCurrent(const int &value)
{
    qCDebug(albumLog) << "GlobalStatus::setStackControlCurrent - Function entry, value:" << value;
    if (m_stackControlCurrent != value) {
        qCDebug(albumLog) << "Setting stack control current from" << m_stackControlCurrent << "to" << value;

        if (m_stackControlCurrent != 0 && value == 0) {
            qCDebug(albumLog) << "Moving to album animation";
            setBackingToMainAlbumView(true);
            Q_EMIT sigMoveToAlbumAnimation();
        } else {
            setBackingToMainAlbumView(false);
        }
        m_stackControlCurrent = value;
        Q_EMIT stackControlCurrentChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setStackControlCurrent - Function exit";
}

int GlobalStatus::stackControlLastCurrent() const
{
    // qCDebug(albumLog) << "GlobalStatus::stackControlLastCurrent - Function entry, returning:" << m_stackControlLastCurrent;
    return m_stackControlLastCurrent;
}

void GlobalStatus::setStackControlLastCurrent(const int &value)
{
    qCDebug(albumLog) << "GlobalStatus::setStackControlLastCurrent - Function entry, value:" << value;
    if (m_stackControlLastCurrent != value) {
        m_stackControlLastCurrent = value;
        Q_EMIT stackControlLastCurrentChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setStackControlLastCurrent - Function exit";
}

int GlobalStatus::thumbnailSizeLevel() const
{
    // qCDebug(albumLog) << "GlobalStatus::thumbnailSizeLevel - Function entry, returning:" << m_thumbnailSizeLevel;
    return m_thumbnailSizeLevel;
}

void GlobalStatus::setThumbnailSizeLevel(const int &value)
{
    qCDebug(albumLog) << "GlobalStatus::setThumbnailSizeLevel - Function entry, value:" << value;
    if (m_thumbnailSizeLevel != value) {
        qCDebug(albumLog) << "Setting thumbnail size level from" << m_thumbnailSizeLevel << "to" << value;
        m_thumbnailSizeLevel = value;

        Q_EMIT thumbnailSizeLevelChanged();

        // 缩放等级有调整， 同步调整网格大小
        qreal newCellBaseWidth = m_thumbnailSizeLevel >= 0 && m_thumbnailSizeLevel <= 9 ? 80 + m_thumbnailSizeLevel * 10 : 80;
        qCDebug(albumLog) << "Adjusting cell base width to:" << newCellBaseWidth;
        setCellBaseWidth(newCellBaseWidth);
    }
    qCDebug(albumLog) << "GlobalStatus::setThumbnailSizeLevel - Function exit";
}

qreal GlobalStatus::cellBaseWidth() const
{
    // qCDebug(albumLog) << "GlobalStatus::cellBaseWidth - Function entry, returning:" << m_cellBaseWidth;
    return m_cellBaseWidth;
}

void GlobalStatus::setCellBaseWidth(const qreal& value)
{
    qCDebug(albumLog) << "GlobalStatus::setCellBaseWidth - Function entry, value:" << value;
    if (!qFuzzyCompare(m_cellBaseWidth, value)) {
        qCDebug(albumLog) << "Setting cell base width from" << m_cellBaseWidth << "to" << value;
        m_cellBaseWidth = value;
        Q_EMIT cellBaseWidthChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setCellBaseWidth - Function exit";
}

QString GlobalStatus::statusBarNumText() const
{
    // qCDebug(albumLog) << "GlobalStatus::statusBarNumText - Function entry, returning:" << m_statusBarNumText;
    return m_statusBarNumText;
}

void GlobalStatus::setStatusBarNumText(const QString &value)
{
    qCDebug(albumLog) << "GlobalStatus::setStatusBarNumText - Function entry, value:" << value;
    if (m_statusBarNumText != value) {
        qCDebug(albumLog) << "Setting status bar text from" << m_statusBarNumText << "to" << value;
        m_statusBarNumText = value;
        Q_EMIT statusBarNumTextChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setStatusBarNumText - Function exit";
}

bool GlobalStatus::allCollectionContentResolved() const
{
    return m_allCollectionContentResolved;
}

void GlobalStatus::setAllCollectionContentResolved(const bool &value)
{
    if (m_allCollectionContentResolved != value) {
        m_allCollectionContentResolved = value;
        Q_EMIT allCollectionContentResolvedChanged();
    }
}

QString GlobalStatus::searchEditText() const
{
    // qCDebug(albumLog) << "GlobalStatus::searchEditText - Function entry, returning:" << m_searchEditText;
    return m_searchEditText;
}

void GlobalStatus::setSearchEditText(const QString &value)
{
    qCDebug(albumLog) << "GlobalStatus::setSearchEditText - Function entry, value:" << value;
    if (m_searchEditText != value) {
        qCDebug(albumLog) << "Setting search edit text from" << m_searchEditText << "to" << value;
        m_searchEditText = value;
        Q_EMIT searchEditTextChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setSearchEditText - Function exit";
}

bool GlobalStatus::albumImportChangeList() const
{
    // qCDebug(albumLog) << "GlobalStatus::albumImportChangeList - Function entry, returning:" << m_bAlbumImportChangeList;
    return m_bAlbumImportChangeList;
}

void GlobalStatus::setAlbumImportChangeList(const bool &value)
{
    qCDebug(albumLog) << "GlobalStatus::setAlbumImportChangeList - Function entry, value:" << value;
    if (m_bAlbumImportChangeList != value) {
        qCDebug(albumLog) << "Setting album import change list from" << m_bAlbumImportChangeList << "to" << value;
        m_bAlbumImportChangeList = value;
        Q_EMIT albumImportChangeListChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setAlbumImportChangeList - Function exit";
}

bool GlobalStatus::albumChangeList() const
{
    // qCDebug(albumLog) << "GlobalStatus::albumChangeList - Function entry, returning:" << m_bAlbumChangeList;
    return  m_bAlbumChangeList;
}

void GlobalStatus::setAlbumChangeList(const bool &value)
{
    qCDebug(albumLog) << "GlobalStatus::setAlbumChangeList - Function entry, value:" << value;
    if (m_bAlbumChangeList != value) {
        qCDebug(albumLog) << "Setting album change list from" << m_bAlbumChangeList << "to" << value;
        m_bAlbumChangeList = value;
        Q_EMIT albumChangeListChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setAlbumChangeList - Function exit";
}

bool GlobalStatus::sideBarIsVisible() const
{
    // qCDebug(albumLog) << "GlobalStatus::sideBarIsVisible - Function entry, returning:" << m_bSideBarIsVisible;
    return  m_bSideBarIsVisible;
}

void GlobalStatus::setSideBarIsVisible(const bool &value)
{
    qCDebug(albumLog) << "GlobalStatus::setSideBarIsVisible - Function entry, value:" << value;
    if (m_bSideBarIsVisible != value) {
        qCDebug(albumLog) << "Setting sidebar visibility from" << m_bSideBarIsVisible << "to" << value;
        m_bSideBarIsVisible = value;
        Q_EMIT sideBarIsVisibleChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setSideBarIsVisible - Function exit";
}

QString GlobalStatus::currentDeviceName() const
{
    // qCDebug(albumLog) << "GlobalStatus::currentDeviceName - Function entry, returning:" << m_currentDeviceName;
    return m_currentDeviceName;
}

void GlobalStatus::setCurrentDeviceName(const QString &value)
{
    qCDebug(albumLog) << "GlobalStatus::setCurrentDeviceName - Function entry, value:" << value;
    if (m_currentDeviceName != value) {
        qCDebug(albumLog) << "Setting current device name from" << m_currentDeviceName << "to" << value;
        m_currentDeviceName = value;
        Q_EMIT currentDeviceNameChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setCurrentDeviceName - Function exit";
}

QString GlobalStatus::currentDevicePath() const
{
    // qCDebug(albumLog) << "GlobalStatus::currentDevicePath - Function entry, returning:" << m_currentDevicePath;
    return m_currentDevicePath;
}

void GlobalStatus::setCurrentDevicePath(const QString &value)
{
    qCDebug(albumLog) << "GlobalStatus::setCurrentDevicePath - Function entry, value:" << value;
    if (m_currentDevicePath != value) {
        qCDebug(albumLog) << "Setting current device path from" << m_currentDevicePath << "to" << value;
        m_currentDevicePath = value;
        m_currentDeviceName = AlbumControl::instance()->getDeviceName(m_currentDevicePath);
        Q_EMIT currentDevicePathChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setCurrentDevicePath - Function exit";
}

bool GlobalStatus::windowDisactived() const
{
    // qCDebug(albumLog) << "GlobalStatus::windowDisactived - Function entry, returning:" << m_bWindowDisactived;
    return m_bWindowDisactived;
}

void GlobalStatus::setWindowDisactived(const bool &value)
{
    qCDebug(albumLog) << "GlobalStatus::setWindowDisactived - Function entry, value:" << value;
    if (m_bWindowDisactived != value) {
        qCDebug(albumLog) << "Setting window deactivated from" << m_bWindowDisactived << "to" << value;
        m_bWindowDisactived = value;
        Q_EMIT windowDisactivedChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setWindowDisactived - Function exit";
}

bool GlobalStatus::loading() const
{
    // qCDebug(albumLog) << "GlobalStatus::loading - Function entry, returning:" << m_bLoading;
    return m_bLoading;
}

void GlobalStatus::setLoading(const bool &value)
{
    qCDebug(albumLog) << "GlobalStatus::setLoading - Function entry, value:" << value;
    if (m_bLoading != value) {
        qCDebug(albumLog) << "Setting loading state from" << m_bLoading << "to" << value;
        m_bLoading = value;
        if (!m_bLoading) {
            qCDebug(albumLog) << "Enabling fade in/out animation after loading";
            setEnableFadeInoutAnimation(true);
        }
        Q_EMIT loadingChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setLoading - Function exit";
}

bool GlobalStatus::enableRatioAnimation() const
{
    // qCDebug(albumLog) << "GlobalStatus::enableRatioAnimation - Function entry, returning:" << m_bEnableRatioAnimation;
    return m_bEnableRatioAnimation;
}

void GlobalStatus::setEnableRatioAnimation(const bool &value)
{
    qCDebug(albumLog) << "GlobalStatus::setEnableRatioAnimation - Function entry, value:" << value;
    if (m_bEnableRatioAnimation != value) {
        qCDebug(albumLog) << "Setting ratio animation from" << m_bEnableRatioAnimation << "to" << value;
        m_bEnableRatioAnimation = value;
        Q_EMIT enableRatioAnimationChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setEnableRatioAnimation - Function exit";
}

bool GlobalStatus::enableFadeInoutAnimation() const
{
    // qCDebug(albumLog) << "GlobalStatus::enableFadeInoutAnimation - Function entry, returning:" << m_bEnableFadeInoutAnimation;
    return m_bEnableFadeInoutAnimation;

}

void GlobalStatus::setEnableFadeInoutAnimation(const bool &value)
{
    qCDebug(albumLog) << "GlobalStatus::setEnableFadeInoutAnimation - Function entry, value:" << value;
    if (m_bEnableFadeInoutAnimation != value) {
        qCDebug(albumLog) << "Setting fade in/out animation from" << m_bEnableFadeInoutAnimation << "to" << value;
        m_bEnableFadeInoutAnimation = value;
        Q_EMIT enableFadeInoutAnimationChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setEnableFadeInoutAnimation - Function exit";
}

bool GlobalStatus::enteringImageViewer() const
{
    // qCDebug(albumLog) << "GlobalStatus::enteringImageViewer - Function entry, returning:" << m_bEnteringImageViewer;
    return m_bEnteringImageViewer;
}

void GlobalStatus::setEnteringImageViewer(const bool &value)
{
    qCDebug(albumLog) << "GlobalStatus::setEnteringImageViewer - Function entry, value:" << value;
    if (m_bEnteringImageViewer != value) {
        qCDebug(albumLog) << "Setting entering image viewer from" << m_bEnteringImageViewer << "to" << value;
        m_bEnteringImageViewer = value;
        Q_EMIT enteringImageViewerChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setEnteringImageViewer - Function exit";
}

bool GlobalStatus::backingToMainAlbumView() const
{
    // qCDebug(albumLog) << "GlobalStatus::backingToMainAlbumView - Function entry, returning:" << m_bBackingToMainAlbumView;
    return m_bBackingToMainAlbumView;
}

void GlobalStatus::setBackingToMainAlbumView(const bool &value)
{
    qCDebug(albumLog) << "GlobalStatus::setBackingToMainAlbumView - Function entry, value:" << value;
    if (m_bBackingToMainAlbumView != value) {
        qCDebug(albumLog) << "Setting backing to main album view from" << m_bBackingToMainAlbumView << "to" << value;
        m_bBackingToMainAlbumView = value;
        Q_EMIT backingToMainAlbumViewChanged();
    }
    qCDebug(albumLog) << "GlobalStatus::setBackingToMainAlbumView - Function exit";
}

void GlobalStatus::initConnect()
{
    qCDebug(albumLog) << "Initializing connections";
    // 数据库监听-删除图片后通知前端刷新自定义相册视图内容
    connect(AlbumControl::instance(), SIGNAL(sigRefreshCustomAlbum(int)), SIGNAL(sigFlushCustomAlbumView(int)));

    // 数据库监听-删除图片后通知前端刷新合集所有项目
    connect(AlbumControl::instance(), SIGNAL(sigRefreshAllCollection()), SIGNAL(sigFlushAllCollectionView()));

    //数据库监听-删除图片后通知前端刷新已导入视图内容
    connect(AlbumControl::instance(), &AlbumControl::sigRefreshImportAlbum, this, [=]() {
        qCDebug(albumLog) << "Refreshing import album view";
        sigFlushHaveImportedView();
        setRefreshRangeBtnState(!m_bRefreshRangeBtnState);
    });

    // 数据库监听-删除图片后通知前端刷新搜索结果视图内容
    connect(AlbumControl::instance(), &AlbumControl::sigRefreshSearchView, this, [=]() {
        if (m_currentViewType == Types::ViewSearchResult) {
            qCDebug(albumLog) << "Refreshing search view";
            sigFlushSearchView();
        }
    });

    // 自动导入相册有新增相册，通知前端刷新侧边栏自动导入相册列表
    connect(AlbumControl::instance(), &AlbumControl::sigRefreshSlider, this, [=]() {
        qCDebug(albumLog) << "Refreshing sidebar import album list";
        setAlbumImportChangeList(!m_bAlbumImportChangeList);
    });
    qCDebug(albumLog) << "GlobalStatus::initConnect - Function exit";
}

QString GlobalStatus::getSelectedNumText(const QStringList &paths, const QString &text, const QString &devicePath)
{
    qCDebug(albumLog) << "GlobalStatus::getSelectedNumText - Function entry, paths:" << paths << "text:" << text << "devicePath:" << devicePath;
    QList<int> ret = AlbumControl::instance()->getPicVideoCountFromPaths(paths, devicePath);

    //QML的翻译不支持%n的特性，只能拆成这种代码
    int photoCount = ret[0];
    int videoCount = ret[1];
    QString selectedNumText("");
    if(paths.size() == 0) {
        selectedNumText = text;
    } else if(paths.size() == 1 && photoCount == 1) {
        selectedNumText = tr("1 item selected (1 photo)");
    } else if(paths.size() == 1 && videoCount == 1) {
        selectedNumText = tr("1 item selected (1 video)");
    } else if(photoCount > 1 && videoCount == 0) {
        selectedNumText = tr("%1 items selected (%1 photos)").arg(photoCount);
    } else if(videoCount > 1 && photoCount == 0) {
        selectedNumText = tr("%1 items selected (%1 videos)").arg(videoCount);
    } else if (photoCount == 1 && videoCount == 1) {
        selectedNumText = tr("%1 item selected (1 photo, 1 video)").arg(photoCount + videoCount);
    } else if (photoCount == 1 && videoCount > 1) {
        selectedNumText = tr("%1 items selected (1 photo, %2 videos)").arg(photoCount + videoCount).arg(videoCount);
    } else if (videoCount == 1 && photoCount > 1) {
        selectedNumText = tr("%1 items selected (%2 photos, 1 video)").arg(photoCount + videoCount).arg(photoCount);
    } else if (photoCount > 1 && videoCount > 1){
        selectedNumText = tr("%1 items selected (%2 photos, %3 videos)").arg(photoCount + videoCount).arg(photoCount).arg(videoCount);
    }

    qCDebug(albumLog) << "GlobalStatus::getSelectedNumText - Function exit, returning:" << selectedNumText;
    return selectedNumText;
}
