#include "dlmenuarrow.h"

DLMenuArrow::DLMenuArrow(QWidget *parent)
    : DPushButton(parent)
{
    setFlat(true);
    setWindowOpacity(0);
    //setAttribute(Qt::WA_TranslucentBackground, true);

    QObject::connect(this, &DLMenuArrow::clicked, this, &DLMenuArrow::onClicked);
}

void DLMenuArrow::setWgt(QWidget *wgt, ActId id)
{
    m_pParentWgt = wgt;
    m_actId = id;
}

void DLMenuArrow::setMouseEnter(bool bFlag)
{
    m_bMouseEnter = bFlag;
    update();
}


void DLMenuArrow::paintEvent(QPaintEvent *ev)
{
    Q_UNUSED(ev);

    QImage image(width(), height(), QImage::Format_ARGB32);
    QPainter pt(this);
    pt.setRenderHint(QPainter::HighQualityAntialiasing);
    pt.begin(&image);

    if (m_bMouseEnter) {
        pt.setPen("#0081FF");
        pt.setBrush(QBrush("#0081FF"));
        pt.drawRect(0, 0, width(), height());
    }
//    else {
//        setWindowOpacity(0);
//    }

    QPalette pa = m_pParentWgt->palette();
    QColor color = pa.color(QPalette::Background);
    QString strRgb = color.name();
    if (strRgb == "#f8f8f8") {
        pt.setPen(QPen(Qt::black));
    }
    else {
        pt.setPen(QPen(Qt::white));
    }

    if (IdArrowUp == m_actId) {
        QPainterPath arrow_up;
        arrow_up.moveTo(width()/2 - OFFSET, OFFSET);
        arrow_up.lineTo(width()/2, 0);
        arrow_up.lineTo(width()/2 + OFFSET, OFFSET);
        pt.drawPath(arrow_up);
    }
    else {
        QPainterPath arrow_down;
        arrow_down.moveTo(width()/2 - OFFSET, height() - OFFSET - 1 );
        arrow_down.lineTo(width()/2, height() - 1);
        arrow_down.lineTo(width()/2 + OFFSET, height() - OFFSET - 1);
        pt.drawPath(arrow_down);
    }

    pt.end();
}


void DLMenuArrow::enterEvent(QEvent *ev)
{
    m_bMouseEnter = true;
    emit sigMouseEnter();
    return QWidget::enterEvent(ev);
}

void DLMenuArrow::leaveEvent(QEvent *ev)
{
    m_bMouseEnter = false;
    return QWidget::enterEvent(ev);
}

void DLMenuArrow::onClicked()
{
    m_bMouseEnter = false;
    setAttribute(Qt::WA_TranslucentBackground, true);
    update();
}



