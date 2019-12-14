#include "timelineitem.h"
#include <QApplication>

TimelineItem::TimelineItem(QWidget *parent) : QWidget(parent)
{

}

void TimelineItem::mousePressEvent(QMouseEvent *e)
{
    if(QApplication::keyboardModifiers() != Qt::ControlModifier && e->button() == Qt::LeftButton){
        emit sigMousePress();
    }
    QWidget::mousePressEvent(e);
}


