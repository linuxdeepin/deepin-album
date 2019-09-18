#include "timelinelist.h"

TimelineList::TimelineList(QWidget *parent) : DListWidget(parent)
{
    setContentsMargins(0,0,0,0);
    setResizeMode(QListView::Adjust);
    setViewMode(QListView::ListMode);
    setFlow(QListView::TopToBottom);
    setSpacing(0);
    setDragEnabled(false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    has = false;

}

void TimelineList::addItemForWidget(QListWidgetItem *aitem)
{
    int y = 1;
    yList.append(y);
    this->addItem(aitem);

}

void TimelineList::dragMoveEvent(QDragMoveEvent *event)
{
    return QListWidget::dragMoveEvent(event);
}

void TimelineList::mouseMoveEvent(QMouseEvent *event)
{
    return QListWidget::mouseMoveEvent(event);
}

void TimelineList::paintEvent(QPaintEvent *e)
{
    if(this->count() > 0)
    {
        int ccount = count();
        for(int i = 0; i < ccount; i++)
        {
            QListWidgetItem *pItem = this->item(i);
            TimelineItem* pWidget = (TimelineItem*)itemWidget(pItem);
            if((pWidget->y() <= pWidget->m_title->height()) && (pWidget->y() > 0))
            {
                emit sigMoveTime(pWidget->y() - pWidget->m_title->height());
                pWidget->m_title->setVisible(true);
                pWidget->m_date->setText(pWidget->m_sdate);
                pWidget->m_num->setText(pWidget->m_snum);

            }
            else if((pWidget->y() <= 0)&& (pWidget->y() + pWidget->m_title->height()))
            {
                has = false;
                emit sigNewTime(pWidget->m_sdate,pWidget->m_snum,i);
                pWidget->m_date->setText("");
                pWidget->m_num->setText("");
            }
            yList[i] = pWidget->y();
        }
    }

    return QListWidget::paintEvent(e);
}
