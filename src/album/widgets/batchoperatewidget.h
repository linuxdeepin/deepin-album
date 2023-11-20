// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BATCHOPERATEWIDGET_H
#define BATCHOPERATEWIDGET_H

#include <QWidget>
#include <QLabel>
#include "controller/viewerthememanager.h"
#include <DListWidget>
#include <DSpinner>
#include <QListWidget>
#include <DListView>
#include <QAbstractItemModel>
#include <QStandardItem>
#include "dbmanager/dbmanager.h"
#include <DAnchors>
#include <dimagebutton.h>
#include <DThumbnailProvider>
#include <QPropertyAnimation>
#include <QHBoxLayout>
#include <DIconButton>
#include <DBlurEffectWidget>
#include <DGuiApplicationHelper>
#include <DLabel>
#include <DToolButton>
#include <DCommandLinkButton>

#include "imageengine/imageengineobject.h"
#include "expansionpanel.h"

DWIDGET_USE_NAMESPACE

class ThumbnailListView;
class ExpansionMenu;
class FilterWidget;
class BatchOperateWidget : public QWidget
{
    Q_OBJECT
public:
    enum OperateType {
        NullType = 0,
        AllPicViewType,//所有照片
        TimeLineViewType,//时间线
        SearchViewType,//搜索
        AlbumViewImportTimeLineViewType,//相册-最近导入
        AlbumViewTrashType,//相册-已删除
        AlbumViewClassType,//相册-图片分类
        AlbumViewFavoriteType,//相册-收藏
        AlbumViewCustomType,//相册-自定义
        AlbumViewPhoneType//相册-设备
    };

public:
    explicit BatchOperateWidget(ThumbnailListView *thumbnailListView, OperateType type, QWidget *parent = nullptr);
    ~BatchOperateWidget() override;

    void initUI();
    //初始化最近删除相关按钮
    void initTrashBtn(QHBoxLayout *hb);
    //刷新按钮显隐状态
    void batchSelectChanged(bool isBatchSelect, bool disConnectSignal);
    //刷新按钮可用状态 isNoSelected:是否有选中，true为没有
    void refreshBtnEnabled(bool noSelected = false);
    //刷新收藏按钮
    void refreshCollectBtn();
    //判断全部选中项是否全部已收藏
    bool isAllSelectedCollected();
protected:
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
signals:
    //进入批量状态
    void signalBatchSelectChanged(bool isBatchSelect);
    //取消全选时通知标题栏刷新
    void sigCancelAll(bool cancel);
    //筛选条件按钮宽度变化信号
    void sigFilterBtnWidthChanged(int width);
public slots:
    //批量操作状态改变
    void sltBatchSelectChanged();
    //全选
    void sltSelectAll();
    //取消全选
    void sltUnSelectAll();
    //删除选中项
    void sltRemoveSelect(bool checked);
    //收藏选中项
    void sltCollectSelect(bool checked);
    //顺时针旋转图片
    void sltRightRotate(bool checked);
    //逆时针旋转图片
    void sltLeftRotate(bool checked);
    //缩略图选中项改变
    void sltSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    //筛选显示变化
    void sltCurrentFilterChanged(ExpansionPanel::FilteData &data);
    //点击最近删除恢复按钮
    void onTrashRecoveryBtnClicked();
    //点击最近删除中删除按钮
    void onTrashDeleteBtnClicked();
    //主题变化
    void onThemeTypeChanged(DGuiApplicationHelper::ColorType themeType);
    //我的收藏改变，需要接收变化的相册名
    void sltAlbumChanged(int UID, const QStringList &paths);
    //图片/视频插入删除
    void sltListViewChanged();
private:
    //初始化下拉菜单
    void initDropdown();
    //初始化信号槽
    void initConnection();
    //刷新恢复与删除按钮是否可用状态
    void refreshTrashBtnState();
private:
    ExpansionMenu       *m_expansionMenu        = nullptr;
    FilterWidget        *m_ToolButton = nullptr;
    //最近删除中使用按钮
    DPushButton         *m_trashRecoveryBtn = nullptr;
    DPushButton         *m_trashDeleteBtn = nullptr;

    DToolButton         *m_collection       = nullptr;
    DToolButton         *m_leftRotate       = nullptr;
    DToolButton         *m_rightRotate      = nullptr;
    DToolButton         *m_delete           = nullptr;
    DCommandLinkButton  *m_chooseAll        = nullptr;  //全选
    DCommandLinkButton  *m_cancelChooseAll  = nullptr;  //取消全选

    DCommandLinkButton  *m_startBatchSelect = nullptr;  //进入批量状态
    DCommandLinkButton  *m_cancelBatchSelect = nullptr; //退出选择状态

    ThumbnailListView   *m_thumbnailListView = nullptr;
    OperateType          m_operateType;
    QTimer *viewChangedFlushTimer = nullptr;
    bool viewChangedFlushFlag = false;
};

#endif // BATCHOPERATEWIDGET_H
