// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "signalmanager.h"
#include "imginfodialog.h"
#include "videoinfodialog.h"
#include "application.h"
#include "mainwindow.h"
#include "imageengineapi.h"

std::atomic_bool SignalManager::inAutoImport;
SignalManager *SignalManager::m_signalManager = nullptr;
SignalManager *SignalManager::instance()
{
    if (m_signalManager == nullptr) {
        m_signalManager = new SignalManager;
    }

    inAutoImport = false;

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
