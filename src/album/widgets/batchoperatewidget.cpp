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

#include "imgviewlistview.h"
#include "thumbnaillistview.h"
#include "expansionmenu.h"
#include "imgdeletedialog.h"
#include "albumgloabl.h"

DWIDGET_USE_NAMESPACE

BatchOperateWidget::BatchOperateWidget(ThumbnailListView *thumbnailListView, OperateType type, QWidget *parent)
    : QWidget(parent)
{
    m_thumbnailListView = thumbnailListView;
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
}

bool BatchOperateWidget::isAllSelectedCollected()
{
    bool isCollected = true;
    QStringList paths =  m_thumbnailListView->selectedPaths();
    if (paths.isEmpty()) {
        isCollected = false;
        return isCollected;
    }
    for (int i = 0; i < paths.size(); i++) {
        if (!DBManager::instance()->isImgExistInAlbum(COMMON_STR_FAVORITES, paths.at(i), AlbumDBType::Favourite)) {
            isCollected = false;
            break;
        }
    }
    return isCollected;
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
        batchSelectChanged(true);
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

        disconnect(m_thumbnailListView->selectionModel(), &QItemSelectionModel::selectionChanged,
                   this, &BatchOperateWidget::sltSelectionChanged);
        m_thumbnailListView->slotChangeAllSelectBtnVisible(false);
        m_thumbnailListView->updatetimeLimeBtnText();
        m_thumbnailListView->clearSelection();
        //发送给时间线，刷新悬浮控件选择按钮显隐状态
        emit signalBatchSelectChanged(false);
        connect(m_thumbnailListView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &BatchOperateWidget::sltSelectionChanged);
    }
}
//全选
void BatchOperateWidget::sltSelectAll()
{
    m_thumbnailListView->selectAllByItemType(m_ToolButton->getFilteType());
    m_thumbnailListView->slotChangeAllSelectBtnVisible(true);
    m_thumbnailListView->updatetimeLimeBtnText();
    m_chooseAll->setVisible(false);
    m_cancelChooseAll->setVisible(true);
}
//取消全选
void BatchOperateWidget::sltUnSelectAll()
{
    m_thumbnailListView->clearSelection();
    m_thumbnailListView->slotChangeAllSelectBtnVisible(true);
    m_thumbnailListView->updatetimeLimeBtnText();
    m_chooseAll->setVisible(true);
    m_cancelChooseAll->setVisible(false);
}
//删除选中项
void BatchOperateWidget::sltRemoveSelect(bool checked)
{
    Q_UNUSED(checked)
    qDebug() << __FUNCTION__ << "---";
    m_thumbnailListView->removeSelectToTrash(m_thumbnailListView->selectedPaths());
}
//收藏选中项
void BatchOperateWidget::sltCollectSelect(bool checked)
{
    Q_UNUSED(checked)
    QStringList paths =  m_thumbnailListView->selectedPaths();
    if (isAllSelectedCollected()) {
        //全部收藏，执行取消收藏动作
        DBManager::instance()->removeFromAlbum(COMMON_STR_FAVORITES, paths, AlbumDBType::Favourite);
    } else {
        //未全部收藏，执行收藏动作
        DBManager::instance()->insertIntoAlbum(COMMON_STR_FAVORITES, paths, AlbumDBType::Favourite);
        emit dApp->signalM->insertedIntoAlbum(COMMON_STR_FAVORITES, paths);
    }
    //收藏后刷新按钮状态，未通过insertIntoAlbum返回值判断是否成功
    if (isAllSelectedCollected()) {
        m_collection->setIcon(QIcon::fromTheme("dcc_ccollection"));
    } else {
        m_collection->setIcon(QIcon::fromTheme("dcc_collection_normal"));
    }
}
//顺时针旋转
void BatchOperateWidget::sltRightRotate(bool checked)
{
    Q_UNUSED(checked)
    QStringList paths =  m_thumbnailListView->selectedPaths();
    //发送给子线程旋转图片
    for (int i = 0; i < paths.size(); i++) {
        emit ImageEngineApi::instance()->sigRotateImageFile(90, paths.at(i));
    }
}
//逆时针旋转
void BatchOperateWidget::sltLeftRotate(bool checked)
{
    Q_UNUSED(checked)
    QStringList paths =  m_thumbnailListView->selectedPaths();
    //发送给子线程旋转图片
    for (int i = 0; i < paths.size(); i++) {
        emit ImageEngineApi::instance()->sigRotateImageFile(-90, paths.at(i));
    }
}
//缩略图列表选中状态发生变化
void BatchOperateWidget::sltSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected)
    //选中项数量变化，更新按钮可用状态
    bool selectMultiple = m_thumbnailListView->selectedPaths().size() > 0;
    batchSelectChanged(true);
    m_collection->setEnabled(selectMultiple);
    m_leftRotate->setEnabled(selectMultiple);
    m_rightRotate->setEnabled(selectMultiple);
    m_delete->setEnabled(selectMultiple);
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
    bool isAllSelected = m_thumbnailListView->isAllSelected(m_ToolButton->getFilteType());
    if (m_cancelBatchSelect->isVisible()) {
        m_chooseAll->setVisible(!isAllSelected);
        m_cancelChooseAll->setVisible(isAllSelected);
    }
    if (m_operateType == AlbumViewTrashType) {
        refreshTrashBtnState();
    }
}

void BatchOperateWidget::sltCurrentFilterChanged(ExpansionPanel::FilteData &data)
{
    if (data.type == ItemInfoType::ItemTypeNull) {
        //显示全部
        m_thumbnailListView->showSelectedTypeItem(ItemInfoType::ItemTypeNull);
    } else if (data.type == ItemInfoType::ItemTypePic) {
        //显示图片
        m_thumbnailListView->showSelectedTypeItem(ItemInfoType::ItemTypePic);
    } else if (data.type == ItemInfoType::ItemTypeVideo) {
        //显示视频
        m_thumbnailListView->showSelectedTypeItem(ItemInfoType::ItemTypeVideo);
    }
    //如果过滤会后数量<=0，则不可用
    m_startBatchSelect->setEnabled(m_thumbnailListView->filterTypeItemCount(m_ToolButton->getFilteType()) > 0);
}
//点击最近删除恢复按钮
void BatchOperateWidget::onTrashRecoveryBtnClicked()
{
    QStringList paths;
    paths = m_thumbnailListView->selectedPaths();
    ImageEngineApi::instance()->recoveryImagesFromTrash(paths);
}
//点击最近删除中删除按钮
void BatchOperateWidget::onTrashDeleteBtnClicked()
{
    QStringList paths = m_thumbnailListView->selectedPaths();

    ImgDeleteDialog *dialog = new ImgDeleteDialog(this, paths.count(), false);
    dialog->setObjectName("deteledialog");
    if (dialog->exec() > 0) {
        ImageEngineApi::instance()->moveImagesToTrash(paths, true, false);
    }

    refreshTrashBtnState();
}
//主题变化
void BatchOperateWidget::onThemeTypeChanged(DGuiApplicationHelper::ColorType themeType)
{
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
    m_ToolButton->setIcon(QIcon::fromTheme("album_all"));
    ExpansionPanel::FilteData data;

    data.icon_r = QIcon::fromTheme("album_all");
    data.icon_r_path = "album_all";
    data.text = QObject::tr("All");
    data.type = ItemInfoType::ItemTypeNull;
    m_expansionMenu->setDefaultFilteData(data);
    m_expansionMenu->addNewButton(data);

    data.icon_r = QIcon::fromTheme("album_pic");
    data.icon_r_path = "album_pic";
    data.text = QObject::tr("Photos");
    data.type = ItemInfoType::ItemTypePic;
    m_expansionMenu->addNewButton(data);

    data.icon_r = QIcon::fromTheme("album_video");
    data.icon_r_path = "album_video";
    data.text = QObject::tr("Videos");
    data.type = ItemInfoType::ItemTypeVideo;
    m_expansionMenu->addNewButton(data);
}

void BatchOperateWidget::batchSelectChanged(bool isBatchSelect)
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
            if (isAllSelectedCollected()) {
                m_collection->setIcon(QIcon::fromTheme("dcc_ccollection"));
            } else {
                m_collection->setIcon(QIcon::fromTheme("dcc_collection_normal"));
            }
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

        m_thumbnailListView->slotChangeAllSelectBtnVisible(false);
        m_thumbnailListView->updatetimeLimeBtnText();
        m_thumbnailListView->clearSelection();
        //发送给时间线，刷新悬浮控件选择按钮显隐状态
        emit signalBatchSelectChanged(false);
    }
}

void BatchOperateWidget::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event)
    batchSelectChanged(false);
}

