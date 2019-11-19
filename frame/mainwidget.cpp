/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
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
#include "controller/importer.h"
#include "controller/configsetter.h"
//#include "module/album/albumpanel.h"
//#include "module/edit/EditPanel.h"
//#include "module/timeline/timelinepanel.h"
#include "module/slideshow/slideshowpanel.h"
//#include "module/view/viewpanel.h"
#include "utils/baseutils.h"
#include "widgets/separator.h"
#include "widgets/processtooltip.h"
#include "widgets/dialogs/imginfodialog.h"

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
const int BOTTOM_TOOLBAR_HEIGHT = 70+10;
const int EXTENSION_PANEL_WIDTH = 300;
const int BOTTOM_TOOLBAR_WIDTH_1 = 482+10;
const int BOTTOM_TOOLBAR_WIDTH_2 = 782+10;
const int THUMBNAIL_ADD_WIDTH = 31;
const int BOTTOM_SPACING = 10;
const int RT_SPACING = 10;

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
}

MainWidget::~MainWidget()
{

}

void MainWidget::resizeEvent(QResizeEvent *e)
{
    if (m_topToolbar)
    {
        m_topToolbar->resize(width(), TOP_TOOLBAR_HEIGHT);

        emit dApp->signalM->resizeFileName();
        if (e->oldSize()  != e->size()) {
            emit m_topToolbar->updateMaxBtn();
        }

        if(window()->isFullScreen())
        {
            m_topToolbar->setVisible(false);
        }
        else
        {
            m_topToolbar->setVisible(true);
        }
    }

    if (m_bottomToolbar)
    {
        if ( m_viewPanel->getPicCount() <= 1 )
        {
            m_bottomToolbar->setFixedWidth(482);
        }
        else if(m_viewPanel->getPicCount() <= 3 )
        {
            m_bottomToolbar->setFixedWidth(782);
        }
        else
        {
            m_bottomToolbar->setFixedWidth(qMin((782+31*(m_viewPanel->getPicCount()-3)),qMax(width()-20,1280)));
        }

        m_bottomToolbar->move((width()-m_bottomToolbar->width())/2, height() - m_bottomToolbar->height()-10);

        if(window()->isFullScreen())
        {
            emit dApp->signalM->sigShowFullScreen();
        }
    }

    if (m_btmSeparatorLine) {
        m_btmSeparatorLine->resize(window()->width(), 1);
        m_btmSeparatorLine->move(0, window()->height() -
                               m_bottomToolbar->height() - 1);
    }
    if(m_extensionPanel){
        if (this->window()->isFullScreen()) {
            if(m_extensionPanel->x()<e->oldSize().width()){
                m_extensionPanel->move((width()-EXTENSION_PANEL_WIDTH-24), 0);
            }else {
                m_extensionPanel->move(width(), 0);
            }
        }
        else{
            if(m_extensionPanel->x() < e->oldSize().width()){
                m_extensionPanel->move((width()-EXTENSION_PANEL_WIDTH-24), TOP_TOOLBAR_HEIGHT);
            }else {
                m_extensionPanel->move(width(),TOP_TOOLBAR_HEIGHT);
            }
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

void MainWidget::onImported(const QString &message, bool success)
{
    ProcessTooltip *t = new ProcessTooltip(this);
    t->showTooltip(message, success);
    t->move((width() - t->width()) / 2, height() * 4 / 5);
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
    connect(info, &ImgInfoDialog::closed, this, [=] {
        info->deleteLater();
        m_infoShowingList.removeAll(path);
    });
#endif
//    connect(dApp->signalM, &SignalManager::gotoPanel,
//            info, &ImgInfoDialog::close);
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

    // Init panel

    m_viewPanel = new ViewPanel();
    m_panelStack->addWidget(m_viewPanel);
    SlideShowPanel *m_slidePanel = new SlideShowPanel();
    m_panelStack->addWidget(m_slidePanel);
}

void MainWidget::initTopToolbar()
{
    m_topToolbar = new TopToolbar(false, this);
    m_topToolbar->resize(width(), TOP_TOOLBAR_HEIGHT);
    m_topToolbar->move(0, 0);
}

void MainWidget::initConnection()
{
    connect(dApp->signalM, &SignalManager::backToMainPanel, this, [=] {
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
    });
    connect(dApp->signalM, &SignalManager::activeWindow, this, [=]{
        window()->raise();
        window()->activateWindow();
    });

    connect(dApp->signalM, &SignalManager::gotoPanel,
            this, &MainWidget::onGotoPanel);
    connect(dApp->signalM, &SignalManager::showInFileManager,
            this, [=] (const QString &path) {
        utils::base::showInFileManager(path);
    });
    connect(dApp->signalM, &SignalManager::showImageInfo,
            this, &MainWidget::onShowImageInfo);
//    connect(dApp->importer, &Importer::imported, this, [=] (bool v) {
//        onImported(tr("Imported successfully"), v);
//    });
//    connect(dApp->viewerTheme, &ViewerThemeManager::viewerThemeChanged, this, &MainWidget::initStyleSheet);
    connect(dApp->signalM, &SignalManager::sigMouseMove, this, [=] {
        if (window()->isFullScreen())
        {
            QPoint pos = mapFromGlobal(QCursor::pos());
            if (height() - 20 < pos.y()
                && height() > pos.y()
                && height() == m_bottomToolbar->y())
            {
                QPropertyAnimation *animation = new QPropertyAnimation(m_bottomToolbar, "pos");
                animation->setDuration(200);
                animation->setEasingCurve(QEasingCurve::NCurveTypes);
                animation->setStartValue(QPoint((width()-m_bottomToolbar->width())/2, m_bottomToolbar->y()));
                animation->setEndValue(QPoint((width()-m_bottomToolbar->width())/2, height() - m_bottomToolbar->height()-10));
                animation->start(QAbstractAnimation::DeleteWhenStopped);
            }
            else if (height() - m_bottomToolbar->height()- 10 > pos.y()
                     && height() - m_bottomToolbar->height()- 10 == m_bottomToolbar->y())
            {
                QPropertyAnimation *animation = new QPropertyAnimation(m_bottomToolbar, "pos");
                animation->setDuration(200);
                animation->setEasingCurve(QEasingCurve::NCurveTypes);
                animation->setStartValue(QPoint((width()-m_bottomToolbar->width())/2, m_bottomToolbar->y()));
                animation->setEndValue(QPoint((width()-m_bottomToolbar->width())/2, height()));
                animation->start(QAbstractAnimation::DeleteWhenStopped);
            }
        }
    });
    connect(dApp->signalM, &SignalManager::sigShowFullScreen, this, [=] {
        m_bottomToolbar->move((width()-m_bottomToolbar->width())/2, height());
    });
}

void MainWidget::initBottomToolbar()
{
    m_bottomToolbar = new BottomToolbar(this);

    m_bottomToolbar->resize(482, BOTTOM_TOOLBAR_HEIGHT);
    m_bottomToolbar->move((width()-m_bottomToolbar->width())/2, height() - m_bottomToolbar->height()-10);

    m_btmSeparatorLine = new QLabel(this);

    connect(dApp->signalM,&SignalManager::updateBottomToolbar,this,[=](bool wideMode){
        if (wideMode) {
            m_bottomToolbar->setFixedHeight(BOTTOM_TOOLBAR_HEIGHT);

            if(m_viewPanel->getPicCount()<= 3)
            {
                m_bottomToolbar->setFixedWidth(782);
            }
            else
            {
                m_bottomToolbar->setFixedWidth(qMin(782+31*(m_viewPanel->getPicCount()-3),qMax(width()-20,1280)));
            }

            m_bottomToolbar->setVisible(true);
            m_btmSeparatorLine->setVisible(m_bottomToolbar->isVisible());
        }
        else {
            m_bottomToolbar->setFixedHeight(BOTTOM_TOOLBAR_HEIGHT);
            m_bottomToolbar->setFixedWidth(482);
            if(m_viewPanel->getPicCount() == 1){
                m_bottomToolbar->setVisible(true);
                m_btmSeparatorLine->setVisible(m_bottomToolbar->isVisible());
            }else {
                m_bottomToolbar->setVisible(false);
                m_btmSeparatorLine->setVisible(m_bottomToolbar->isVisible());
            }
        }

        m_bottomToolbar->move((width()-m_bottomToolbar->width())/2, height() - m_bottomToolbar->height()-10);
    });

    connect(dApp->signalM, &SignalManager::updateBottomToolbarContent,
            this, [=](QWidget *c, bool wideMode) {
        if (c == nullptr)
            return;
        m_bottomToolbar->setContent(c);
        if (wideMode) {
            m_bottomToolbar->setFixedHeight(BOTTOM_TOOLBAR_HEIGHT);
//            m_bottomToolbar->setFixedWidth(1280);
            if(m_viewPanel->getPicCount()<= 3){
                m_bottomToolbar->setFixedWidth(782);
            }else {
                m_bottomToolbar->setFixedWidth(qMin(782+31*(m_viewPanel->getPicCount()-3),qMax(width()-20,1280)));
            }
            m_bottomToolbar->setVisible(true);
            m_btmSeparatorLine->setVisible(m_bottomToolbar->isVisible());
        }
        else {
            m_bottomToolbar->setFixedHeight(BOTTOM_TOOLBAR_HEIGHT);
            m_bottomToolbar->setFixedWidth(482);
            if(m_viewPanel->getPicCount() == 1){
                m_bottomToolbar->setVisible(true);
                m_btmSeparatorLine->setVisible(m_bottomToolbar->isVisible());
            }else {
                m_bottomToolbar->setVisible(false);
                m_btmSeparatorLine->setVisible(m_bottomToolbar->isVisible());
            }
        }

        if(window()->isFullScreen() || window()->isMaximized()){
//            m_bottomToolbar->setFixedWidth(window()->width()-20);
        }
//        m_bottomToolbar->setRadius(18);
        m_bottomToolbar->move((width()-m_bottomToolbar->width())/2, height() - m_bottomToolbar->height()-10);
    });
    connect(dApp->signalM, &SignalManager::showBottomToolbar, this, [=] {
        m_bottomToolbar->setVisible(true);
        m_btmSeparatorLine->setVisible(m_bottomToolbar->isVisible());
//        // Make the bottom toolbar always stay at the bottom after windows resize
//        m_bottomToolbar->move(0, height());
//        m_bottomToolbar->moveWithAnimation(0, height() - m_bottomToolbar->height());
    });

    connect(dApp->signalM, &SignalManager::hideBottomToolbar,
            this, [=](bool immediately) {
        m_bottomToolbar->move((width()-m_bottomToolbar->width())/2, height());
        m_bottomToolbar->setVisible(false);
        m_btmSeparatorLine->setVisible(m_bottomToolbar->isVisible());
        Q_UNUSED(immediately)
//        if (immediately) {
//            m_bottomToolbar->move(0, height());
//            m_bottomToolbar->setVisible(false);
//        }
//        else {
//            m_bottomToolbar->moveWithAnimation(0, height());
//        }
    });
}

void MainWidget::initExtensionPanel()
{
    m_extensionPanel = new ExtensionPanel(this);
    m_extensionPanel->move(width(), 0);
    m_extensionPanel->hide();
    connect(dApp->signalM, &SignalManager::updateExtensionPanelContent,
            this, [=](QWidget *c) {
        if (c != nullptr)
            m_extensionPanel->setContent(c);
    });
    connect(dApp->signalM, &SignalManager::showExtensionPanel, this, [=] {
        // Is visible
        if (m_extensionPanel->x() == (width()-EXTENSION_PANEL_WIDTH-24)) {
            return;
        }
#ifdef LITE_DIV
        m_extensionPanel->resize(m_extensionPanel->width(), height());
#endif
        m_extensionPanel->show();
        //m_extensionPanel's height is dependent on the visible of topToolbar
        if (this->window()->isFullScreen()) {
//            m_extensionPanel->move(- qMax(m_extensionPanel->width(),
//                                          EXTENSION_PANEL_WIDTH), 0);
//            m_extensionPanel->moveWithAnimation(0, 0);
            m_extensionPanel->move(width(), 0);
            m_extensionPanel->moveWithAnimation((width()-EXTENSION_PANEL_WIDTH-24), 0);
        } else {
//            m_extensionPanel->move(- qMax(m_extensionPanel->width(),
//                                   EXTENSION_PANEL_WIDTH), TOP_TOOLBAR_HEIGHT);
//            m_extensionPanel->moveWithAnimation(0, TOP_TOOLBAR_HEIGHT);
            m_extensionPanel->move(width(), TOP_TOOLBAR_HEIGHT);
            m_extensionPanel->moveWithAnimation((width()-EXTENSION_PANEL_WIDTH-24), TOP_TOOLBAR_HEIGHT);
        }
    });
    connect(dApp->signalM, &SignalManager::hideExtensionPanel,
            this, [=] (bool immediately) {
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
        }
        else {
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
}

//void MainWidget::initStyleSheet()
//{
//    if (dApp->viewerTheme->getCurrentTheme() == ViewerThemeManager::Dark) {
//        this->setStyleSheet(utils::base::getFileContent(":/resources/dark/qss/frame.qss"));
//    } else {
//        this->setStyleSheet(utils::base::getFileContent(":/resources/light/qss/frame.qss"));
//    }
//}
