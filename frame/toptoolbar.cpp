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
#include "toptoolbar.h"
#include "application.h"
#include "controller/configsetter.h"
#include "controller/importer.h"
#include "controller/signalmanager.h"
#include "controller/viewerthememanager.h"
#include "settings/settingswindow.h"
#include "widgets/dialogs/aboutdialog.h"
#include "utils/baseutils.h"
#include "utils/shortcut.h"

#include <dwindowminbutton.h>
#include <dwindowmaxbutton.h>
#include <dwindowclosebutton.h>
#include <dwindowoptionbutton.h>
#include <dtitlebar.h>

#include <QDebug>
#include <QHBoxLayout>
#include <QPainter>
#include <QProcess>
#include <QResizeEvent>
#include <QShortcut>
#include <QStyleFactory>
#include <QImageReader>
#include <QLabel>
#include <DFontSizeManager>
#include <DApplicationHelper>
DWIDGET_USE_NAMESPACE

namespace {

const int TOP_TOOLBAR_HEIGHT = 50;
const int ICON_MARGIN = 6;

//const QColor DARK_COVERCOLOR = QColor(0, 0, 0, 217);
//const QColor LIGHT_COVERCOLOR = QColor(255, 255, 255, 230);

const QColor DARK_TOP_BORDERCOLOR = QColor(255, 255, 255, 0);
const QColor LIGHT_TOP_BORDERCOLOR = QColor(255, 255, 255, 0);

const QColor DARK_BOTTOM_BORDERCOLOR = QColor(0, 0, 0, 51);
const QColor LIGHT_BOTTOM_BORDERCOLOR = QColor(0, 0, 0, 26);
}  // namespace

TopToolbar::TopToolbar(bool manager, QWidget *parent)
    :DBlurEffectWidget(parent)
{
    m_manager = manager;

    QPalette palette;
    palette.setColor(QPalette::Background, QColor(0,0,0,0)); // 最后一项为透明度
    setPalette(palette);

    initMenu();
    initWidgets();

}

void TopToolbar::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        if (window()->isMaximized())
            window()->showNormal();
        else if (! window()->isFullScreen())  // It would be normal state
            window()->showMaximized();
    }

    DBlurEffectWidget::mouseDoubleClickEvent(e);
}

void TopToolbar::paintEvent(QPaintEvent *e)
{
    QPainter p(this);

    QPixmap pixmap(":/resources/common/ttb60.svg");
    const QPalette pal = QGuiApplication::palette();//this->palette();
    QBrush bgColor = QBrush(pixmap.scaled(size().width(),60));
    QRectF bgRect;
    bgRect.setSize(size());
    QPainterPath pp;
    pp.addRoundedRect(QRectF(bgRect.x(),bgRect.y(),bgRect.width(),60), 0, 0);
    p.fillPath(pp, bgColor);
}

void TopToolbar::initWidgets()
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    m_titlebar = new DTitlebar(this);
    m_titlebar->setWindowFlags(Qt::WindowMinMaxButtonsHint |Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
    m_titlebar->setMenu(m_menu);
    QPalette pa;
    pa.setColor(QPalette::WindowText,QColor(255,255,255,255));
    m_titlebar->setIcon(QIcon::fromTheme("deepin-album"));

    QObject::connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [=](){
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();

        QPalette pa1;
        pa1.setColor(QPalette::ButtonText,QColor(255,255,255,204));
        m_titlebar->setPalette(pa1);
    });

    m_titlebar->setTitle("");

    m_titletxt=new DLabel;
    m_titletxt->setText("");
    m_titletxt->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T7));
    DPalette pa1 = DApplicationHelper::instance()->palette(m_titletxt);
    pa1.setBrush(DPalette::WindowText, pa1.color(DPalette::TextLively));
    m_titletxt->setPalette(pa);
    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect(m_titletxt);
    shadowEffect->setOffset(0, 1);
    shadowEffect->setBlurRadius(1);
    m_titletxt->setGraphicsEffect(shadowEffect);
    m_titlebar->addWidget(m_titletxt,Qt::AlignCenter);
    QPalette titleBarPA;
    titleBarPA.setColor(QPalette::ButtonText,QColor(255,255,255,204));
    titleBarPA.setColor(QPalette::WindowText,QColor(255,255,255,255));
    m_titlebar->setPalette(titleBarPA);
    m_titlebar->setBackgroundTransparent(true);
    m_layout->addWidget(m_titlebar);
    connect(dApp->signalM, &SignalManager::updateFileName,
            this, [ = ](const QString & filename){
        QString a = geteElidedText(DFontSizeManager::instance()->get(DFontSizeManager::T7),filename,width()-500);
        m_titletxt->setText(a);
        connect(dApp->signalM, &SignalManager::resizeFileName,
                this, [ = ](){
            QString b = geteElidedText(DFontSizeManager::instance()->get(DFontSizeManager::T7),filename,width()-500);
            m_titletxt->setText(b);
        });
    });
}

QString TopToolbar::geteElidedText(QFont font, QString str, int MaxWidth)
{
    QFontMetrics fontWidth(font);
    int width = fontWidth.width(str);
    if(width>=MaxWidth){
        str = fontWidth.elidedText(str,Qt::ElideRight,MaxWidth);
    }
    return str;
}

void TopToolbar::initMenu()
{
    m_menu = new DMenu(this);

    m_menu->addSeparator();

    QShortcut *scE = new QShortcut(QKeySequence("Ctrl+Q"), this);
    QShortcut *scViewShortcut = new QShortcut(QKeySequence("Ctrl+Shift+/"), this);

    connect(scE, SIGNAL(activated()), dApp, SLOT(quit()));
    connect(scViewShortcut, SIGNAL(activated()), this, SLOT(onViewShortcut()));

}

void TopToolbar::onViewShortcut() {
    QRect rect = window()->geometry();
    QPoint pos(rect.x() + rect.width()/2 , rect.y() + rect.height()/2);
    Shortcut sc;
    QStringList shortcutString;
    QString param1 = "-j="+sc.toStr();
    QString param2 = "-p=" + QString::number(pos.x()) + "," + QString::number(pos.y());
    shortcutString << param1 << param2;

    QProcess::startDetached("deepin-shortcut-viewer", shortcutString);
}

//void TopToolbar::onAbout()
//{
//    AboutDialog *ad = new AboutDialog;
//    ad->show();
//    QWidget *w = window();
//    QPoint gp = w->mapToGlobal(QPoint(0, 0));
//    ad->move((w->width() - ad->width()) / 2 + gp.x(),
//               (w->height() - ad->sizeHint().height()) / 2 + gp.y());
//}

void TopToolbar::onHelp()
{
    if (m_manualPro.isNull()) {
        const QString pro = "dman";
        const QStringList args("deepin-image-viewer");
        m_manualPro = new QProcess(this);
        connect(m_manualPro.data(), SIGNAL(finished(int)),
                m_manualPro.data(), SLOT(deleteLater()));
        m_manualPro->start(pro, args);
    }
}

#ifndef LITE_DIV
void TopToolbar::onNewAlbum()
{
    emit dApp->signalM->createAlbum();
}

void TopToolbar::onSetting()
{
    m_settingsWindow->move((width() - m_settingsWindow->width()) / 2 +
                           mapToGlobal(QPoint(0, 0)).x(),
                           (window()->height() - m_settingsWindow->height()) / 2 +
                           mapToGlobal(QPoint(0, 0)).y());
    m_settingsWindow->show();
}
#endif

void TopToolbar::onDeepColorMode() {
    if (dApp->viewerTheme->getCurrentTheme() == ViewerThemeManager::Dark) {
        dApp->viewerTheme->setCurrentTheme(
                    ViewerThemeManager::Light);
    } else {
        dApp->viewerTheme->setCurrentTheme(
                    ViewerThemeManager::Dark);
    }
}
