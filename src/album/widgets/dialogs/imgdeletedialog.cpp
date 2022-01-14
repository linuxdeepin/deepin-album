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
#include "imgdeletedialog.h"

#include <QVBoxLayout>

#include <DLabel>
#include <DPushButton>
#include <DFontSizeManager>
#include <DApplicationHelper>
#include <DSuggestButton>
#include <DTitlebar>

ImgDeleteDialog::ImgDeleteDialog(DWidget *parent, int imgCount, int videoCount, bool isTrash)
    : DDialog(parent)
{
    setModal(true);
    setContentsMargins(0, 0, 0, 0);
    QIcon icon = QIcon::fromTheme("deepin-album");
    this->setIcon(icon);
    DLabel *m_Toplabel = new DLabel(this);
    DLabel *m_Bottomlabel = new DLabel(this);

    //新版删除文案
    if (isTrash) { //从最近删除点进来的
        if (imgCount + videoCount == 1) {
            m_Toplabel->setText(tr("Are you sure you want to permanently delete this file?"));
            m_Bottomlabel->setText(tr("You cannot restore it any longer"));
        } else {
            m_Toplabel->setText(tr("Are you sure you want to permanently delete %1 files?").arg(imgCount + videoCount));
            m_Bottomlabel->setText(tr("You cannot restore them any longer"));
        }
    } else { //从其它地方点进来的
        if (imgCount + videoCount == 1) {
            m_Toplabel->setText(tr("Are you sure you want to delete this file locally?"));
            m_Bottomlabel->setText(tr("You can restore it in the trash"));
        } else {
            m_Toplabel->setText(tr("Are you sure you want to delete %1 files locally?").arg(imgCount + videoCount));
            m_Bottomlabel->setText(tr("You can restore them in the trash"));
        }
    }

    //字体风格调整
    m_Toplabel->setWordWrap(true);
    m_Toplabel->setAlignment(Qt::AlignHCenter);
    DFontSizeManager::instance()->bind(m_Toplabel, DFontSizeManager::T6, QFont::Medium);
    m_Bottomlabel->setWordWrap(true);
    m_Bottomlabel->setAlignment(Qt::AlignHCenter);
    m_Bottomlabel->setForegroundRole(DPalette::TextTips);
    DFontSizeManager::instance()->bind(m_Bottomlabel, DFontSizeManager::T7, QFont::Medium);

    DWidget *contentWidget = new DWidget(this);
    contentWidget->setFixedHeight(this->height() - 10);
    contentWidget->setContentsMargins(0, 0, 0, 0);
    QVBoxLayout *layout = new QVBoxLayout(contentWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addStretch();
    layout->addWidget(m_Toplabel);
    layout->addWidget(m_Bottomlabel);
    layout->addStretch();
    addContent(contentWidget);
    insertButton(0, tr("Cancel"), false, DDialog::ButtonNormal);
    insertButton(1, tr("Delete"), true, DDialog::ButtonWarning);

    getButton(0)->setFocusPolicy(Qt::TabFocus);
    getButton(1)->setFocusPolicy(Qt::TabFocus);
    getButton(0)->installEventFilter(this);
    getButton(1)->installEventFilter(this);

    QWidget *closeButton =  this->findChild<QWidget *>("DTitlebarDWindowCloseButton");
    closeButton->setFocusPolicy(Qt::TabFocus);
    closeButton->installEventFilter(this);

    //剔除Titlebar焦点
    DTitlebar *titlebar = findChild<DTitlebar *>();
    if (titlebar) {
        titlebar->setFocusPolicy(Qt::ClickFocus);
    }

    m_allTabOrder.clear();
    m_allTabOrder.insert(0, closeButton);
    m_allTabOrder.insert(1, getButton(0));
    m_allTabOrder.insert(2, getButton(1));
}

bool ImgDeleteDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Tab) {
            if (getButton(0) == obj) {
                getButton(1)->setFocus();
                return true;
            } else if (getButton(1) == obj) {
                m_allTabOrder.at(0)->setFocus();
                return true;
            } else if (obj->objectName() == "DTitlebarDWindowCloseButton") {
                getButton(0)->setFocus();
                return true;
            } else {
                m_allTabOrder.at(0)->setFocus();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}
