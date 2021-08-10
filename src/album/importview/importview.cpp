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
#include <dgiovolumemanager.h>
#include <dgiofile.h>
#include <dgiofileinfo.h>
#include <dgiovolume.h>
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "utils/unionimage.h"
#include "imageengine/imageengineapi.h"
#include "ac-desktop-define.h"

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
    m_pImportBtn->setText(tr("Import Photos"));
    m_pImportBtn->setFixedSize(302, 36);
    m_pImportBtn->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));
    DLabel *pLabel2 = new DLabel();
    DFontSizeManager::instance()->bind(pLabel2, DFontSizeManager::T9, QFont::ExtraLight);
    pLabel2->setForegroundRole(DPalette::TextTips);
    pLabel2->setFixedHeight(18);
    pLabel2->setText(tr("Or drag photos here"));
    pImportFrameLayout->setMargin(0);
    pImportFrameLayout->addStretch();
    pImportFrameLayout->addWidget(pLabel, 0, Qt::AlignCenter);
    pImportFrameLayout->addSpacing(5);
    pImportFrameLayout->addWidget(m_pImportBtn, 0, Qt::AlignCenter);
    pImportFrameLayout->addSpacing(10);
    pImportFrameLayout->addWidget(pLabel2, 0, Qt::AlignCenter);
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
    const QMimeData *mimeData = e->mimeData();
    if (!utils::base::checkMimeData(mimeData)) {
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

void ImportView::dragLeaveEvent(QDragLeaveEvent *e)
{
    Q_UNUSED(e);
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
}

void ImportView::onImprotBtnClicked(bool useDialog, const QStringList &list)
{
    qDebug() << "ImportView::onImprotBtnClicked()";
    static QStringList sList;
    for (const QString &i : UnionImage_NameSpace::unionImageSupportFormat())
        sList << ("*." + i);
    QString filter = tr("All Photos");
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
    dialog.setWindowTitle(tr("Import Photos"));
    dialog.setAllowMixedSelection(true);

    if (useDialog) {
        if (dialog.exec() != QDialog::Accepted) {
            qDebug() << "mode != QDialog::Accepted";
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
    ImageEngineApi::instance()->SaveImagesCache(file_list);
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



