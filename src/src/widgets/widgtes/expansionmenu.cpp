// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "expansionmenu.h"
#include "expansionpanel.h"
#include "widgets/thumbnail/timelinedatewidget.h"
#include <QHBoxLayout>
#include <DCommandLinkButton>
#include <DFontSizeManager>
#include <DGuiApplicationHelper>
#include <DHiDPIHelper>
#include <DPaletteHelper>
#include <QDebug>

#include <DLabel>
#include <QAbstractButton>
#include <QMouseEvent>
#include <QScreen>

FilterWidget::FilterWidget(QWidget *parent): QWidget(parent)
{
    qDebug() << "Creating FilterWidget";
    QHBoxLayout *hb = new QHBoxLayout(this);
    hb->setSpacing(4);
    hb->setContentsMargins(0, 0, 0, 0);
    this->setContentsMargins(0, 0, 10, 0);
    this->setLayout(hb);

    m_leftLabel = new FilterLabel(this);
    m_leftLabel->setFixedSize(QSize(16, 16));
    DFontSizeManager::instance()->bind(m_leftLabel, DFontSizeManager::T7, QFont::Normal);
    hb->addWidget(m_leftLabel);

    m_btn = new FilterLabel(this);
    DFontSizeManager::instance()->bind(m_btn, DFontSizeManager::T7, QFont::Normal);
    hb->addWidget(m_btn);

    m_rightLabel = new FilterLabel(this);
    DFontSizeManager::instance()->bind(m_rightLabel, DFontSizeManager::T7, QFont::Normal);
    hb->addWidget(m_rightLabel);
    m_rightLabel->setPixmap(QIcon::fromTheme("album_arrowdown").pixmap(QSize(8, 6)));

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
            this, &FilterWidget::themeTypeChanged);
    themeTypeChanged(DGuiApplicationHelper::instance()->themeType());

    m_btn->installEventFilter(this);
    m_rightLabel->installEventFilter(this);
    qDebug() << "FilterWidget initialization completed";
}

FilterWidget::~FilterWidget()
{
    // qDebug() << "Destroying FilterWidget";
}

void FilterWidget::setIcon(QIcon icon)
{
    // qDebug() << "FilterWidget::setIcon - Entry";
    m_leftLabel->setPixmap(icon.pixmap(QSize(14, 14)));
}

void FilterWidget::setText(QString text)
{
    // qDebug() << "FilterWidget::setText - Entry";
    m_btn->setText(text);
}

void FilterWidget::setFilteData(ExpansionPanel::FilteData &data)
{
    // qDebug() << "FilterWidget::setFilteData - Entry";
    m_data = data;
    themeTypeChanged(DGuiApplicationHelper::instance()->themeType());
    m_btn->setText(data.text);
}

ExpansionPanel::FilteData FilterWidget::getFilterData()
{
    // qDebug() << "FilterWidget::getFilterData - Entry";
    return m_data;
}

ItemType FilterWidget::getFilteType()
{
    // qDebug() << "FilterWidget::getFilteType - Entry";
    return m_data.type;
}

void FilterWidget::onClicked()
{
    // qDebug() << "FilterWidget::onClicked - Entry";
    emit clicked();
}

void FilterWidget::themeTypeChanged(int type)
{
    // qDebug() << "FilterWidget::themeTypeChanged - Entry";
    DPalette pal = DPaletteHelper::instance()->palette(m_btn);
    if (type == 1) {
        // qDebug() << "FilterWidget::themeTypeChanged - Entry, type is 1";
        QString path = ":/icons/deepin/builtin/icons/light/";
        path += m_data.icon_r_path;
        path += "_16px.svg";
        QPixmap pix = DHiDPIHelper::loadNxPixmap(path);
        const qreal ratio = devicePixelRatioF();
        pix.setDevicePixelRatio(ratio);
        m_leftLabel->setPixmap(pix);
        m_rightLabel->setPixmap(DHiDPIHelper::loadNxPixmap(":/icons/deepin/builtin/icons/light/album_arrowdown_10px.svg").scaled(10, 10));
        m_btn->setText(m_data.text);
        pal.setBrush(DPalette::Text, lightTextColor);
    } else {
        // qDebug() << "FilterWidget::themeTypeChanged - Entry, type is 2";
        QString path = ":/icons/deepin/builtin/icons/dark";
        path += m_data.icon_r_path;
        path += "_16px.svg";
        QPixmap pix = DHiDPIHelper::loadNxPixmap(path);
        const qreal ratio = devicePixelRatioF();
        pix.setDevicePixelRatio(ratio);
        m_leftLabel->setPixmap(pix);
        m_rightLabel->setPixmap(DHiDPIHelper::loadNxPixmap(":/icons/deepin/builtin/icons/darkalbum_arrowdown_10px.svg").scaled(10, 10));
        m_btn->setText(m_data.text);
        pal.setBrush(DPalette::Text, darkTextColor);
    }
    m_btn->setForegroundRole(DPalette::Text);
    m_btn->setPalette(pal);
    // qDebug() << "FilterWidget::themeTypeChanged - Exit";
}

bool FilterWidget::eventFilter(QObject *obj, QEvent *event)
{
    // qDebug() << "FilterWidget::eventFilter - Entry";
    if (obj == m_btn || obj == m_rightLabel) {
        QMouseEvent *e = dynamic_cast<QMouseEvent *>(event);
        if (e && (e->type() == QEvent::MouseMove)) {
            //解决触屏上点击后移动整个应用问题
            return true;
        } else if (e && e->type() == QEvent::MouseButtonPress) {
            qDebug() << "Mouse press event on" << (obj == m_btn ? "button" : "right label");
            onClicked();
            return true;
        }
    }
    // qDebug() << "FilterWidget::eventFilter - Exit";
    return QWidget::eventFilter(obj, event);
}

void FilterWidget::resizeEvent(QResizeEvent *e)
{
    // qDebug() << "Filter widget resized to width:" << this->width();
    sigWidthChanged(this->width());
    QWidget::resizeEvent(e);
}

ExpansionMenu::ExpansionMenu(QWidget *parent)
    : QObject(parent)
{
    qDebug() << "Creating ExpansionMenu";
    panel = new ExpansionPanel();
    panel->setVisible(false);
    mainButton = new FilterWidget(parent);
    connect(panel, &ExpansionPanel::currentItemChanged, this, &ExpansionMenu::onCurrentItemChanged);
    connect(mainButton, &FilterWidget::clicked, this, &ExpansionMenu::onMainButtonClicked);
    qDebug() << "ExpansionMenu initialization completed";
}

FilterWidget *ExpansionMenu::mainWidget()
{
    // qDebug() << "ExpansionMenu::mainWidget - Entry";
    return mainButton;
}

void ExpansionMenu::onCurrentItemChanged(ExpansionPanel::FilteData &data)
{
    qDebug() << "Current item changed - type:" << data.type << "text:" << data.text;
    mainButton->setFilteData(data);
    emit mainButton->currentItemChanged(data);
}

void ExpansionMenu::onMainButtonClicked()
{
    qDebug() << "Main button clicked - panel visibility:" << panel->isHidden();
    panel->isHidden() ? panel->show() : panel->hide();
    //不允许弹窗被右侧屏幕遮挡部分
    QList<QScreen *> screens = QGuiApplication::screens();
    int width = 0;//所有屏幕整体宽度
    for (int i = 0; i < screens.size(); i++) {
        width += screens.at(i)->availableGeometry().width();
    }
    if (width - QCursor().pos().x() < 190) {
        qDebug() << "Adjusting panel position to prevent right screen overlap";
        panel->setGeometry(width - 191, QCursor().pos().y() + 15, 0, 0);
    } else {
        panel->setGeometry(QCursor().pos().x(), QCursor().pos().y() + 15, 0, 0);
    }
}

void ExpansionMenu::addNewButton(ExpansionPanel::FilteData &data)
{
    // qDebug() << "ExpansionMenu::addNewButton - Entry";
    panel->addNewButton(data);
}

void ExpansionMenu::setDefaultFilteData(ExpansionPanel::FilteData &data)
{
    // qDebug() << "ExpansionMenu::setDefaultFilteData - Entry";
    mainButton->setFilteData(data);
}

FilterLabel::FilterLabel(QWidget *parent)
{
    // qDebug() << "Creating FilterLabel";
}

FilterLabel::~FilterLabel()
{
    // qDebug() << "Destroying FilterLabel";
}

void FilterLabel::mouseReleaseEvent(QMouseEvent *ev)
{
    // qDebug() << "FilterLabel::mouseReleaseEvent - Entry";
    emit clicked();
    QLabel::mouseReleaseEvent(ev);
    // qDebug() << "FilterLabel::mouseReleaseEvent - Exit";
}
