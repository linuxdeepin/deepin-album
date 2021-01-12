#include "timelineitem.h"
#include <QApplication>

TimelineItem::TimelineItem(QWidget *parent)
    : QWidget(parent), m_title(nullptr), m_date(nullptr)
    , m_num(nullptr), m_Chose(nullptr)
{

}

void TimelineItem::mousePressEvent(QMouseEvent *e)
{
    if (QApplication::keyboardModifiers() != Qt::ControlModifier && e->button() == Qt::LeftButton) {
        emit sigMousePress();
    }
    QWidget::mousePressEvent(e);
}


