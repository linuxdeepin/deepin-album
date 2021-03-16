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

ImgDeleteDialog::ImgDeleteDialog(DWidget *parent, int count, bool bdeleteallonlyone)
    : DDialog(parent)
{
    setModal(true);
    setContentsMargins(0, 0, 0, 0);
    QIcon icon = QIcon::fromTheme("deepin-album");
    this->setIcon(icon);
    DLabel *m_label = new DLabel(this);
    if (1 == count && !bdeleteallonlyone) {
        m_label->setText(tr("Are you sure you want to delete this photo from the album?"));
    } else {
        QString str = tr("Are you sure you want to delete %1 photos from albums?");
        m_label->setText(str.arg(count));
    }
    m_label->setWordWrap(true);
    m_label->setAlignment(Qt::AlignHCenter);
    DWidget *contentWidget = new DWidget(this);
    contentWidget->setFixedHeight(this->height() - 80);
    contentWidget->setContentsMargins(0, 0, 0, 0);
    QVBoxLayout *layout = new QVBoxLayout(contentWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addStretch();
    layout->addWidget(m_label);
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
