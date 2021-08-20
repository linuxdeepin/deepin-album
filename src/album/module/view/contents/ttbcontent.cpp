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
#include "ttbcontent.h"
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
#include <QtMath>

#include "imgviewlistview.h"
#include "imgviewwidget.h"
#include "mainwindow.h"
DWIDGET_USE_NAMESPACE
namespace {
const int LEFT_MARGIN = 10;
const QSize ICON_SIZE = QSize(50, 50);
const int ICON_SPACING = 10;
const int FILENAME_MAX_LENGTH = 600;
const int RIGHT_TITLEBAR_WIDTH = 100;
const QString LOCMAP_SELECTED_DARK = ":/resources/dark/images/58 drak.svg";
const QString LOCMAP_NOT_SELECTED_DARK = ":/resources/dark/images/imagewithbg-dark.svg";
const QString LOCMAP_SELECTED_LIGHT = ":/resources/light/images/58.svg";
const QString LOCMAP_NOT_SELECTED_LIGHT = ":/resources/light/images/imagewithbg.svg";

const int TOOLBAR_MINIMUN_WIDTH = 782;
const int TOOLBAR_JUSTONE_WIDTH = 532;
const int RT_SPACING = 20;
const int TOOLBAR_HEIGHT = 60;

const int TOOLBAR_DVALUE = 114 + 8;

const int THUMBNAIL_WIDTH = 32;
const int THUMBNAIL_ADD_WIDTH = 32;
const int THUMBNAIL_LIST_ADJUST = 9;
const int THUMBNAIL_VIEW_DVALUE = 668;

const int LOAD_LEFT_RIGHT = 25;     //前后加载图片数（动态）

}  // namespace

TTBContent::TTBContent(bool inDB, QWidget *parent) : QLabel(parent)
{
    onThemeChanged(dApp->viewerTheme->getCurrentTheme());
    m_windowWidth = std::max(this->window()->width(),
                             ConfigSetter::instance()->value("MAINWINDOW", "WindowWidth").toInt());

    QHBoxLayout *hb = new QHBoxLayout(this);
    hb->setContentsMargins(LEFT_MARGIN, 0, LEFT_MARGIN, 3);
    hb->setSpacing(0);
    m_inDB = inDB;
#ifndef LITE_DIV
    m_returnBtn = new ReturnButton();
    m_returnBtn->setMaxWidth(RETURN_BTN_MAX);
    m_returnBtn->setMaximumWidth(RETURN_BTN_MAX);
    m_returnBtn->setObjectName("ReturnBtn");
    m_returnBtn->setToolTip(tr("Back"));

    m_folderBtn = new PushButton();
    m_folderBtn->setFixedSize(QSize(24, 24));
    m_folderBtn->setObjectName("FolderBtn");
    //AC_SET_OBJECT_NAME(m_folderBtn, Ttbcontent_Back_Button);
    //AC_SET_ACCESSIBLE_NAME(m_folderBtn, Ttbcontent_Back_Button);
    m_folderBtn->setToolTip("Image management");
    if (m_inDB) {
        hb->addWidget(m_returnBtn);
    } else {
        hb->addWidget(m_folderBtn);
    }
    hb->addSpacing(20);

    connect(m_returnBtn, &ReturnButton::clicked, this, [ = ] {
        emit clicked();
    });
    connect(m_folderBtn, &PushButton::clicked, this, [ = ] {
        emit clicked();
    });
    connect(m_returnBtn, &ReturnButton::returnBtnWidthChanged, this, [ = ] {
        updateFilenameLayout();
    });
#endif
    // Adapt buttons////////////////////////////////////////////////////////////
    m_backButton = new DIconButton(this);
    m_backButton->setFixedSize(ICON_SIZE);
    AC_SET_OBJECT_NAME(m_backButton, Ttbcontent_Back_Button);
    AC_SET_ACCESSIBLE_NAME(m_backButton, Ttbcontent_Back_Button);
    m_backButton->setIcon(QIcon::fromTheme("dcc_back"));
    m_backButton->setIconSize(QSize(36, 36));
    m_backButton->setToolTip(tr("Back"));

    hb->addWidget(m_backButton);
    hb->addStretch();
    //hb->addSpacing(ICON_SPACING * 5);
    connect(m_backButton, &DIconButton::clicked, this, &TTBContent::onBackButtonClicked);

    // preButton
    m_preButton = new DIconButton(this);
    AC_SET_OBJECT_NAME(m_preButton, Ttbcontent_Pre_Button);
    AC_SET_ACCESSIBLE_NAME(m_preButton, Ttbcontent_Pre_Button);
    m_preButton->setFixedSize(ICON_SIZE);
    m_preButton->setIcon(QIcon::fromTheme("dcc_previous"));
    m_preButton->setIconSize(QSize(36, 36));
    m_preButton->setToolTip(tr("Previous"));

    m_preButton->hide();

    connect(m_preButton, &DIconButton::clicked, this, &TTBContent::onPreButton);
    // nextButton
    m_nextButton = new DIconButton(this);
    AC_SET_OBJECT_NAME(m_nextButton, Ttbcontent_Next_Button);
    AC_SET_ACCESSIBLE_NAME(m_nextButton, Ttbcontent_Next_Button);
    m_nextButton->setFixedSize(ICON_SIZE);
    m_nextButton->setIcon(QIcon::fromTheme("dcc_next"));
    m_nextButton->setIconSize(QSize(36, 36));
    m_nextButton->setToolTip(tr("Next"));

    m_nextButton->hide();

    connect(parent, SIGNAL(sigResize()), this, SLOT(onResize()));
    connect(m_nextButton, &DIconButton::clicked, this, &TTBContent::onNextButton);

    hb->addWidget(m_preButton);
    hb->addSpacing(ICON_SPACING);
    hb->addWidget(m_nextButton);
    hb->addSpacing(ICON_SPACING);

    // adaptImageBtn
    m_adaptImageBtn = new DIconButton(this);
    AC_SET_OBJECT_NAME(m_adaptImageBtn, Ttbcontent_AdaptImg_Button);
    AC_SET_ACCESSIBLE_NAME(m_adaptImageBtn, Ttbcontent_AdaptImg_Button);
    m_adaptImageBtn->setFixedSize(ICON_SIZE);
    m_adaptImageBtn->setIcon(QIcon::fromTheme("dcc_11"));
    m_adaptImageBtn->setIconSize(QSize(36, 36));
    m_adaptImageBtn->setToolTip(tr("1:1 Size"));
    m_adaptImageBtn->setCheckable(true);


    hb->addWidget(m_adaptImageBtn);
    hb->addSpacing(ICON_SPACING);
    connect(m_adaptImageBtn, &DIconButton::clicked, this, &TTBContent::onAdaptImageBtnClicked);


    // adaptScreenBtn
    m_adaptScreenBtn = new DIconButton(this);
    m_adaptScreenBtn->setFixedSize(ICON_SIZE);
    AC_SET_OBJECT_NAME(m_adaptScreenBtn, Ttbcontent_AdaptScreen_Button);
    AC_SET_ACCESSIBLE_NAME(m_adaptScreenBtn, Ttbcontent_AdaptScreen_Button);
    m_adaptScreenBtn->setIcon(QIcon::fromTheme("dcc_fit"));
    m_adaptScreenBtn->setIconSize(QSize(36, 36));
    m_adaptScreenBtn->setToolTip(tr("Fit to window"));
//    m_adaptScreenBtn->setCheckable(true);


    hb->addWidget(m_adaptScreenBtn);
    hb->addSpacing(ICON_SPACING);
    connect(m_adaptScreenBtn, &DIconButton::clicked, this, &TTBContent::onAdaptScreenBtnClicked);

    // Collection button
    m_clBT = new DIconButton(this);
    m_clBT->setFixedSize(ICON_SIZE);
    m_clBT->setToolTip(tr("Favorite"));
    m_clBT->setIcon(QIcon::fromTheme("dcc_collection_normal"));
    m_clBT->setIconSize(QSize(36, 36));
    AC_SET_OBJECT_NAME(m_clBT, Ttbcontent_Collect_Button);
    AC_SET_ACCESSIBLE_NAME(m_clBT, Ttbcontent_Collect_Button);

    connect(m_clBT, &DIconButton::clicked, this, &TTBContent::onclBTClicked);

    hb->addWidget(m_clBT);
    hb->addSpacing(ICON_SPACING);

    // rotateLBtn
    m_rotateLBtn = new DIconButton(this);
    AC_SET_OBJECT_NAME(m_rotateLBtn, Ttbcontent_Rotate_Left_Button);
    AC_SET_ACCESSIBLE_NAME(m_rotateLBtn, Ttbcontent_Rotate_Left_Button);
    m_rotateLBtn->setFixedSize(ICON_SIZE);
    m_rotateLBtn->setIcon(QIcon::fromTheme("dcc_left"));
    m_rotateLBtn->setIconSize(QSize(36, 36));
    m_rotateLBtn->setToolTip(tr("Rotate counterclockwise"));
    hb->addWidget(m_rotateLBtn);
    hb->addSpacing(ICON_SPACING);
    connect(m_rotateLBtn, &DIconButton::clicked, this, &TTBContent::onRotateLBtnClicked);

    // rotateRBtn
    m_rotateRBtn = new DIconButton(this);
    AC_SET_OBJECT_NAME(m_rotateRBtn, Ttbcontent_Rotate_Right_Button);
    AC_SET_ACCESSIBLE_NAME(m_rotateRBtn, Ttbcontent_Rotate_Right_Button);
    m_rotateRBtn->setFixedSize(ICON_SIZE);
    m_rotateRBtn->setIcon(QIcon::fromTheme("dcc_right"));
    m_rotateRBtn->setIconSize(QSize(36, 36));
    m_rotateRBtn->setToolTip(tr("Rotate clockwise"));

    hb->addWidget(m_rotateRBtn);
    hb->addSpacing(ICON_SPACING + 8);
    connect(m_rotateRBtn, &DIconButton::clicked, this, &TTBContent::onRotateRBtnClicked);

    // imgListView
    m_imgListWidget = new MyImageListWidget(this);
    connect(m_imgListWidget, &MyImageListWidget::openImg, this, &TTBContent::feedBackCurrentIndex, Qt::QueuedConnection);
//    connect(m_imgListView, &MyImageListWidget::mouseLeftReleased, this, &TTBContent::updateScreen);

    //图片少的时候设置列表大小
    //todo设置大小
//    if (m_allfileslist.size() <= 3) {
//        m_imgListView->setFixedSize(QSize(TOOLBAR_DVALUE, TOOLBAR_HEIGHT));
//    } else {
//        m_imgListView->setFixedSize(QSize(qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_allfileslist.size() - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) - THUMBNAIL_VIEW_DVALUE + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT));
//    }

    hb->addWidget(m_imgListWidget);
    hb->addSpacing(ICON_SPACING + 14);

    m_trashBtn = new DIconButton(this);
    m_trashBtn->setFixedSize(ICON_SIZE);
    AC_SET_OBJECT_NAME(m_trashBtn, Ttbcontent_Trash_Button);
    AC_SET_ACCESSIBLE_NAME(m_trashBtn, Ttbcontent_Trash_Button);
    m_trashBtn->setIcon(QIcon::fromTheme("dcc_delete"));
    m_trashBtn->setIconSize(QSize(36, 36));
    m_trashBtn->setToolTip(tr("Delete"));

    hb->addWidget(m_trashBtn);
    hb->addSpacing(10);
    m_fileNameLabel = new ElidedLabel();

    connect(m_trashBtn, &DIconButton::clicked, this, &TTBContent::onTrashBtnClicked);
}

void TTBContent::setAllFileInfo(const SignalManager::ViewInfo &info)
{
    qDebug() << "---" << __FUNCTION__ << "---";
    if (info.paths.size() <= 1) {
        m_contentWidth = TOOLBAR_JUSTONE_WIDTH;
    } else if (info.paths.size() <= 3) {
        m_contentWidth = TOOLBAR_MINIMUN_WIDTH;
        m_preButton->setVisible(true);
        m_nextButton->setVisible(true);
    } else {
        m_preButton->setVisible(true);
        m_nextButton->setVisible(true);
        m_contentWidth = qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (info.paths.size() - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) + THUMBNAIL_LIST_ADJUST;
    }

    setFixedWidth(m_contentWidth);
    setFixedHeight(72);

    m_imgListWidget->setAllFile(info.dBImgInfos, info.path);
}

int TTBContent::getAllFileCount()
{
    if (m_imgListWidget) {
        return m_imgListWidget->getImgCount();
    } else {
        return -1;
    }
}

int TTBContent::itemLoadedSize()
{
    return m_imgListWidget->getImgCount();
}

void TTBContent::updateScreen()
{
//todo,不知道干啥的
}

void TTBContent::disCheckAdaptImageBtn()
{
    qDebug() << "---" << __FUNCTION__ << "---";
    m_adaptImageBtn->setChecked(false);
    badaptImageBtnChecked = false;
}
void TTBContent::disCheckAdaptScreenBtn()
{
    m_adaptScreenBtn->setChecked(false);
    badaptScreenBtnChecked = false;
}

void TTBContent::checkAdaptImageBtn()
{
    qDebug() << "---" << __FUNCTION__ << "---";
    m_adaptImageBtn->setChecked(true);
    badaptImageBtnChecked = true;
}
void TTBContent::checkAdaptScreenBtn()
{
    m_adaptScreenBtn->setChecked(true);
    badaptScreenBtnChecked = true;
}

void TTBContent::deleteImage()
{
    if (m_imgListWidget->getImgCount() == 0)
        return;
    //移除正在展示照片
    if (m_imgListWidget) {
        m_imgListWidget->removeCurrent();
    }
    emit removed();     //删除数据库图片
    onResize();
}

void TTBContent::updateFilenameLayout()
{
    using namespace utils::base;
    DFontSizeManager::instance()->bind(m_fileNameLabel, DFontSizeManager::T8);
    QFontMetrics fm(DFontSizeManager::instance()->get(DFontSizeManager::T8));
    QString filename = QFileInfo(m_currentpath).fileName();
    QString name;

    int strWidth = fm.boundingRect(filename).width();
    int leftMargin = 0;
    int m_leftContentWidth = 0;
#ifndef LITE_DIV
    if (m_inDB)
        m_leftContentWidth = m_returnBtn->buttonWidth() + 6
                             + (ICON_SIZE.width() + 2) * 6 + LEFT_SPACE;
    else {
        m_leftContentWidth = m_folderBtn->width()  + 8
                             + (ICON_SIZE.width() + 2) * 5 + LEFT_SPACE;
    }
#else
    // 39 为logo以及它的左右margin
    m_leftContentWidth = 5 + (ICON_SIZE.width() + 2) * 5 + 39;
#endif

    int ww = dApp->setter->value("MAINWINDOW",  "WindowWidth").toInt();
    m_windowWidth =  std::max(std::max(this->window()->geometry().width(), this->width()), ww);
    m_contentWidth = std::max(m_windowWidth - RIGHT_TITLEBAR_WIDTH + 2, 1);
    setFixedWidth(m_contentWidth);
//    setFixedWidth(dApp->getMainWindow()->width() - 20);
    m_contentWidth = this->width() - m_leftContentWidth;

    if (strWidth > m_contentWidth || strWidth > FILENAME_MAX_LENGTH) {
        name = fm.elidedText(filename, Qt::ElideMiddle, std::min(m_contentWidth - 32,
                                                                 FILENAME_MAX_LENGTH));
        strWidth = fm.boundingRect(name).width();
        leftMargin = std::max(0, (m_windowWidth - strWidth) / 2
                              - m_leftContentWidth - LEFT_MARGIN - 2);
    } else {
        leftMargin = std::max(0, (m_windowWidth - strWidth) / 2
                              - m_leftContentWidth - 6);
        name = filename;
    }

    m_fileNameLabel->setText(name, leftMargin);
}

void TTBContent::onBackButtonClicked()
{
    //2020/6/9 DJH 优化退出全屏，不再闪出退出全屏的间隙 31331
    this->setVisible(false);
    emit dApp->signalM->hideImageView();
    this->setVisible(true);
    emit dApp->signalM->sigPauseOrStart(false); //唤醒后台外设线程
#ifdef tablet_PC
    emit resetShoworHide();
#endif
}

void TTBContent::onAdaptImageBtnClicked()
{
    m_adaptImageBtn->setChecked(true);
    if (!badaptImageBtnChecked) {
        badaptImageBtnChecked = true;
        emit resetTransform(false);
    }
}

void TTBContent::onAdaptScreenBtnClicked()
{
    m_adaptScreenBtn->setChecked(true);
    if (!badaptScreenBtnChecked) {
        badaptScreenBtnChecked = true;
        emit resetTransform(true);
    }
}

void TTBContent::onclBTClicked()
{
    if (true == m_bClBTChecked) {
        DBManager::instance()->removeFromAlbum(COMMON_STR_FAVORITES, QStringList(m_currentpath), AlbumDBType::Favourite);
    } else {
        DBManager::instance()->insertIntoAlbum(COMMON_STR_FAVORITES, QStringList(m_currentpath), AlbumDBType::Favourite);
        emit dApp->signalM->insertedIntoAlbum(COMMON_STR_FAVORITES, QStringList(m_currentpath));
    }
}

void TTBContent::onRotateLBtnClicked()
{
    emit rotateCounterClockwise();
}

void TTBContent::onRotateRBtnClicked()
{
    emit rotateClockwise();
}

void TTBContent::onTrashBtnClicked()
{
    emit dApp->signalM->deleteByMenu();
}

void TTBContent::onNextButton()
{
    if (m_imgListWidget) {
        m_imgListWidget->openNext();
        m_preButton->setEnabled(true);
        m_nextButton->setEnabled(!m_imgListWidget->isLast());
    }
}

void TTBContent::onPreButton()
{
    if (m_imgListWidget) {
        m_imgListWidget->openPre();
        m_preButton->setEnabled(!m_imgListWidget->isFirst());
        m_nextButton->setEnabled(true);
    }
}

void TTBContent::onThemeChanged(ViewerThemeManager::AppTheme theme)
{
    Q_UNUSED(theme);
}

void TTBContent::setCurrentDir(const QString &text)
{
#ifndef LITE_DIV
    if (text == COMMON_STR_FAVORITES) {
        text = tr(COMMON_STR_FAVORITES);
    }
    m_returnBtn->setText(text);
#else
    Q_UNUSED(text)
#endif
}

void TTBContent::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    m_windowWidth =  this->window()->geometry().width();
    if (m_imgListWidget->getImgCount() <= 1) {
        m_contentWidth = TOOLBAR_JUSTONE_WIDTH;
    } else if (m_imgListWidget->getImgCount() <= 3) {
        m_contentWidth = TOOLBAR_MINIMUN_WIDTH;
        //todo设置大小
//        m_imgListView->setFixedSize(QSize(TOOLBAR_DVALUE, TOOLBAR_HEIGHT));
        m_imgListWidget->setFixedSize(QSize(TOOLBAR_DVALUE, TOOLBAR_HEIGHT));
    } else {
        m_contentWidth = qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_imgListWidget->getImgCount() - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) + THUMBNAIL_LIST_ADJUST;
        m_imgListWidget->setFixedSize(QSize(qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_imgListWidget->getImgCount() - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) - THUMBNAIL_VIEW_DVALUE + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT));
    }
    setFixedWidth(m_contentWidth);
}


void TTBContent::setImage(const QString &path)
{
    qDebug() << "---" << __FUNCTION__ << "---";
    if ((m_imgListWidget->getImgCount() > 0) && !QFileInfo(path).exists()) {
        emit dApp->signalM->picNotExists(true);
        m_currentpath = path;
        if (m_imgListWidget->getImgCount() == 1)
            return;
    } else {
        emit dApp->signalM->picNotExists(false);
    }
    m_currentpath = path;
    DBImgInfo info = m_imgListWidget->getImgInfo(path);
    //>1的判断用来解决右键打开时按钮状态不正确问题，不是很好，后期替换公共能力的时候使用新的方法判断
    if (info.image.isNull() && (m_imgListWidget->getImgCount() > 1)) {
        m_adaptImageBtn->setDisabled(true);
        m_adaptScreenBtn->setDisabled(true);
        m_rotateLBtn->setDisabled(true);
        m_rotateRBtn->setDisabled(true);
    } else {
        setCurrentItem();
    }
}

bool TTBContent::setCurrentItem()
{
    if (m_currentpath.isEmpty() || !QFileInfo(m_currentpath).exists()) {
        m_adaptImageBtn->setDisabled(true);
        m_adaptScreenBtn->setDisabled(true);
        m_rotateLBtn->setDisabled(true);
        m_rotateRBtn->setDisabled(true);
        m_trashBtn->setDisabled(true);
        m_clBT->setDisabled(true);
    } else {
        m_adaptImageBtn->setDisabled(false);
        m_adaptScreenBtn->setDisabled(false);

        updateScreen();
        m_trashBtn->setDisabled(false);

        if (UnionImage_NameSpace::canSave(m_currentpath) && QFile::permissions(m_currentpath).testFlag(QFile::WriteUser)) {
            m_rotateLBtn->setDisabled(false);
            m_rotateRBtn->setDisabled(false);
        } else {
            m_rotateLBtn->setEnabled(false);
            m_rotateRBtn->setEnabled(false);
        }
    }
    QString fileName = "";
    if (m_currentpath != "") {
        fileName = QFileInfo(m_currentpath).fileName();
    }
    emit dApp->signalM->updateFileName(fileName);
//    updateFilenameLayout();
    updateCollectButton();
    return true;
}

void TTBContent::updateCollectButton()
{
    if (m_currentpath.isEmpty()) {
        return;
    }
    if (DBManager::instance()->isImgExistInAlbum(COMMON_STR_FAVORITES, m_currentpath, AlbumDBType::Favourite)) {
        m_clBT->setToolTip(tr("Unfavorite"));
        m_clBT->setIcon(QIcon::fromTheme("dcc_ccollection"));
        m_clBT->setIconSize(QSize(36, 36));
        m_bClBTChecked = true;
    } else {
        m_clBT->setToolTip(tr("Favorite"));
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        Q_UNUSED(themeType);
        m_clBT->setIcon(QIcon::fromTheme("dcc_collection_normal"));
        m_clBT->setIconSize(QSize(36, 36));
        m_bClBTChecked = false;
    }
}

void TTBContent::onResize()
{
    m_windowWidth =  this->window()->geometry().width();
    if (m_imgListWidget->getImgCount() <= 1) {
        m_preButton->setVisible(false);
        m_nextButton->setVisible(false);
        m_contentWidth = TOOLBAR_JUSTONE_WIDTH;
    } else if (m_imgListWidget->getImgCount() <= 3) {
        m_contentWidth = TOOLBAR_MINIMUN_WIDTH;
        //todo设置大小
//        m_imgListView->setFixedSize(QSize(TOOLBAR_DVALUE, TOOLBAR_HEIGHT));
        m_imgListWidget->setFixedSize(QSize(TOOLBAR_DVALUE, TOOLBAR_HEIGHT));
    } else {
        m_contentWidth = qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_imgListWidget->getImgCount() - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) + THUMBNAIL_LIST_ADJUST;
        //todo设置大小
//        m_imgListView->setFixedSize(QSize(qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_filelist_size - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) - THUMBNAIL_VIEW_DVALUE + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT));
        m_imgListWidget->setFixedSize(QSize(qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_imgListWidget->getImgCount() - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) - THUMBNAIL_VIEW_DVALUE + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT));
    }
    setFixedWidth(m_contentWidth);
}
