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
#include "mainwidget.h"
#include "application.h"
#include "controller/configsetter.h"
//#include "module/slideshow/slideshowpanel.h"
//#include "module/view/viewpanel.h"
#include "utils/baseutils.h"
#include "widgets/dialogs/imginfodialog.h"
#include "ac-desktop-define.h"

#include <QFileSystemWatcher>
#include <QLabel>
#include <QDebug>
#include <QDesktopWidget>
#include <QFile>
#include <QHBoxLayout>
#include <QDebug>
#include "controller/signalmanager.h"
#include <ddialog.h>
using namespace Dtk::Widget;

namespace {

const int TOP_TOOLBAR_HEIGHT = 50;
const int BOTTOM_TOOLBAR_HEIGHT = 70 + 10;
//const int EXTENSION_PANEL_WIDTH = 300;
const int BOTTOM_TOOLBAR_WIDTH_1 = 532 + 10 + 2;
const int BOTTOM_TOOLBAR_WIDTH_2 = 782 + 10 + 2;
const int THUMBNAIL_ADD_WIDTH = 32;
const int BOTTOM_SPACING = 10;
const int RT_SPACING = 10;
const int BOTTOM_REPAIR_SPACING = 5;
const int TOOLBAR_MINIMUN_WIDTH = 630 - 20 + 10 + 2;
const int BOTTOM_ADJUST = 0;

const QString SETTINGS_GROUP = "MAINWIDGET";
const QString SETTINGS_MAINPANEL_KEY = "MainPanel";

}  // namespace

MainWidget::MainWidget(bool manager, QWidget *parent)
    : QFrame(parent)
{
//    initStyleSheet();
#ifndef LITE_DIV
    initPanelStack(manager);
#else
    Q_UNUSED(manager)
    initPanelStack(false);
#endif
    initExtensionPanel();
    initTopToolbar();
    initBottomToolbar();

    initConnection();
    setMouseTracking(true);
    setObjectName("MainWidget");
}

MainWidget::~MainWidget()
{

}

void MainWidget::resizeEvent(QResizeEvent *e)
{
    if (m_topToolbar) {
        m_topToolbar->resize(width(), TOP_TOOLBAR_HEIGHT);

        emit dApp->signalM->resizeFileName();
        if (e->oldSize()  != e->size()) {
            emit m_topToolbar->updateMaxBtn();
        }

        if (window()->isFullScreen()) {
            m_topToolbar->setVisible(false);
        } else {
            m_topToolbar->setVisible(true);
        }
    }

    if (m_bottomToolbar) {
        if (m_viewPanel->getPicCount() <= 1) {
            m_bottomToolbar->setFixedWidth(BOTTOM_TOOLBAR_WIDTH_1);
        } else if (m_viewPanel->getPicCount() <= 3) {
            m_bottomToolbar->setFixedWidth(BOTTOM_TOOLBAR_WIDTH_2);
        } else {
            m_bottomToolbar->setFixedWidth(qMin((BOTTOM_TOOLBAR_WIDTH_2 + THUMBNAIL_ADD_WIDTH * (m_viewPanel->getPicCount() - 3)) + BOTTOM_ADJUST, qMax(this->width() - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)));
        }
        m_bottomToolbar->move((this->width() - m_bottomToolbar->width()) / 2, this->height() - m_bottomToolbar->height() - BOTTOM_SPACING + BOTTOM_REPAIR_SPACING);
        if (window()->isFullScreen()) {
            emit dApp->signalM->sigShowFullScreen();
        }
    }
    if (m_btmSeparatorLine) {
        m_btmSeparatorLine->resize(window()->width(), 1);
        m_btmSeparatorLine->move(0, window()->height() -
                                 m_bottomToolbar->height() - 1);
    }
}

void MainWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    if (m_bottomToolbar) {
        if (window()->isFullScreen()) {
            emit dApp->signalM->sigShowFullScreen();
        }
    }
}

void MainWidget::onGotoPanel(ModulePanel *panel)
{
    QPointer<ModulePanel> p(panel);
    if (p.isNull()) {
        return;
    }
    // Record the last panel for restore in the next time launch
    if (p->isMainPanel() && ! p->moduleName().isEmpty()) {
        dApp->setter->setValue(SETTINGS_GROUP, SETTINGS_MAINPANEL_KEY,
                               QVariant(p->moduleName()));
    }
    m_panelStack->setCurrentWidget(panel);
}

void MainWidget::onShowImageInfo(const QString &path)
{
    if (m_infoShowingList.indexOf(path) != -1)
        return;
    else
        m_infoShowingList << path;

#ifndef LITE_DIV
    ImgInfoDialog *info = new ImgInfoDialog(path);
    info->move((width() - info->width()) / 2 +
               mapToGlobal(QPoint(0, 0)).x(),
               (window()->height() - info->sizeHint().height()) / 2 +
               mapToGlobal(QPoint(0, 0)).y());
    info->show();
    info->setWindowState(Qt::WindowActive);
    connect(info, &ImgInfoDialog::closed, this, [ = ] {
        info->deleteLater();
        m_infoShowingList.removeAll(path);
    });
#endif
}

void MainWidget::onBackToMainPanel()
{
    window()->show();
    window()->raise();
    window()->activateWindow();
    QString name = dApp->setter->value(SETTINGS_GROUP,
                                       SETTINGS_MAINPANEL_KEY).toString();
    if (name.isEmpty()) {
        emit dApp->signalM->gotoTimelinePanel();
        return;
    }

    for (int i = 0; i < m_panelStack->count(); i++) {
        if (ModulePanel *p =
                    static_cast<ModulePanel *>(m_panelStack->widget(i))) {
            if ((p->moduleName() == name) && p->isMainPanel()) {
                emit dApp->signalM->gotoPanel(p);
                return;
            }
        }
    }
}

void MainWidget::onActiveWindow()
{
    window()->raise();
    window()->activateWindow();
}

void MainWidget::onShowInFileManager(const QString &path)
{
    utils::base::showInFileManager(path);
}

void MainWidget::onMouseMove(bool show)
{
#ifdef tablet_PC
    if (!show) {
        QPropertyAnimation *animation = new QPropertyAnimation(m_bottomToolbar, "pos", this);
        animation->setDuration(200);
        animation->setEasingCurve(QEasingCurve::NCurveTypes);
        animation->setStartValue(QPoint((width() - m_bottomToolbar->width()) / 2, m_bottomToolbar->y()));
        animation->setEndValue(QPoint((width() - m_bottomToolbar->width()) / 2, height()));
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
        QPropertyAnimation *animation = new QPropertyAnimation(m_bottomToolbar, "pos", this);
        animation->setDuration(200);
        animation->setEasingCurve(QEasingCurve::NCurveTypes);
        animation->setStartValue(QPoint((width() - m_bottomToolbar->width()) / 2, m_bottomToolbar->y()));
        animation->setEndValue(QPoint((width() - m_bottomToolbar->width()) / 2, height() - m_bottomToolbar->height() - 10));
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }
#else
    Q_UNUSED(show)
    if (window()->isFullScreen()) {
        QPoint pos = mapFromGlobal(QCursor::pos());
        if (height() - 90 < pos.y() && height() > pos.y() && height() >= m_bottomToolbar->y()) {
            QPropertyAnimation *animation = new QPropertyAnimation(m_bottomToolbar, "pos", this);
            animation->setDuration(200);
            animation->setEasingCurve(QEasingCurve::NCurveTypes);
            animation->setStartValue(QPoint((width() - m_bottomToolbar->width()) / 2, m_bottomToolbar->y()));
            animation->setEndValue(QPoint((width() - m_bottomToolbar->width()) / 2, height() - m_bottomToolbar->height() - 10));
            animation->start(QAbstractAnimation::DeleteWhenStopped);
        } else if (height() - m_bottomToolbar->height() - 10 > pos.y()
                   && height() - m_bottomToolbar->height() - 10 <= m_bottomToolbar->y()) {
            //隐藏状态下，区域外的移动事件不响应
            if (m_bottomToolbar->y() >= height()) {
                return;
            }
            QPropertyAnimation *animation = new QPropertyAnimation(m_bottomToolbar, "pos", this);
            animation->setDuration(200);
            animation->setEasingCurve(QEasingCurve::NCurveTypes);
            animation->setStartValue(QPoint((width() - m_bottomToolbar->width()) / 2, m_bottomToolbar->y()));
            animation->setEndValue(QPoint((width() - m_bottomToolbar->width()) / 2, height()));
            animation->start(QAbstractAnimation::DeleteWhenStopped);
        }
    }
#endif
}

void MainWidget::onShowFullScreen()
{
    m_bottomToolbar->move((width() - m_bottomToolbar->width()) / 2, height());
}

void MainWidget::onUpdateBottomToolbar(bool wideMode)
{
    if (wideMode) {
        m_bottomToolbar->setFixedHeight(BOTTOM_TOOLBAR_HEIGHT);
        if (m_viewPanel->getPicCount() <= 3) {
            m_bottomToolbar->setFixedWidth(BOTTOM_TOOLBAR_WIDTH_2);
        } else {
            m_bottomToolbar->setFixedWidth(qMin(BOTTOM_TOOLBAR_WIDTH_2 + THUMBNAIL_ADD_WIDTH * (m_viewPanel->getPicCount() - 3) + BOTTOM_ADJUST, qMax(this->width() - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)));
        }
        m_bottomToolbar->setVisible(true);
        m_btmSeparatorLine->setVisible(m_bottomToolbar->isVisible());
    } else {
        m_bottomToolbar->setFixedHeight(BOTTOM_TOOLBAR_HEIGHT);
        m_bottomToolbar->setFixedWidth(BOTTOM_TOOLBAR_WIDTH_1);
        if (m_viewPanel->getPicCount() == 1) {
            m_bottomToolbar->setVisible(true);
            m_btmSeparatorLine->setVisible(m_bottomToolbar->isVisible());
        } else {
            m_bottomToolbar->setVisible(false);
            m_btmSeparatorLine->setVisible(m_bottomToolbar->isVisible());
        }
    }
    m_bottomToolbar->move((this->width() - m_bottomToolbar->width()) / 2, this->height() - BOTTOM_TOOLBAR_HEIGHT - BOTTOM_SPACING + BOTTOM_REPAIR_SPACING);
}

void MainWidget::onUpdateBottomToolbarContent(QWidget *c, bool wideMode)
{
    if (c == nullptr)
        return;
    m_bottomToolbar->setContent(c);
    if (wideMode) {
        m_bottomToolbar->setFixedHeight(BOTTOM_TOOLBAR_HEIGHT);
        if (m_viewPanel->getPicCount() <= 3) {
            m_bottomToolbar->setFixedWidth(BOTTOM_TOOLBAR_WIDTH_2);
        } else {
            m_bottomToolbar->setFixedWidth(qMin(BOTTOM_TOOLBAR_WIDTH_2 + THUMBNAIL_ADD_WIDTH * (m_viewPanel->getPicCount() - 3) + BOTTOM_ADJUST, qMax(this->width() - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)));
        }
        m_bottomToolbar->setVisible(true);
        m_btmSeparatorLine->setVisible(m_bottomToolbar->isVisible());
    } else {
        m_bottomToolbar->setFixedHeight(BOTTOM_TOOLBAR_HEIGHT);
        m_bottomToolbar->setFixedWidth(BOTTOM_TOOLBAR_WIDTH_1);
        if (m_viewPanel->getPicCount() == 1) {
            m_bottomToolbar->setVisible(true);
            m_btmSeparatorLine->setVisible(m_bottomToolbar->isVisible());
        } else {
            m_bottomToolbar->setVisible(false);
            m_btmSeparatorLine->setVisible(m_bottomToolbar->isVisible());
        }
    }
    m_bottomToolbar->move((this->width() - m_bottomToolbar->width()) / 2,
                          this->height() - BOTTOM_TOOLBAR_HEIGHT - BOTTOM_SPACING + BOTTOM_REPAIR_SPACING);
}

void MainWidget::onShowBottomToolbar()
{
    // 显示时先停止动画
    QList<QPropertyAnimation *> lis = this->findChildren<QPropertyAnimation *>();
    for (auto animation : lis) {
        animation->stop();
    }
    m_bottomToolbar->setVisible(true);
    m_btmSeparatorLine->setVisible(m_bottomToolbar->isVisible());
}

void MainWidget::onHideBottomToolbar(bool immediately)
{
    m_bottomToolbar->move((width() - m_bottomToolbar->width()) / 2, height());
    m_btmSeparatorLine->setVisible(m_bottomToolbar->isVisible());
    Q_UNUSED(immediately)
}

void MainWidget::onUpdateExtensionPanelContent(QWidget *c)
{
    if (c != nullptr)
        m_extensionPanel->setContent(c);
}

void MainWidget::onShowExtensionPanel()
{
    m_extensionPanel->move(window()->x() + (window()->width() - m_extensionPanel->width()) / 2,
                           window()->y() + (window()->height() - m_extensionPanel->height()) / 2);
    m_extensionPanel->show();
}

void MainWidget::initPanelStack(bool manager)
{
#ifndef LITE_DIV
    m_manager = manager;
#else
    Q_UNUSED(manager)
#endif
    m_panelStack = new QStackedWidget(this);
    m_panelStack->setObjectName("PanelStack");

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_panelStack);
    m_viewPanel = new ViewPanel();
    m_panelStack->addWidget(m_viewPanel);
}

void MainWidget::initTopToolbar()
{
    m_topToolbar = new TopToolbar(false, this);
    m_topToolbar->resize(width(), TOP_TOOLBAR_HEIGHT);
    m_topToolbar->move(0, 0);
}

void MainWidget::initConnection()
{
    connect(dApp->signalM, &SignalManager::backToMainPanel, this, &MainWidget::onBackToMainPanel);
    connect(dApp->signalM, &SignalManager::activeWindow, this, &MainWidget::onActiveWindow);
    connect(dApp->signalM, &SignalManager::gotoPanel, this, &MainWidget::onGotoPanel);
    connect(dApp->signalM, &SignalManager::showInFileManager, this, &MainWidget::onShowInFileManager);
    connect(dApp->signalM, &SignalManager::showImageInfo, this, &MainWidget::onShowImageInfo);
    connect(dApp->signalM, &SignalManager::sigMouseMove, this, &MainWidget::onMouseMove);
    connect(dApp->signalM, &SignalManager::sigShowFullScreen, this, &MainWidget::onShowFullScreen);
}

void MainWidget::initBottomToolbar()
{
    m_bottomToolbar = new BottomToolbar(this);
    m_bottomToolbar->resize(532, BOTTOM_TOOLBAR_HEIGHT);
    m_bottomToolbar->move((width() - m_bottomToolbar->width()) / 2, height() - m_bottomToolbar->height() - 10);
    m_btmSeparatorLine = new QLabel(this);
    connect(dApp->signalM, &SignalManager::updateBottomToolbar, this, &MainWidget::onUpdateBottomToolbar);
    connect(dApp->signalM, &SignalManager::updateBottomToolbarContent, this, &MainWidget::onUpdateBottomToolbarContent);
    connect(dApp->signalM, &SignalManager::showBottomToolbar, this, &MainWidget::onShowBottomToolbar);
    connect(dApp->signalM, &SignalManager::hideBottomToolbar, this, &MainWidget::onHideBottomToolbar);
}

void MainWidget::initExtensionPanel()
{
    m_extensionPanel = ExtensionPanel::getInstance(this);
    m_extensionPanel->close();
    connect(dApp->signalM, &SignalManager::updateExtensionPanelContent, this, &MainWidget::onUpdateExtensionPanelContent);
    connect(dApp->signalM, &SignalManager::showExtensionPanel, this, &MainWidget::onShowExtensionPanel);
#if 0
    connect(dApp->signalM, &SignalManager::hideExtensionPanel,
    this, [ = ](bool immediately) {
        if (immediately) {
            m_extensionPanel->requestStopAnimation();
            if (this->window()->isFullScreen()) {
//                m_extensionPanel->move(- qMax(m_extensionPanel->width(),
//                                              EXTENSION_PANEL_WIDTH), 0);
                m_extensionPanel->move(width(), 0);
            } else {
//                m_extensionPanel->move(- qMax(m_extensionPanel->width(),
//                                    EXTENSION_PANEL_WIDTH), TOP_TOOLBAR_HEIGHT);
                m_extensionPanel->move(width(), TOP_TOOLBAR_HEIGHT);
            }
        } else {
            if (this->window()->isFullScreen()) {
//                m_extensionPanel->moveWithAnimation(- qMax(m_extensionPanel->width(),
//                                                      EXTENSION_PANEL_WIDTH), 0);
                m_extensionPanel->moveWithAnimation(width(),  0);
            } else {
//                m_extensionPanel->moveWithAnimation(- qMax(m_extensionPanel->width(),
//                                          EXTENSION_PANEL_WIDTH), TOP_TOOLBAR_HEIGHT);
                m_extensionPanel->moveWithAnimation(width(), TOP_TOOLBAR_HEIGHT);
            }
        }
    });
#endif
}
