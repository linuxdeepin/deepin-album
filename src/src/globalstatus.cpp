// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "globalstatus.h"
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
static const int sc_RightMenuSeparatorHeight = 12;   // 右键菜单分割层的高度
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
    qDebug() << "Initializing GlobalStatus";
    initConnect();
}

GlobalStatus::~GlobalStatus() {
    qDebug() << "Destroying GlobalStatus";
    // 在程序退出的过程中
    // 由于析构过成功会触发destoryed信号，从而导致qml上使用GlobalStatus对象的地方触发重新绑定，
    // 但此时本对象已经析构，qml上会使用一些已经析构掉的对象，导致崩溃
    disconnect(this, nullptr, nullptr, nullptr);
}

/**
   @return 返回是否全屏显示图片
 */
bool GlobalStatus::showFullScreen() const
{
    return storeshowFullScreen;
}

/**
   @brief 设置全屏显示图片
 */
void GlobalStatus::setShowFullScreen(bool value)
{
    if (value != storeshowFullScreen) {
        qDebug() << "Setting full screen from" << storeshowFullScreen << "to" << value;
        storeshowFullScreen = value;
        Q_EMIT showFullScreenChanged();
    }
}

/**
   @return 返回是否允许显示导航窗口
 */
bool GlobalStatus::enableNavigation() const
{
    return storeenableNavigation;
}

/**
   @brief 设置是否允许显示导航窗口
 */
void GlobalStatus::setEnableNavigation(bool value)
{
    if (value != storeenableNavigation) {
        qDebug() << "Setting navigation enabled from" << storeenableNavigation << "to" << value;
        storeenableNavigation = value;
        Q_EMIT enableNavigationChanged();
    }
}

/**
   @return 返回是否显示右键菜单
 */
bool GlobalStatus::showRightMenu() const
{
    return storeshowRightMenu;
}

/**
   @brief 设置是否显示右键菜单
 */
void GlobalStatus::setShowRightMenu(bool value)
{
    if (value != storeshowRightMenu) {
        qDebug() << "Setting right menu visibility from" << storeshowRightMenu << "to" << value;
        storeshowRightMenu = value;
        Q_EMIT showRightMenuChanged();
    }
}

/**
   @return 当前是否弹窗显示详细图像信息
 */
bool GlobalStatus::showImageInfo() const
{
    return storeshowImageInfo;
}

/**
   @brief 设置是否弹窗显示详细图像信息
 */
void GlobalStatus::setShowImageInfo(bool value)
{
    if (value != storeshowImageInfo) {
        qDebug() << "Setting image info visibility from" << storeshowImageInfo << "to" << value;
        storeshowImageInfo = value;
        Q_EMIT showImageInfoChanged();
    }
}

/**
   @return 返回滑动视图是否响应操作
 */
bool GlobalStatus::viewInteractive() const
{
    return storeviewInteractive;
}

/**
   @brief 设置滑动视图是否响应操作
 */
void GlobalStatus::setViewInteractive(bool value)
{
    if (value != storeviewInteractive) {
        qDebug() << "Setting view interactive from" << storeviewInteractive << "to" << value;
        storeviewInteractive = value;
        Q_EMIT viewInteractiveChanged();
    }
}

/**
   @return 返回滑动视图是否处于轻弹状态
 */
bool GlobalStatus::viewFlicking() const
{
    return storeviewFlicking;
}

/**
   @brief 设置当前滑动视图是否处于轻弹状态
 */
void GlobalStatus::setViewFlicking(bool value)
{
    if (value != storeviewFlicking) {
        qDebug() << "Setting view flicking from" << storeviewFlicking << "to" << value;
        storeviewFlicking = value;
        Q_EMIT viewFlickingChanged();
    }
}

/**
   @return 返回当前是否允许标题栏、底栏动画效果
 */
bool GlobalStatus::animationBlock() const
{
    return storeanimationBlock;
}

/**
   @brief 设置当前允许标题栏、底栏动画效果的标志值为 \a value
 */
void GlobalStatus::setAnimationBlock(bool value)
{
    if (value != storeanimationBlock) {
        qDebug() << "Setting animation block from" << storeanimationBlock << "to" << value;
        storeanimationBlock = value;
        Q_EMIT animationBlockChanged();
    }
}

/**
   @return 返回当前是否允许全屏展示动画
 */
bool GlobalStatus::fullScreenAnimating() const
{
    return storefullScreenAnimating;
}

/**
   @brief 设置当前是否允许全屏展示动画的标志值为 \a value
 */
void GlobalStatus::setFullScreenAnimating(bool value)
{
    if (value != storefullScreenAnimating) {
        qDebug() << "Setting full screen animating from" << storefullScreenAnimating << "to" << value;
        storefullScreenAnimating = value;
        Q_EMIT fullScreenAnimatingChanged();
    }
}

/**
   @return 返回当前缩略图列表允许显示的宽度
 */
int GlobalStatus::thumbnailVaildWidth() const
{
    return storethumbnailVaildWidth;
}

/**
   @brief 设置当前缩略图列表允许显示的宽度为 \a value
 */
void GlobalStatus::setThumbnailVaildWidth(int value)
{
    if (value != storethumbnailVaildWidth) {
        qDebug() << "Setting thumbnail valid width from" << storethumbnailVaildWidth << "to" << value;
        storethumbnailVaildWidth = value;
        Q_EMIT thumbnailVaildWidthChanged();
    }
}

/**
   @return 返回当前显示的界面索引
 */
Types::StackPage GlobalStatus::stackPage() const
{
    return storestackPage;
}

/**
   @brief 设置当前显示的界面索引为 \a value ，将切换显示的界面类型
 */
void GlobalStatus::setStackPage(Types::StackPage value)
{
    if (value != storestackPage) {
        qDebug() << "Setting stack page from" << storestackPage << "to" << value;
        storestackPage = value;
        Q_EMIT stackPageChanged();
    }
}

bool GlobalStatus::showExportDialog() const
{
    return storeshowExportDialog;
}

void GlobalStatus::setShowExportDialog(bool value)
{
    if (value != storeshowExportDialog) {
        qDebug() << "Setting export dialog visibility from" << storeshowExportDialog << "to" << value;
        storeshowExportDialog = value;
        Q_EMIT showExportDialogChanged();
    }
}

int GlobalStatus::minHeight() const
{
    return sc_MinHeight;
}

int GlobalStatus::minWidth() const
{
    return sc_MinWidth;
}

int GlobalStatus::minHideHeight() const
{
    return sc_MinHideHeight;
}

int GlobalStatus::floatMargin() const
{
    return sc_FloatMargin;
}

int GlobalStatus::titleHeight() const
{
    return sc_TitleHeight;
}

int GlobalStatus::thumbnailViewHeight() const
{
    return sc_ThumbnailViewHeight;
}

int GlobalStatus::showBottomY() const
{
    return sc_ShowBottomY;
}

int GlobalStatus::switchImageHotspotWidth() const
{
    return sc_SwitchImageHotspotWidth;
}

int GlobalStatus::actionMargin() const
{
    return sc_ActionMargin;
}

int GlobalStatus::rightMenuItemHeight() const
{
    return sc_RightMenuItemHeight;
}

double GlobalStatus::animationDefaultDuration() const
{
    return sc_AnimationDefaultDuration;
}

/**
   @brief 默认 PathView 在路径中的 Item 计数
   @note 会影响 PathView 相关的动画效果计算，修改此值需慎重考虑
 */
int GlobalStatus::pathViewItemCount() const
{
    return sc_PathViewItemCount;
}

void GlobalStatus::setFileControl(FileControl *fc)
{
    qDebug() << "Setting file control";
    m_fileControl = fc;

    if (!m_fileControl) {
        qWarning() << "File control is null";
        return;
    }

    bool bRet = false;
    m_nAnimationDuration = m_fileControl->getConfigValue("", "animationDuration", 400).toInt(&bRet);
    if (!bRet) {
        qWarning() << "Failed to get animation duration, using default: 400";
        m_nAnimationDuration = 400;
    }
    
    m_nLargeImagePreviewAnimationDuration = m_fileControl->getConfigValue("", "largeImagePreviewAnimationDuration", 800).toInt(&bRet);
    if (!bRet) {
        qWarning() << "Failed to get large image preview animation duration, using default: 800";
        m_nLargeImagePreviewAnimationDuration = 800;
    }
    
    m_bEnableSidebarAnimation = m_fileControl->getConfigValue("", "enableSidebarAnimation", 0).toInt(&bRet);
    if (!bRet) {
        qWarning() << "Failed to get sidebar animation setting, using default: false";
        m_bEnableSidebarAnimation = false;
    }
}

int GlobalStatus::rightMenuSeparatorHeight() const
{
    return sc_RightMenuSeparatorHeight;
}

int GlobalStatus::sideBarWidth() const
{
    return sc_SideBarWidth;
}

int GlobalStatus::statusBarHeight() const
{
    return sc_StatusBarHeight;
}

int GlobalStatus::collectionTopMargin() const
{
    return sc_CollectionTopMargin;
}

int GlobalStatus::thumbnailViewTitleHieght() const
{
    return sc_ThumbnailViewTitleHieght;
}

int GlobalStatus::verticalScrollBarWidth() const
{
    return sc_VerticalScrollBarWidth;
}

int GlobalStatus::rectSelScrollStep() const
{
    return sc_RectSelScrollStep;
}

int GlobalStatus::thumbnailListRightMargin() const
{
    return sc_ThumbnailListRightMargin;
}

int GlobalStatus::thumbnialListCellSpace() const
{
    return sc_ThumbnialListCellSpace;
}

int GlobalStatus::needHideSideBarWidth() const
{
    return sc_NeedHideSideBarWidth;
}

int GlobalStatus::animationDuration() const
{
    return m_nAnimationDuration;
}

int GlobalStatus::largeImagePreviewAnimationDuration() const
{
    return m_nLargeImagePreviewAnimationDuration;
}

bool GlobalStatus::sidebarAnimationEnabled() const
{
    return m_bEnableSidebarAnimation;
}

qreal GlobalStatus::sideBarX() const
{
    return m_sideBar_X;
}

void GlobalStatus::setSideBarX(const qreal& value)
{
    if (!qFuzzyCompare(m_sideBar_X, value)) {
        qDebug() << "Setting sidebar X from" << m_sideBar_X << "to" << value;
        m_sideBar_X = value;
        Q_EMIT sideBarXChanged();
    }
}

QVariantList GlobalStatus::selectedPaths() const
{
    return m_selectedPaths;
}

void GlobalStatus::setSelectedPaths(const QVariantList& value)
{
    if (m_selectedPaths != value) {
        qDebug() << "Setting selected paths, count:" << value.size();
        m_selectedPaths = value;
        Q_EMIT selectedPathsChanged();
    }
}

bool GlobalStatus::bRefreshFavoriteIconFlag() const
{
    return m_bRefreshFavoriteIconFlag;
}

void GlobalStatus::setBRefreshFavoriteIconFlag(const bool& value)
{
    if (m_bRefreshFavoriteIconFlag != value) {
        qDebug() << "Setting refresh favorite icon flag from" << m_bRefreshFavoriteIconFlag << "to" << value;
        m_bRefreshFavoriteIconFlag = value;
        Q_EMIT bRefreshFavoriteIconFlagChanged();
    }
}

bool GlobalStatus::refreshRangeBtnState() const
{
    return m_bRefreshRangeBtnState;
}

void GlobalStatus::setRefreshRangeBtnState(const bool& value)
{
    if (m_bRefreshRangeBtnState != value) {
        qDebug() << "Setting refresh range button state from" << m_bRefreshRangeBtnState << "to" << value;
        m_bRefreshRangeBtnState = value;
        Q_EMIT refreshRangeBtnStateChanged();
    }
}

Types::ThumbnailViewType GlobalStatus::currentViewType() const
{
    return m_currentViewType;
}

void GlobalStatus::setCurrentViewType(const Types::ThumbnailViewType &value)
{
    if (m_currentViewType != value) {
        qDebug() << "Setting current view type from" << m_currentViewType << "to" << value;
        m_currentViewType = value;

        setEnableRatioAnimation(false);
        setBackingToMainAlbumView(false);
        // 若相册数据库没有图片资源，则调整显示"没有图片"提示视图
        if (AlbumControl::instance()->getAllCount() <= 0) {
            qDebug() << "No images in album database, adjusting view type";
            switch (value) {
            case Types::ViewImport:
            case Types::ViewNoPicture:
            case Types::ViewCollecttion:
                break;
            case Types::ViewSearchResult:
                m_currentViewType = Types::ViewNoPicture;
                qDebug() << "Changing to no picture view for search result";
                break;
            default:
                break;
            }
        }
        Q_EMIT currentViewTypeChanged();
    }
}

int GlobalStatus::currentCollecttionViewIndex() const
{
    return m_currentCollecttionViewIndex;
}

void GlobalStatus::setCurrentCollecttionViewIndex(const int &value)
{
    if (m_currentCollecttionViewIndex != value) {
        qDebug() << "Setting collection view index from" << m_currentCollecttionViewIndex << "to" << value;
        m_currentCollecttionViewIndex = value;
        setEnableRatioAnimation(false);
        setBackingToMainAlbumView(false);
        Q_EMIT currentCollecttionViewIndexChanged();
    }
}

Types::SwitchType GlobalStatus::currentSwitchType() const
{
    return m_currentSwitchType;
}

void GlobalStatus::setCurrentSwitchType(const int &value)
{
    if (m_currentSwitchType != value) {
        qDebug() << "Setting switch type from" << m_currentSwitchType << "to" << value;
        m_currentSwitchType = static_cast<Types::SwitchType>(value);
        Q_EMIT currentSwitchTypeChanged();
    }
}

int GlobalStatus::currentCustomAlbumUId() const
{
    return m_currentCustomAlbumUId;
}

void GlobalStatus::setCurrentCustomAlbumUId(const int &value)
{
    if (m_currentCustomAlbumUId != value) {
        qDebug() << "Setting custom album UID from" << m_currentCustomAlbumUId << "to" << value;
        setBackingToMainAlbumView(false);
        m_currentCustomAlbumUId = value;
        Q_EMIT currentCustomAlbumUIdChanged();
    }
}

int GlobalStatus::stackControlCurrent() const
{
    return m_stackControlCurrent;
}

void GlobalStatus::setStackControlCurrent(const int &value)
{
    if (m_stackControlCurrent != value) {
        qDebug() << "Setting stack control current from" << m_stackControlCurrent << "to" << value;

        if (m_stackControlCurrent != 0 && value == 0) {
            qDebug() << "Moving to album animation";
            setBackingToMainAlbumView(true);
            Q_EMIT sigMoveToAlbumAnimation();
        } else {
            setBackingToMainAlbumView(false);
        }
        m_stackControlCurrent = value;
        Q_EMIT stackControlCurrentChanged();
    }
}

int GlobalStatus::stackControlLastCurrent() const
{
    return m_stackControlLastCurrent;
}

void GlobalStatus::setStackControlLastCurrent(const int &value)
{
    if (m_stackControlLastCurrent != value) {
        m_stackControlLastCurrent = value;
        Q_EMIT stackControlLastCurrentChanged();
    }
}

int GlobalStatus::thumbnailSizeLevel() const
{
    return m_thumbnailSizeLevel;
}

void GlobalStatus::setThumbnailSizeLevel(const int &value)
{
    if (m_thumbnailSizeLevel != value) {
        qDebug() << "Setting thumbnail size level from" << m_thumbnailSizeLevel << "to" << value;
        m_thumbnailSizeLevel = value;

        Q_EMIT thumbnailSizeLevelChanged();

        // 缩放等级有调整， 同步调整网格大小
        qreal newCellBaseWidth = m_thumbnailSizeLevel >= 0 && m_thumbnailSizeLevel <= 9 ? 80 + m_thumbnailSizeLevel * 10 : 80;
        qDebug() << "Adjusting cell base width to:" << newCellBaseWidth;
        setCellBaseWidth(newCellBaseWidth);
    }
}

qreal GlobalStatus::cellBaseWidth() const
{
    return m_cellBaseWidth;
}

void GlobalStatus::setCellBaseWidth(const qreal& value)
{
    if (!qFuzzyCompare(m_cellBaseWidth, value)) {
        qDebug() << "Setting cell base width from" << m_cellBaseWidth << "to" << value;
        m_cellBaseWidth = value;
        Q_EMIT cellBaseWidthChanged();
    }
}

QString GlobalStatus::statusBarNumText() const
{
    return m_statusBarNumText;
}

void GlobalStatus::setStatusBarNumText(const QString &value)
{
    if (m_statusBarNumText != value) {
        qDebug() << "Setting status bar text from" << m_statusBarNumText << "to" << value;
        m_statusBarNumText = value;
        Q_EMIT statusBarNumTextChanged();
    }
}

QString GlobalStatus::searchEditText() const
{
    return m_searchEditText;
}

void GlobalStatus::setSearchEditText(const QString &value)
{
    if (m_searchEditText != value) {
        qDebug() << "Setting search edit text from" << m_searchEditText << "to" << value;
        m_searchEditText = value;
        Q_EMIT searchEditTextChanged();
    }
}

bool GlobalStatus::albumImportChangeList() const
{
    return m_bAlbumImportChangeList;
}

void GlobalStatus::setAlbumImportChangeList(const bool &value)
{
    if (m_bAlbumImportChangeList != value) {
        qDebug() << "Setting album import change list from" << m_bAlbumImportChangeList << "to" << value;
        m_bAlbumImportChangeList = value;
        Q_EMIT albumImportChangeListChanged();
    }
}

bool GlobalStatus::albumChangeList() const
{
    return  m_bAlbumChangeList;
}

void GlobalStatus::setAlbumChangeList(const bool &value)
{
    if (m_bAlbumChangeList != value) {
        qDebug() << "Setting album change list from" << m_bAlbumChangeList << "to" << value;
        m_bAlbumChangeList = value;
        Q_EMIT albumChangeListChanged();
    }
}

bool GlobalStatus::sideBarIsVisible() const
{
    return  m_bSideBarIsVisible;
}

void GlobalStatus::setSideBarIsVisible(const bool &value)
{
    if (m_bSideBarIsVisible != value) {
        qDebug() << "Setting sidebar visibility from" << m_bSideBarIsVisible << "to" << value;
        m_bSideBarIsVisible = value;
        Q_EMIT sideBarIsVisibleChanged();
    }
}

QString GlobalStatus::currentDeviceName() const
{
    return m_currentDeviceName;
}

void GlobalStatus::setCurrentDeviceName(const QString &value)
{
    if (m_currentDeviceName != value) {
        qDebug() << "Setting current device name from" << m_currentDeviceName << "to" << value;
        m_currentDeviceName = value;
        Q_EMIT currentDeviceNameChanged();
    }
}

QString GlobalStatus::currentDevicePath() const
{
    return m_currentDevicePath;
}

void GlobalStatus::setCurrentDevicePath(const QString &value)
{
    if (m_currentDevicePath != value) {
        qDebug() << "Setting current device path from" << m_currentDevicePath << "to" << value;
        m_currentDevicePath = value;
        m_currentDeviceName = AlbumControl::instance()->getDeviceName(m_currentDevicePath);
        Q_EMIT currentDevicePathChanged();
    }
}

bool GlobalStatus::windowDisactived() const
{
    return m_bWindowDisactived;
}

void GlobalStatus::setWindowDisactived(const bool &value)
{
    if (m_bWindowDisactived != value) {
        qDebug() << "Setting window deactivated from" << m_bWindowDisactived << "to" << value;
        m_bWindowDisactived = value;
        Q_EMIT windowDisactivedChanged();
    }
}

bool GlobalStatus::loading() const
{
    return m_bLoading;
}

void GlobalStatus::setLoading(const bool &value)
{
    if (m_bLoading != value) {
        qDebug() << "Setting loading state from" << m_bLoading << "to" << value;
        m_bLoading = value;
        if (!m_bLoading) {
            qDebug() << "Enabling fade in/out animation after loading";
            setEnableFadeInoutAnimation(true);
        }
        Q_EMIT loadingChanged();
    }
}

bool GlobalStatus::enableRatioAnimation() const
{
    return m_bEnableRatioAnimation;
}

void GlobalStatus::setEnableRatioAnimation(const bool &value)
{
    if (m_bEnableRatioAnimation != value) {
        qDebug() << "Setting ratio animation from" << m_bEnableRatioAnimation << "to" << value;
        m_bEnableRatioAnimation = value;
        Q_EMIT enableRatioAnimationChanged();
    }
}

bool GlobalStatus::enableFadeInoutAnimation() const
{
    return m_bEnableFadeInoutAnimation;

}

void GlobalStatus::setEnableFadeInoutAnimation(const bool &value)
{
    if (m_bEnableFadeInoutAnimation != value) {
        qDebug() << "Setting fade in/out animation from" << m_bEnableFadeInoutAnimation << "to" << value;
        m_bEnableFadeInoutAnimation = value;
        Q_EMIT enableFadeInoutAnimationChanged();
    }
}

bool GlobalStatus::enteringImageViewer() const
{
    return m_bEnteringImageViewer;
}

void GlobalStatus::setEnteringImageViewer(const bool &value)
{
    if (m_bEnteringImageViewer != value) {
        qDebug() << "Setting entering image viewer from" << m_bEnteringImageViewer << "to" << value;
        m_bEnteringImageViewer = value;
        Q_EMIT enteringImageViewerChanged();
    }
}

bool GlobalStatus::backingToMainAlbumView() const
{
    return m_bBackingToMainAlbumView;
}

void GlobalStatus::setBackingToMainAlbumView(const bool &value)
{
    if (m_bBackingToMainAlbumView != value) {
        qDebug() << "Setting backing to main album view from" << m_bBackingToMainAlbumView << "to" << value;
        m_bBackingToMainAlbumView = value;
        Q_EMIT backingToMainAlbumViewChanged();
    }
}

void GlobalStatus::initConnect()
{
    qDebug() << "Initializing connections";
    // 数据库监听-删除图片后通知前端刷新自定义相册视图内容
    connect(AlbumControl::instance(), SIGNAL(sigRefreshCustomAlbum(int)), SIGNAL(sigFlushCustomAlbumView(int)));

    // 数据库监听-删除图片后通知前端刷新合集所有项目
    connect(AlbumControl::instance(), SIGNAL(sigRefreshAllCollection()), SIGNAL(sigFlushAllCollectionView()));

    //数据库监听-删除图片后通知前端刷新已导入视图内容
    connect(AlbumControl::instance(), &AlbumControl::sigRefreshImportAlbum, this, [=]() {
        qDebug() << "Refreshing import album view";
        sigFlushHaveImportedView();
        setRefreshRangeBtnState(!m_bRefreshRangeBtnState);
    });

    // 数据库监听-删除图片后通知前端刷新搜索结果视图内容
    connect(AlbumControl::instance(), &AlbumControl::sigRefreshSearchView, this, [=]() {
        if (m_currentViewType == Types::ViewSearchResult) {
            qDebug() << "Refreshing search view";
            sigFlushSearchView();
        }
    });

    // 自动导入相册有新增相册，通知前端刷新侧边栏自动导入相册列表
    connect(AlbumControl::instance(), &AlbumControl::sigRefreshSlider, this, [=]() {
        qDebug() << "Refreshing sidebar import album list";
        setAlbumImportChangeList(!m_bAlbumImportChangeList);
    });
}

QString GlobalStatus::getSelectedNumText(const QStringList &paths, const QString &text, const QString &devicePath)
{
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

    return selectedNumText;
}
