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
#include "batchoperatewidget.h"
#include "application.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "utils/unionimage.h"

#include "dbmanager/dbmanager.h"
#include "controller/configsetter.h"
#include "widgets/elidedlabel.h"
#include "controller/signalmanager.h"
#include "imageengine/imageengineapi.h"
#include "ac-desktop-define.h"

#include <QTimer>
#include <QScroller>
#include <QScrollBar>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QDebug>
#include <QPainterPath>
#include <DLabel>
#include <QAbstractItemModel>
#include <DImageButton>
#include <DThumbnailProvider>
#include <DApplicationHelper>
#include <DSpinner>
#include <DWarningButton>
#include <QtMath>
#include <DHiDPIHelper>

#include "thumbnaillistview.h"
#include "expansionmenu.h"
#include "imgdeletedialog.h"
#include "albumgloabl.h"

DWIDGET_USE_NAMESPACE

BatchOperateWidget::BatchOperateWidget(ThumbnailListView *thumbnailListView, OperateType type, QWidget *parent)
    : QWidget(parent)
{
    m_thumbnailListView = thumbnailListView;
    m_thumbnailListView->setBatchOperateWidget(this);
    m_operateType = type;
    initUI();
    initConnection();
}

BatchOperateWidget::~BatchOperateWidget()
{
}
//初始化信号槽
void BatchOperateWidget::initConnection()
{
    //缩略图选中项发生变化
    connect(m_thumbnailListView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &BatchOperateWidget::sltSelectionChanged);
    //收藏
    connect(m_collection, &DToolButton::clicked, this, &BatchOperateWidget::sltCollectSelect);
    //左旋转
    connect(m_leftRotate, &DToolButton::clicked, this, &BatchOperateWidget::sltLeftRotate);
    //右旋转
    connect(m_rightRotate, &DToolButton::clicked, this, &BatchOperateWidget::sltRightRotate);
    //删除
    connect(m_delete, &DToolButton::clicked, this, &BatchOperateWidget::sltRemoveSelect);
    //全选
    connect(m_chooseAll, &DCommandLinkButton::clicked, this, &BatchOperateWidget::sltSelectAll);
    //取消全选
    connect(m_cancelChooseAll, &DCommandLinkButton::clicked, this, &BatchOperateWidget::sltUnSelectAll);
    //进入选择状态
    connect(m_startBatchSelect, &DCommandLinkButton::clicked, this, &BatchOperateWidget::sltBatchSelectChanged);
    //退出选择状态
    connect(m_cancelBatchSelect, &DCommandLinkButton::clicked, this, &BatchOperateWidget::sltBatchSelectChanged);
    //筛选条件变化
    connect(m_ToolButton, &FilterWidget::currentItemChanged, this, &BatchOperateWidget::sltCurrentFilterChanged);
    //点击最近删除恢复按钮
    connect(m_trashRecoveryBtn, &DPushButton::clicked, this, &BatchOperateWidget::onTrashRecoveryBtnClicked);
    //点击最近删除中删除按钮
    connect(m_trashDeleteBtn, &DPushButton::clicked, this, &BatchOperateWidget::onTrashDeleteBtnClicked);
    //主题变化
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &BatchOperateWidget::onThemeTypeChanged);
    //数据库变化
    //我的收藏
    connect(dApp->signalM, &SignalManager::removedFromAlbum, this, &BatchOperateWidget::sltAlbumChanged);
    connect(dApp->signalM, &SignalManager::insertedIntoAlbum, this, &BatchOperateWidget::sltAlbumChanged, Qt::QueuedConnection);
    //图片/视频插入删除
    viewChangedFlushTimer = new QTimer(this);
    connect(viewChangedFlushTimer, &QTimer::timeout, this, &BatchOperateWidget::sltListViewChanged);
    connect(m_thumbnailListView, &DListView::rowCountChanged, [this]() {
        if (!viewChangedFlushFlag) {
            viewChangedFlushTimer->start(200);
            viewChangedFlushFlag = true;
        }
    });
}

void BatchOperateWidget::sltAlbumChanged(int UID, const QStringList &paths)
{
    Q_UNUSED(paths)
    if (UID == DBManager::SpUID::u_Favorite) {
        refreshCollectBtn();
    }
}

void BatchOperateWidget::sltListViewChanged()
{
    viewChangedFlushTimer->stop();
    viewChangedFlushFlag = false;

    ExpansionPanel::FilteData data;
    data.type = m_ToolButton->getFilteType();
    sltCurrentFilterChanged(data);

    batchSelectChanged(false, true);
}

bool BatchOperateWidget::isAllSelectedCollected()
{
    QStringList paths =  m_thumbnailListView->selectedPaths();
    if (paths.isEmpty()) {
        return false;
    }

    return DBManager::instance()->isAllImgExistInAlbum(DBManager::SpUID::u_Favorite, paths, AlbumDBType::Favourite);
}
//刷新恢复与删除按钮是否可用状态
void BatchOperateWidget::refreshTrashBtnState()
{
    QStringList paths =  m_thumbnailListView->selectedPaths();
    m_trashRecoveryBtn->setEnabled(paths.size() > 0);
    m_trashDeleteBtn->setEnabled(paths.size() > 0);
}
//进入选择状态
void BatchOperateWidget::sltBatchSelectChanged()
{
    qDebug() << __FUNCTION__ << "---";
    DCommandLinkButton *btn = qobject_cast<DCommandLinkButton *>(sender());
    if (btn == m_startBatchSelect) {
        batchSelectChanged(true, false);
    } else {
        batchSelectChanged(false, true);
    }
    refreshBtnEnabled();
    m_thumbnailListView->setFocus();
}
//全选
void BatchOperateWidget::sltSelectAll()
{
    m_thumbnailListView->selectAllByItemType(m_ToolButton->getFilteType());
    m_thumbnailListView->slotChangeAllSelectBtnVisible(true);
    m_thumbnailListView->updatetimeLimeBtnText();
    m_thumbnailListView->setFocus();
    m_chooseAll->setVisible(false);
    m_cancelChooseAll->setVisible(true);
}
//取消全选
void BatchOperateWidget::sltUnSelectAll()
{
    m_thumbnailListView->clearSelection();
    m_thumbnailListView->slotChangeAllSelectBtnVisible(true);
    m_thumbnailListView->updatetimeLimeBtnText();
    m_thumbnailListView->setFocus();
    m_chooseAll->setVisible(true);
    m_cancelChooseAll->setVisible(false);
}
//删除选中项
void BatchOperateWidget::sltRemoveSelect(bool checked)
{
    Q_UNUSED(checked)
    qDebug() << __FUNCTION__ << "---";
    QStringList paths = m_thumbnailListView->selectedPaths();
    m_thumbnailListView->removeSelectToTrash(paths);
    m_thumbnailListView->setFocus();
}
//收藏选中项
void BatchOperateWidget::sltCollectSelect(bool checked)
{
    Q_UNUSED(checked)
    QStringList paths =  m_thumbnailListView->selectedPaths();
    if (isAllSelectedCollected()) {
        //全部收藏，执行取消收藏动作
        DBManager::instance()->removeFromAlbum(DBManager::SpUID::u_Favorite, paths, AlbumDBType::Favourite);
    } else {
        //未全部收藏，执行收藏动作
        DBManager::instance()->insertIntoAlbum(DBManager::SpUID::u_Favorite, paths, AlbumDBType::Favourite);
        emit dApp->signalM->insertedIntoAlbum(DBManager::SpUID::u_Favorite, paths);
    }
    //收藏后刷新按钮状态，未通过insertIntoAlbum返回值判断是否成功
    if (isAllSelectedCollected()) {
        m_collection->setIcon(QIcon::fromTheme("dcc_ccollection"));
    } else {
        m_collection->setIcon(QIcon::fromTheme("dcc_collection_normal"));
    }
    m_thumbnailListView->setFocus();
}
//顺时针旋转
void BatchOperateWidget::sltRightRotate(bool checked)
{
    Q_UNUSED(checked)
    QStringList paths =  m_thumbnailListView->selectedPaths();
    //发送给子线程旋转图片
    emit ImageEngineApi::instance()->sigRotateImageFile(90, paths);
    m_thumbnailListView->setFocus();
}
//逆时针旋转
void BatchOperateWidget::sltLeftRotate(bool checked)
{
    Q_UNUSED(checked)
    QStringList paths =  m_thumbnailListView->selectedPaths();
    //发送给子线程旋转图片
    emit ImageEngineApi::instance()->sigRotateImageFile(-90, paths);
    m_thumbnailListView->setFocus();
}
//缩略图列表选中状态发生变化
void BatchOperateWidget::sltSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(selected)
    Q_UNUSED(deselected)
    if (m_thumbnailListView->getAppointTypeSelectItemCount(ItemTypeNull) == 0) {
        refreshBtnEnabled(true);
        return;
    }
    batchSelectChanged(true, false);
    refreshBtnEnabled();
    m_thumbnailListView->setFocus();
}

void BatchOperateWidget::sltCurrentFilterChanged(ExpansionPanel::FilteData &data)
{
    if (data.type == ItemType::ItemTypeNull) {
        //显示全部
        m_thumbnailListView->showAppointTypeItem(ItemType::ItemTypeNull);
    } else if (data.type == ItemType::ItemTypePic) {
        //显示图片
        m_thumbnailListView->showAppointTypeItem(ItemType::ItemTypePic);
    } else if (data.type == ItemType::ItemTypeVideo) {
        //显示视频
        m_thumbnailListView->showAppointTypeItem(ItemType::ItemTypeVideo);
    }
    //如果过滤会后数量<=0，则不可用
    m_startBatchSelect->setEnabled(m_thumbnailListView->getAppointTypeItemCount(m_ToolButton->getFilteType()) > 0);
    m_thumbnailListView->setFocus();
}
//点击最近删除恢复按钮
void BatchOperateWidget::onTrashRecoveryBtnClicked()
{
    QStringList paths;
    paths = m_thumbnailListView->selectedPaths();
    ImageEngineApi::instance()->recoveryImagesFromTrash(paths);
    m_thumbnailListView->setFocus();
}
//点击最近删除中删除按钮
void BatchOperateWidget::onTrashDeleteBtnClicked()
{
    QStringList paths = m_thumbnailListView->selectedPaths();
    ImgDeleteDialog *dialog = new ImgDeleteDialog(this, paths.size(), true);
    dialog->setObjectName("deteledialog");
    if (dialog->exec() > 0) {
        m_thumbnailListView->clearSelection();
        ImageEngineApi::instance()->moveImagesToTrash(paths, true, false);
    }

    refreshTrashBtnState();
    m_thumbnailListView->setFocus();
}
//主题变化
void BatchOperateWidget::onThemeTypeChanged(DGuiApplicationHelper::ColorType themeType)
{
    Q_UNUSED(themeType)
    DPalette ReBtn = DApplicationHelper::instance()->palette(m_trashRecoveryBtn);
    ReBtn.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
    m_trashRecoveryBtn->setPalette(ReBtn);

    DPalette DeBtn = DApplicationHelper::instance()->palette(m_trashDeleteBtn);
    DeBtn.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
    m_trashDeleteBtn->setPalette(DeBtn);
}
//初始化控件
void BatchOperateWidget::initUI()
{
    QHBoxLayout *hb = new QHBoxLayout(this);
    hb->setSpacing(10);
    hb->setContentsMargins(0, 0, 0, 0);
    this->setContentsMargins(0, 0, 0, 0);
    this->setLayout(hb);
    //初始化最近删除相关按钮
    initTrashBtn(hb);
    //筛选按钮
    initDropdown();
    hb->addWidget(m_ToolButton);
    m_ToolButton->setVisible(true);
    //收藏
    m_collection = new DToolButton(this);
    m_collection->setToolTip(QObject::tr("Favorite"));
    m_collection->setIconSize(QSize(36, 36));
    m_collection->setFixedSize(36, 36);
    m_collection->setCheckable(false);
    hb->addWidget(m_collection);
    m_collection->setVisible(false);
    //初始化不可用，选择图片后可用
    m_collection->setEnabled(false);
    //左旋转
    m_leftRotate = new DToolButton(this);
    m_leftRotate->setToolTip(QObject::tr("Rotate counterclockwise"));
    m_leftRotate->setIcon(QIcon::fromTheme("dcc_left"));
    m_leftRotate->setIconSize(QSize(36, 36));
    m_leftRotate->setFixedSize(36, 36);
    m_leftRotate->setCheckable(false);
    hb->addWidget(m_leftRotate);
    m_leftRotate->setVisible(false);
    //初始化不可用，选择图片后可用
    m_leftRotate->setEnabled(false);
    //右旋转
    m_rightRotate = new DToolButton(this);
    m_rightRotate->setToolTip(QObject::tr("Rotate clockwise"));
    m_rightRotate->setIcon(QIcon::fromTheme("dcc_right"));
    m_rightRotate->setIconSize(QSize(36, 36));
    m_rightRotate->setFixedSize(36, 36);
    m_rightRotate->setCheckable(false);
    hb->addWidget(m_rightRotate);
    m_rightRotate->setVisible(false);
    //初始化不可用，选择图片后可用
    m_rightRotate->setEnabled(false);
    //删除
    m_delete = new DToolButton(this);
    m_delete->setToolTip(QObject::tr("Delete"));
    m_delete->setIcon(QIcon::fromTheme("dcc_delete"));
    m_delete->setIconSize(QSize(36, 36));
    m_delete->setFixedSize(36, 36);
    m_delete->setCheckable(false);
    hb->addWidget(m_delete);
    m_delete->setVisible(false);
    //初始化不可用，选择图片后可用
    m_delete->setEnabled(false);
    //全选
    m_chooseAll = new DCommandLinkButton(QObject::tr("Select All"));
    hb->addWidget(m_chooseAll);
    m_chooseAll->setVisible(false);
    DFontSizeManager::instance()->bind(m_chooseAll, DFontSizeManager::T5);
    m_chooseAll->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
    //取消全选
    m_cancelChooseAll = new DCommandLinkButton(QObject::tr("Unselect All"));
    hb->addWidget(m_cancelChooseAll);
    m_cancelChooseAll->setVisible(false);
    DFontSizeManager::instance()->bind(m_cancelChooseAll, DFontSizeManager::T5);
    m_cancelChooseAll->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
    //进入选择状态
    m_startBatchSelect = new DCommandLinkButton(QObject::tr("Select"));//选择
    hb->addWidget(m_startBatchSelect);
    DFontSizeManager::instance()->bind(m_startBatchSelect, DFontSizeManager::T5);
    m_startBatchSelect->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
    //退出选择状态
    m_cancelBatchSelect = new DCommandLinkButton(QObject::tr("Cancel"));//取消
    hb->addWidget(m_cancelBatchSelect);
    m_cancelBatchSelect->setVisible(false);
    DFontSizeManager::instance()->bind(m_cancelBatchSelect, DFontSizeManager::T5);
    m_cancelBatchSelect->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
}
//初始化最近删除相关按钮
void BatchOperateWidget::initTrashBtn(QHBoxLayout *hb)
{
    m_trashRecoveryBtn = new DPushButton();
    AC_SET_OBJECT_NAME(m_trashRecoveryBtn, Album_Restore_Button);
    AC_SET_ACCESSIBLE_NAME(m_trashRecoveryBtn, Album_Restore_Button);
    m_trashRecoveryBtn->setText(QObject::tr("Restore"));
    m_trashRecoveryBtn->setEnabled(false);
    m_trashRecoveryBtn->setFixedSize(100, 36);
    m_trashRecoveryBtn->setVisible(false);

    DPalette ReBtn = DApplicationHelper::instance()->palette(m_trashRecoveryBtn);
    ReBtn.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
    m_trashRecoveryBtn->setPalette(ReBtn);

    hb->addWidget(m_trashRecoveryBtn);

    m_trashDeleteBtn = new DWarningButton();
    AC_SET_OBJECT_NAME(m_trashDeleteBtn, Album_Delete_Button);
    AC_SET_ACCESSIBLE_NAME(m_trashDeleteBtn, Album_Delete_Button);
    m_trashDeleteBtn->setText(QObject::tr("Delete"));
    m_trashDeleteBtn->setFixedSize(100, 36);
    m_trashDeleteBtn->setVisible(false);

    DPalette DeBtn = DApplicationHelper::instance()->palette(m_trashDeleteBtn);
    ReBtn.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
    m_trashDeleteBtn->setPalette(ReBtn);
    hb->addWidget(m_trashDeleteBtn);
}

void BatchOperateWidget::initDropdown()
{
    m_expansionMenu = new ExpansionMenu(this);
    m_ToolButton = m_expansionMenu->mainWidget();
    m_ToolButton->setText(QObject::tr("All"));
    //m_ToolButton->setIcon(DHiDPIHelper::loadNxPixmap(":/icons/deepin/builtin/icons/light/album_all_16px.svg"));
    m_ToolButton->setIcon(DHiDPIHelper::loadNxPixmap(":/icons/deepin/builtin/icons/dark/album_all_16px.svg"));
    ExpansionPanel::FilteData data;

    data.icon_r_light = QIcon::fromTheme("album_all");
    data.icon_r_dark  = QIcon::fromTheme("album_all_hover");
    data.icon_r_path  = "album_all";
    data.text = QObject::tr("All");
    data.type = ItemType::ItemTypeNull;
    m_expansionMenu->setDefaultFilteData(data);
    m_expansionMenu->addNewButton(data);

    data.icon_r_light = QIcon::fromTheme("album_pic");
    data.icon_r_dark  = QIcon::fromTheme("album_pic_hover");
    data.icon_r_path  = "album_pic";
    data.text = QObject::tr("Photos");
    data.type = ItemType::ItemTypePic;
    m_expansionMenu->addNewButton(data);

    data.icon_r_light = QIcon::fromTheme("album_video");
    data.icon_r_dark  = QIcon::fromTheme("album_video_hover");
    data.icon_r_path  = "album_video";
    data.text = QObject::tr("Videos");
    data.type = ItemType::ItemTypeVideo;
    m_expansionMenu->addNewButton(data);
}

void BatchOperateWidget::batchSelectChanged(bool isBatchSelect, bool disConnectSignal)
{
    if (isBatchSelect) {
        m_ToolButton->setVisible(false);
        //最近删除
        if (m_operateType == AlbumViewTrashType) {
            m_trashRecoveryBtn->setVisible(true);
            m_trashDeleteBtn->setVisible(true);
            //刷新按钮是否可用
            refreshTrashBtnState();
            //根据所有选中图片，更新收藏按钮状态
            m_collection->setVisible(false);
            m_leftRotate->setVisible(false);
            m_rightRotate->setVisible(false);
            m_delete->setVisible(false);
            //全选与取消全选按钮状态由是否全部选中刷新
            bool isAllSelected = m_thumbnailListView->isAllSelected(m_ToolButton->getFilteType());
            m_chooseAll->setVisible(!isAllSelected);
            m_cancelChooseAll->setVisible(isAllSelected);
            //进入选择状态后，进入按钮隐藏，退出按钮显示
            m_startBatchSelect->setVisible(false);
            m_cancelBatchSelect->setVisible(true);
            //发送给时间线，刷新悬浮控件选择按钮显隐状态
//            emit signalBatchSelectChanged(true);
        } else {
            m_trashRecoveryBtn->setVisible(false);
            m_trashDeleteBtn->setVisible(false);
            //根据所有选中图片，更新收藏按钮状态
            m_collection->setVisible(true);
            m_leftRotate->setVisible(true);
            m_rightRotate->setVisible(true);
            m_delete->setVisible(true);
            //全选与取消全选按钮状态由是否全部选中刷新
            bool isAllSelected = m_thumbnailListView->isAllSelected(m_ToolButton->getFilteType());
            qDebug() << __FUNCTION__ << "---isAllSelected = " << isAllSelected;
            m_chooseAll->setVisible(!isAllSelected);
            m_cancelChooseAll->setVisible(isAllSelected);
            //进入选择状态后，进入按钮隐藏，退出按钮显示
            m_startBatchSelect->setVisible(false);
            m_cancelBatchSelect->setVisible(true);

            m_thumbnailListView->slotChangeAllSelectBtnVisible(true);
            m_thumbnailListView->updatetimeLimeBtnText();
            //发送给时间线，刷新悬浮控件选择按钮显隐状态
            emit signalBatchSelectChanged(true);
        }
    } else {
        m_trashRecoveryBtn->setVisible(false);
        m_trashDeleteBtn->setVisible(false);
        m_ToolButton->setVisible(true);
        //根据所有选中图片，更新收藏按钮状态
        m_collection->setVisible(false);
        m_leftRotate->setVisible(false);
        m_rightRotate->setVisible(false);
        m_delete->setVisible(false);
        //全选与取消全选按钮状态由是否全部选中刷新
        m_chooseAll->setVisible(false);
        m_cancelChooseAll->setVisible(false);
        //进入选择状态后，进入按钮隐藏，退出按钮显示
        m_startBatchSelect->setVisible(true);
        m_cancelBatchSelect->setVisible(false);

        if (disConnectSignal) {
            disconnect(m_thumbnailListView->selectionModel(), &QItemSelectionModel::selectionChanged,
                       this, &BatchOperateWidget::sltSelectionChanged);
        }
        m_thumbnailListView->slotChangeAllSelectBtnVisible(false);
        m_thumbnailListView->updatetimeLimeBtnText();
        m_thumbnailListView->clearSelection();
        //发送给时间线，刷新悬浮控件选择按钮显隐状态
        emit signalBatchSelectChanged(false);
        if (disConnectSignal) {
            connect(m_thumbnailListView->selectionModel(), &QItemSelectionModel::selectionChanged,
                    this, &BatchOperateWidget::sltSelectionChanged);
        }
    }
}

void BatchOperateWidget::refreshBtnEnabled(bool noSelected)
{
    //选中项数量变化，更新按钮可用状态
    bool selectMultiple = !m_thumbnailListView->selectedPaths().isEmpty();
    m_collection->setEnabled(selectMultiple);
    m_leftRotate->setEnabled(selectMultiple);
    m_rightRotate->setEnabled(selectMultiple);
    m_delete->setEnabled(m_thumbnailListView->isSelectedCanUseDelete());
    //选择发生变化，刷新收藏按钮状态
    if (m_collection->isVisible()) {
        if (isAllSelectedCollected()) {
            m_collection->setIcon(QIcon::fromTheme("dcc_ccollection"));
        } else {
            m_collection->setIcon(QIcon::fromTheme("dcc_collection_normal"));
        }
    }
    //旋转按钮状态由是否全部都支持旋转确定
    bool supportRotate = m_thumbnailListView->isAllSelectedSupportRotate();
    m_leftRotate->setEnabled(supportRotate);
    m_rightRotate->setEnabled(supportRotate);
    //全选与取消全选按钮状态由是否全部选中刷新
    bool isAllSelected;
    //如果参数已经传进来了，就不需要再重新判断是否全选
    if (noSelected) {
        isAllSelected = false;
    } else {
        isAllSelected = m_thumbnailListView->isAllSelected(m_ToolButton->getFilteType());
    }
    if (m_cancelBatchSelect->isVisible()) {
        m_chooseAll->setVisible(!isAllSelected);
        m_cancelChooseAll->setVisible(isAllSelected);
    }
    if (m_operateType == AlbumViewTrashType) {
        refreshTrashBtnState();
    }
}

void BatchOperateWidget::refreshCollectBtn()
{
    if (isAllSelectedCollected()) {
        m_collection->setIcon(QIcon::fromTheme("dcc_ccollection"));
    } else {
        m_collection->setIcon(QIcon::fromTheme("dcc_collection_normal"));
    }
}

void BatchOperateWidget::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event)
    batchSelectChanged(false, true);
    refreshBtnEnabled();
}

void BatchOperateWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);

    m_thumbnailListView->setFocus();
}
