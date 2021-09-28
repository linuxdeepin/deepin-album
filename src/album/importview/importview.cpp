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
#include "importview.h"
#include <DApplicationHelper>
#include <DFileDialog>
#include <QGraphicsDropShadowEffect>
#include <DSuggestButton>
#include <DGuiApplicationHelper>
#include <dgiovolumemanager.h>
#include <dgiofile.h>
#include <dgiofileinfo.h>
#include <dgiovolume.h>
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "utils/unionimage.h"
#include "imageengine/imageengineapi.h"
#include "imagedataservice.h"
#include "ac-desktop-define.h"
#include "movieservice.h"

ImportView::ImportView()
    : m_pImportBtn(nullptr), pLabel(nullptr)
{
    setAcceptDrops(true);
    AC_SET_OBJECT_NAME(this, Import_Image_View);
    AC_SET_ACCESSIBLE_NAME(this, Import_Image_View);
    initUI();
    initConnections();
}

void ImportView::initConnections()
{
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this, &ImportView::onThemeTypeChanged);
}

void ImportView::initUI()
{
    QVBoxLayout *pImportFrameLayout = new QVBoxLayout();
    pLabel = new DLabel();
    pLabel->setFixedSize(128, 128);
    QPixmap pixmap;
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        pixmap = utils::base::renderSVG(":/resources/images/other/icon_import_photo.svg", QSize(128, 128));
    }
    if (themeType == DGuiApplicationHelper::DarkType) {
        pixmap = utils::base::renderSVG(":/resources/images/other/icon_import_photo_dark.svg", QSize(128, 128));
    }
    pLabel->setPixmap(pixmap);
    m_pImportBtn = new DSuggestButton();
    AC_SET_OBJECT_NAME(m_pImportBtn, Import_Image_View_Button);
    AC_SET_ACCESSIBLE_NAME(m_pImportBtn, Import_Image_View_Button);
    DFontSizeManager::instance()->bind(m_pImportBtn, DFontSizeManager::T6, QFont::ExtraLight);
    m_pImportBtn->setText(tr("Import Photos and Videos"));
    m_pImportBtn->setFixedSize(302, 36);
    m_pImportBtn->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));

    pDragLabel = new DLabel();
    DFontSizeManager::instance()->bind(pDragLabel, DFontSizeManager::T9, QFont::ExtraLight);
    pDragLabel->setForegroundRole(DPalette::TextTips);
    pDragLabel->setFixedHeight(18);
    pDragLabel->setText(tr("Or drag them here"));

    //#BUG90879，字体风格调整
    DPalette palette = DApplicationHelper::instance()->palette(pDragLabel);
    QColor color_TTT = palette.color(DPalette::ToolTipText);
    if (themeType == DGuiApplicationHelper::LightType) {
        color_TTT.setAlphaF(0.4);
        palette.setBrush(DPalette::Text, color_TTT);
        pDragLabel->setForegroundRole(DPalette::Text);
        pDragLabel->setPalette(palette);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_TTT.setAlphaF(0.5);
        palette.setBrush(DPalette::Text, color_TTT);
        pDragLabel->setForegroundRole(DPalette::Text);
        pDragLabel->setPalette(palette);
    }

    pImportFrameLayout->setMargin(0);
    pImportFrameLayout->addStretch();
    pImportFrameLayout->addWidget(pLabel, 0, Qt::AlignCenter);
    pImportFrameLayout->addSpacing(5);
    pImportFrameLayout->addWidget(m_pImportBtn, 0, Qt::AlignCenter);
    pImportFrameLayout->addSpacing(10);
    pImportFrameLayout->addWidget(pDragLabel, 0, Qt::AlignCenter);
    pImportFrameLayout->addStretch();
    setLayout(pImportFrameLayout);
}

void ImportView::mousePressEvent(QMouseEvent *e)
{
    setFocus();
    DWidget::mousePressEvent(e);
}

void ImportView::dragEnterEvent(QDragEnterEvent *e)
{
    bool bData = utils::base::checkMimeUrls(e->mimeData()->urls());
    if (!bData) {
        return;
    }
    e->setDropAction(Qt::CopyAction);
    e->accept();
}

void ImportView::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) {
        return;
    }
    ImageEngineApi::instance()->ImportImagesFromUrlList(urls, m_albumname, this);
    event->accept();
}

void ImportView::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void ImportView::onThemeTypeChanged()
{
    QPixmap pixmap;
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        pixmap = utils::base::renderSVG(":/resources/images/other/icon_import_photo.svg", QSize(128, 128));
    }
    if (themeType == DGuiApplicationHelper::DarkType) {
        pixmap = utils::base::renderSVG(":/resources/images/other/icon_import_photo_dark.svg", QSize(128, 128));
    }
    pLabel->setPixmap(pixmap);

    //#BUG90879，字体风格调整
    DPalette palette = DApplicationHelper::instance()->palette(pDragLabel);
    QColor color_TTT = palette.color(DPalette::ToolTipText);
    if (themeType == DGuiApplicationHelper::LightType) {
        color_TTT.setAlphaF(0.4);
        palette.setBrush(DPalette::Text, color_TTT);
        pDragLabel->setForegroundRole(DPalette::Text);
        pDragLabel->setPalette(palette);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_TTT.setAlphaF(0.5);
        palette.setBrush(DPalette::Text, color_TTT);
        pDragLabel->setForegroundRole(DPalette::Text);
        pDragLabel->setPalette(palette);
    }
}

void ImportView::onImprotBtnClicked(bool useDialog, const QStringList &list)
{
    static QStringList sList;
    for (const QString &i : UnionImage_NameSpace::unionImageSupportFormat())
        sList << ("*." + i);
    //添加视频过滤
    for (const QString &i : utils::base::m_videoFiletypes) {
        sList << i;
    }
    QString filter = tr("All photos and videos");
    filter.append('(');
    filter.append(sList.join(" "));
    filter.append(')');
    static QString cfgGroupName = QStringLiteral("General"), cfgLastOpenPath = QStringLiteral("LastOpenPath");
    QString pictureFolder = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QDir existChecker(pictureFolder);
    if (!existChecker.exists()) {
        pictureFolder = QDir::currentPath();
    }
    pictureFolder = dApp->setter->value(cfgGroupName, cfgLastOpenPath, pictureFolder).toString();
    DFileDialog dialog(this);
    dialog.setObjectName("ImportViewDialog");
    dialog.setFileMode(DFileDialog::ExistingFiles);
    dialog.setDirectory(pictureFolder);
    dialog.setNameFilter(filter);
    dialog.setOption(QFileDialog::HideNameFilterDetails);
    dialog.setWindowTitle(tr("Import Photos and Videos"));
    dialog.setAllowMixedSelection(true);

    if (useDialog) {
        if (dialog.exec() != QDialog::Accepted) {
            emit dApp->signalM->sigImportFailedToView();
            return;
        }
    }

    QStringList file_list = useDialog ? dialog.selectedFiles() : list;
    if (file_list.isEmpty()) {
        qDebug() << "file_list.isEmpty()";
        emit dApp->signalM->sigImportFailedToView();
        emit dApp->signalM->ImportFailed();
        return;
    }

//    ImageEngineApi::instance()->makeThumbnailByPaths(file_list);
    ImageEngineApi::instance()->ImportImagesFromFileList(file_list, m_albumname, this, true);
}


bool ImportView::imageImported(bool success)
{
    emit dApp->signalM->closeWaitDialog();
    if (!success) {
        emit dApp->signalM->sigImportFailedToView();
    }
    return true;
}

void ImportView::setAlbumname(const QString &name)
{
    m_albumname = name;
}



