#include "dfmdarrowlineexpand.h"

#include <DFontSizeManager>
#include <DApplicationHelper>
#include <QPainter>
#include <QPainterPath>

DFMDArrowLineExpand::DFMDArrowLineExpand()
{
//    if (headerLine()) {
//        DFontSizeManager::instance()->bind(headerLine(), DFontSizeManager::T6);

//        DPalette pa = DApplicationHelper::instance()->palette(headerLine());
//        pa.setBrush(DPalette::Text, pa.color(DPalette::TextTitle));
//        headerLine()->setPalette(pa);
//        connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this, [ = ] {
//            DPalette pa = DApplicationHelper::instance()->palette(headerLine());
//            pa.setBrush(DPalette::Text, pa.color(DPalette::TextTitle));
//            headerLine()->setPalette(pa);
//        });

//        headerLine()->setLeftMargin(10);
//    }
}

void DFMDArrowLineExpand::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    QRectF bgRect;
    bgRect.setSize(size());
    const QPalette pal = QGuiApplication::palette();//this->palette();
    QColor bgColor = pal.color(QPalette::Background);

    QPainterPath path;
    path.addRoundedRect(bgRect, 8, 8);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillPath(path, bgColor);
    painter.setRenderHint(QPainter::Antialiasing, false);
}
