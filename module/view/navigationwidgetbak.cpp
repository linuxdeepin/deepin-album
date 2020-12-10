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
#include "application.h"
#include "controller/configsetter.h"
#include "navigationwidget.h"
#include "widgets/imagebutton.h"
#include "utils/baseutils.h"

#include <QPainter>
#include <dwindowclosebutton.h>
#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>
#include <QtDebug>
#include <DDialogCloseButton>
#include <DGuiApplicationHelper>

namespace {

const QString SETTINGS_GROUP = "VIEWPANEL";
const QString SETTINGS_ALWAYSHIDDEN_KEY = "NavigationAlwaysHidden";
const int IMAGE_MARGIN = 5;
const int IMAGE_MARGIN_BOTTOM = 5;
const QString ICON_CLOSE_NORMAL_LIGHT = ":/resources/light/images/button_tab_close_normal 2.svg";
const QString ICON_CLOSE_HOVER_LIGHT = ":/resources/light/images/button_tab_close_hover 2.svg";
const QString ICON_CLOSE_PRESS_LIGHT = ":/resources/light/images/button_tab_close_press 2.svg";
const QString ICON_CLOSE_NORMAL_DARK = ":/resources/dark/images/button_tab_close_normal 3.svg";
const QString ICON_CLOSE_HOVER_DARK = ":/resources/dark/images/button_tab_close_hover 3.svg";
const QString ICON_CLOSE_PRESS_DARK = ":/resources/dark/images/button_tab_close_press 3.svg";

}  // namespace

using namespace Dtk::Widget;

NavigationWidget::NavigationWidget(QWidget *parent)
    : QWidget(parent)
{
    hide();
    resize(150, 112);
    onThemeChanged(dApp->viewerTheme->getCurrentTheme());

    ImageButton *closeBtn_light = new ImageButton(ICON_CLOSE_NORMAL_LIGHT, ICON_CLOSE_HOVER_LIGHT, ICON_CLOSE_PRESS_LIGHT, " ", this);
    closeBtn_light->setTooltipVisible(true);
    closeBtn_light->setFixedSize(32, 32);
    closeBtn_light->move(QPoint(this->x() + this->width() - 27 - 6,
                                rect().topRight().y() + 4 - 6));
    DPalette palette1 ;
    palette1.setColor(DPalette::Background, QColor(0, 0, 0, 1));
    closeBtn_light->setPalette(palette1);
    closeBtn_light->hide();
    connect(closeBtn_light, &ImageButton::clicked, [this]() {
        setAlwaysHidden(true);
    });

    ImageButton *closeBtn_dark = new ImageButton(ICON_CLOSE_NORMAL_DARK, ICON_CLOSE_HOVER_DARK, ICON_CLOSE_PRESS_DARK, " ", this);
    closeBtn_dark->setTooltipVisible(true);
    closeBtn_dark->setFixedSize(32, 32);
    closeBtn_dark->move(QPoint(this->x() + this->width() - 27 - 6,
                               rect().topRight().y() + 4 - 6));
    DPalette palette2 ;
    palette2.setColor(DPalette::Background, QColor(0, 0, 0, 1));
    closeBtn_dark->setPalette(palette2);
    closeBtn_dark->hide();
    connect(closeBtn_dark, &ImageButton::clicked, [this]() {
        setAlwaysHidden(true);
    });

    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::DarkType) {
        closeBtn_light->hide();
        closeBtn_dark->show();
    } else {
        closeBtn_dark->hide();
        closeBtn_light->show();
    }

    QObject::connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [ = ]() {
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::DarkType) {
            closeBtn_light->hide();
            closeBtn_dark->show();
            m_bgImgUrl = utils::view::naviwindow::DARK_BG_IMG ;
            m_BgColor = utils::view::naviwindow::DARK_BG_COLOR;
            m_mrBgColor = utils::view::naviwindow::DARK_MR_BG_COLOR;
            m_mrBorderColor = utils::view::naviwindow::DARK_MR_BORDER_Color;
            m_imgRBorderColor = utils::view::naviwindow:: DARK_IMG_R_BORDER_COLOR;
        } else {
            closeBtn_dark->hide();
            closeBtn_light->show();
            m_bgImgUrl = utils::view::naviwindow::LIGHT_BG_IMG ;
            m_BgColor = utils::view::naviwindow::LIGHT_BG_COLOR;
            m_mrBgColor = utils::view::naviwindow::LIGHT_MR_BG_COLOR;
            m_mrBorderColor = utils::view::naviwindow::LIGHT_MR_BORDER_Color;
            m_imgRBorderColor = utils::view::naviwindow::LIGHT_IMG_R_BORDER_COLOR;
        }
    });

    m_mainRect = QRect(rect().x() + IMAGE_MARGIN,
                       rect().y() + IMAGE_MARGIN_BOTTOM,
                       rect().width() - IMAGE_MARGIN * 2,
                       rect().height() - IMAGE_MARGIN_BOTTOM * 2);

    connect(dApp->viewerTheme, &ViewerThemeManager::viewerThemeChanged, this,
            &NavigationWidget::onThemeChanged);
}

void NavigationWidget::setAlwaysHidden(bool value)
{
    dApp->setter->setValue(SETTINGS_GROUP, SETTINGS_ALWAYSHIDDEN_KEY,
                           QVariant(value));
    if (isAlwaysHidden())
        hide();
    else
        show();
}

bool NavigationWidget::isAlwaysHidden() const
{
    return dApp->setter->value(SETTINGS_GROUP, SETTINGS_ALWAYSHIDDEN_KEY,
                               QVariant(false)).toBool();
}

void NavigationWidget::setImage(const QImage &img)
{
    const qreal ratio = devicePixelRatioF();

    QRect tmpImageRect = QRect(m_mainRect.x() * ratio, m_mainRect.y() * ratio,
                               qRound(m_mainRect.width() * ratio),
                               qRound(m_mainRect.height() * ratio));

    m_originRect = img.rect();

    // 只在图片比可显示区域大时才缩放
    if (tmpImageRect.width() < m_originRect.width() || tmpImageRect.height() < m_originRect.height()) {
        m_img = img.scaled(tmpImageRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    } else {
        m_img = img;
    }

    m_pix = QPixmap::fromImage(m_img);
    m_pix.setDevicePixelRatio(ratio);
    m_imageScale = qMax(1.0, qMax(qreal(img.width()) / qreal(m_img.width()), qreal(img.height()) / qreal(m_img.height())));
    m_r = QRectF(0, 0, m_img.width() / ratio, m_img.height() / ratio);

    update();
}

void NavigationWidget::setRectInImage(const QRect &r)
{
    if (m_img.isNull())
        return;
    m_r.setX((qreal)r.x() / m_imageScale );
    m_r.setY((qreal)r.y() / m_imageScale );
    m_r.setWidth((qreal)r.width() / m_imageScale );
    m_r.setHeight((qreal)r.height() / m_imageScale );

    update();
}

void NavigationWidget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
        tryMoveRect(e->pos() / devicePixelRatioF());
}

void NavigationWidget::mouseMoveEvent(QMouseEvent *e)
{
    tryMoveRect(e->pos() / devicePixelRatioF());
}

void NavigationWidget::tryMoveRect(const QPoint &p)
{
    const int x0 = (m_mainRect.width() - m_img.width()) / 2;
    const int y0 = (m_mainRect.height() - m_img.height()) / 2;
    const QRect imageRect(x0, y0, m_img.width(), m_img.height());
    if (! imageRect.contains(p))
        return;
    const qreal x = 1.0 * (p.x() - x0) / m_img.width() * m_originRect.width();
    const qreal y = 1.0 * (p.y() - y0) / m_img.height() * m_originRect.height();

    Q_EMIT requestMove(x, y);
}

void NavigationWidget::paintEvent(QPaintEvent *)
{
    QImage img(m_img);
    if (m_img.isNull()) {
        QPainter p(this);
        p.fillRect(m_r, m_BgColor);
        return;
    }

    const qreal ratio = devicePixelRatioF();

    QPainter p(&img);
    p.fillRect(m_r, m_mrBgColor);
    p.setPen(m_mrBorderColor);
    p.drawRect(m_r);
    p.end();
    p.begin(this);

    QImage background(m_bgImgUrl);

    p.drawImage(this->rect(), background);
    QRect imageDrawRect =  QRect((m_mainRect.width() - m_img.width() / ratio) / 2 + IMAGE_MARGIN,
                                 (m_mainRect.height() - m_img.height() / ratio) / 2 + utils::common::BORDER_WIDTH,
                                 m_img.width() / ratio, m_img.height() / ratio);
    //**draw transparent background
//    QPixmap pm(12, 12);
//    QPainter pmp(&pm);
//    //TODO: the transparent box
//    //should not be scaled with the image
//    pmp.fillRect(0, 0, 6, 6, LIGHT_CHECKER_COLOR);
//    pmp.fillRect(6, 6, 6, 6, LIGHT_CHECKER_COLOR);
//    pmp.fillRect(0, 6, 6, 6, DARK_CHECKER_COLOR);
//    pmp.fillRect(6, 0, 6, 6, DARK_CHECKER_COLOR);
//    pmp.end();

//    p.fillRect(imageDrawRect, QBrush(pm));
    p.drawImage(imageDrawRect, img);
    QRect borderRect = QRect(imageDrawRect.x(), imageDrawRect.y(), imageDrawRect.width()
                             - utils::common::BORDER_WIDTH, imageDrawRect.height() - utils::common::BORDER_WIDTH);
    p.setPen(m_imgRBorderColor);
    p.drawRect(borderRect);
    p.end();
}

void NavigationWidget::onThemeChanged(ViewerThemeManager::AppTheme theme)
{
    if (theme == ViewerThemeManager::Dark) {
        m_bgImgUrl = utils::view::naviwindow::DARK_BG_IMG ;
        m_BgColor = utils::view::naviwindow::DARK_BG_COLOR;
        m_mrBgColor = utils::view::naviwindow::DARK_MR_BG_COLOR;
        m_mrBorderColor = utils::view::naviwindow::DARK_MR_BORDER_Color;
        m_imgRBorderColor = utils::view::naviwindow:: DARK_IMG_R_BORDER_COLOR;
    } else {
        m_bgImgUrl = utils::view::naviwindow::LIGHT_BG_IMG ;
        m_BgColor = utils::view::naviwindow::LIGHT_BG_COLOR;
        m_mrBgColor = utils::view::naviwindow::LIGHT_MR_BG_COLOR;
        m_mrBorderColor = utils::view::naviwindow::LIGHT_MR_BORDER_Color;
        m_imgRBorderColor = utils::view::naviwindow::LIGHT_IMG_R_BORDER_COLOR;
    }
    update();
}