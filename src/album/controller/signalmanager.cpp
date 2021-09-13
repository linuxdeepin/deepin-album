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
#include "signalmanager.h"
#include "imginfodialog.h"
#include "videoinfodialog.h"
#include "application.h"
#include "mainwindow.h"

SignalManager *SignalManager::m_signalManager = nullptr;
SignalManager *SignalManager::instance()
{
    if (m_signalManager == nullptr) {
        m_signalManager = new SignalManager;
    }

    return m_signalManager;
}

void SignalManager::emitSliderValueChg(int value)
{
    m_sliderValue = value;
    emit sigMainwindowSliderValueChg(value);
}

int SignalManager::getSliderValue()
{
    return m_sliderValue;
}

void SignalManager::showInfoDlg(const QString &path)
{
    DBImgInfo data;
    ImageEngineApi::instance()->getImageData(path, data);
    if (data.itemType == ItemTypePic) {
        ImgInfoDialog *dialog = new ImgInfoDialog(path, dApp->getMainWindow());
        dialog->setObjectName("ImgInfoDialog");
        dialog->show();
        dialog->move((dApp->getMainWindow()->width() - dialog->width() - 50 + dApp->getMainWindow()->mapToGlobal(QPoint(0, 0)).x())
                     , 100 + dApp->getMainWindow()->mapToGlobal(QPoint(0, 0)).y());
        dialog->setWindowState(Qt::WindowActive);
        connect(dialog, &ImgInfoDialog::closed, this, [ = ] {
            dialog->deleteLater();
        });

        connect(dialog, &ImgInfoDialog::visibleChanged, this, [ = ](bool visible) {
            if (!visible) {
                dialog->deleteLater();
            }
        });
    } else if (data.itemType == ItemTypeVideo) {
        VideoInfoDialog *dialog = new VideoInfoDialog(path, dApp->getMainWindow());
        dialog->setObjectName("VideoInfoDialog");
        dialog->show();
        dialog->setWindowState(Qt::WindowActive);
        connect(dialog, &VideoInfoDialog::closed, this, [ = ] {
            dialog->deleteLater();
        });

        connect(dialog, &VideoInfoDialog::visibleChanged, this, [ = ](bool visible) {
            if (!visible) {
                dialog->deleteLater();
            }
        });
    }
}

SignalManager::SignalManager(QObject *parent) : QObject(parent)
{

}
