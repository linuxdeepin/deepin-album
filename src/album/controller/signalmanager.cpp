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
#include "imageengineapi.h"

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

void SignalManager::showInfoDlg(const QString &path, ItemType type, bool isTrash)
{
    if (type == ItemTypeNull) { //类型未知的时候才去抓它的类型
        DBImgInfo data;
        ImageEngineApi::instance()->getImageData(path, data);
        type = data.itemType;
    }

    QString realPath;
    QString displayName(DBImgInfo::getFileNameFromFilePath(path)); //这里传入的path是原始路径，所以可以直接用
    if (isTrash) { //获取最近删除图片的真实路径
        realPath = utils::base::getDeleteFullPath(utils::base::hashByString(path), displayName);
        if (!QFile::exists(realPath)) {
            realPath = path;
        }
    } else {
        realPath = path;
    }

    if (type == ItemTypePic) {
        ImgInfoDialog *dialog = new ImgInfoDialog(realPath, displayName, dApp->getMainWindow());
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
    } else if (type == ItemTypeVideo) {
        VideoInfoDialog *dialog = new VideoInfoDialog(realPath, displayName, isTrash, dApp->getMainWindow());
        dialog->setObjectName("VideoInfoDialog");
        dialog->show();
        //概率性获取位置错误，需要再次移动
        dialog->move((dApp->getMainWindow()->width() - dialog->width() - 50 + dApp->getMainWindow()->mapToGlobal(QPoint(0, 0)).x())
                     , 100 + dApp->getMainWindow()->mapToGlobal(QPoint(0, 0)).y());
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
    , m_sliderValue(0)
{

}
