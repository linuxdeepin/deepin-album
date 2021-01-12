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
#include "extensionpanel.h"
#include <QPainter>
#include "application.h"
#include "controller/signalmanager.h"
#include "darrowbutton.h"
#include <DFontSizeManager>

using namespace Dtk::Widget;

namespace {

// const int CONTROL_BUTTON_WIDTH = 20;
// const int CONTROL_BUTTON_HEIGHT = 60;
// const int CONTROL_BUTTON_CUBIC_LENGTH = 30;
const int EXTENSION_PANEL_WIDTH = 300 + 20;
// const int EXTENSION_PANEL_MAX_WIDTH = 340;

const QColor DARK_COVERBRUSH = QColor(0, 0, 0, 100);
const QColor LIGHT_COVERBRUSH = QColor(255, 255, 255, 179);
//const int ANIMATION_DURATION = 500;
const QEasingCurve ANIMATION_EASING_CURVE = QEasingCurve::InOutCubic;
}  // namespace

ExtensionPanel *ExtensionPanel::instance = nullptr;

ExtensionPanel::ExtensionPanel(QWidget *parent)
    : DDialog(parent)
{
    this->setWindowTitle(tr("Photo info"));
    DFontSizeManager::instance()->bind(this, DFontSizeManager::T6, QFont::Medium);

//    m_contentLayout = new QVBoxLayout(this);
//    m_contentLayout->setContentsMargins(10, 0, 10, 0);
//    m_contentLayout->setSpacing(0);
    setFixedWidth(EXTENSION_PANEL_WIDTH);
    setFixedHeight(540);
    setContentLayoutContentsMargins(QMargins(0, 0, 0, 0));
    setModal(true);
}

ExtensionPanel *ExtensionPanel::getInstance(QWidget *parent)
{
    if (instance == nullptr) {
        instance = new ExtensionPanel(parent);
    }
    return instance;
}

void ExtensionPanel::setContent(QWidget *content)
{
    if (content) {
        QLayoutItem *child;
        if ((child = this->layout()->takeAt(0)) != nullptr) {
            if (child->widget())
                child->widget()->deleteLater();
            delete child;
        }

        m_content = content;
        updateRectWithContent();
        this->addContent(content);
    }
}

void ExtensionPanel::updateRectWithContent()
{
    connect(dApp->signalM, &SignalManager::extensionPanelHeight, this,
    [ = ](int height) {
        setFixedHeight(qMin(615, height + 5)); //tmp for imageinfo
    });
}

void ExtensionPanel::mouseMoveEvent(QMouseEvent *e)
{
    Q_UNUSED(e);
    DDialog::mouseMoveEvent(e);
}

void ExtensionPanel::paintEvent(QPaintEvent *pe)
{
    DDialog::paintEvent(pe);
}
