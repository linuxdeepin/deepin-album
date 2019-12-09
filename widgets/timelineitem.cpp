#include "timelineitem.h"

TimelineItem::TimelineItem(QWidget *parent) : QWidget(parent)
{

}

void TimelineItem::mousePressEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton){
        emit sigMousePress();
    }
    QWidget::mousePressEvent(e);
}


