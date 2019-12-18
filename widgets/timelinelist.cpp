#include "timelinelist.h"
#include "controller/signalmanager.h"

TimelineList::TimelineList(QWidget *parent) : DListWidget(parent)
{
    setContentsMargins(0, 0, 0, 0);
    setResizeMode(QListView::Adjust);
    setViewMode(QListView::ListMode);
    setFlow(QListView::TopToBottom);
    setSpacing(0);
    setDragEnabled(false);
//    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    has = false;
    installEventFilter(this);

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
    int blankHeight = 0; //add 3975
    if (this->count() > 0) {
        int ccount = count();
        for (int i = 0; i < ccount; i++) {
            QListWidgetItem *pItem = this->item(i);
            TimelineItem *pWidget = (TimelineItem *)itemWidget(pItem);
            //add start 3975
            if (pWidget->m_type == "blank")
            {
                   blankHeight = 47;
            }
            //add end 3975
            if (pWidget->m_type != "blank")  //add 3975
            {   //add 3975
                if ((pWidget->y() <= pWidget->m_title->height()) && (pWidget->y() > blankHeight)) { //edit 3975
    #if 1
                    QListWidgetItem *pLastItem;
                    pLastItem = this->item(i - 1);
                    TimelineItem* pLastWidget = (TimelineItem*)itemWidget(pLastItem);
                    emit sigMoveTime(pWidget->y() - pWidget->m_title->height(),pLastWidget->m_sdate,pLastWidget->m_snum,pLastWidget->m_Chose->text());
    #endif
                    pWidget->m_title->setVisible(true);
                    pWidget->m_date->setText(pWidget->m_sdate);
                    pWidget->m_num->setText(pWidget->m_snum);
                    pWidget->m_Chose->setVisible(true);

    //            } else if ((pWidget->y() <= 0) && (pWidget->y() + pWidget->m_title->height() > 0)) {
                } else if ((pWidget->y() <= blankHeight) ) { //edit 3975
                    has = false;
                    emit sigNewTime(pWidget->m_sdate, pWidget->m_snum, i);
                    pWidget->m_date->setText("");
                    pWidget->m_num->setText("");
                    pWidget->m_Chose->setVisible(false);
                } else {
                    pWidget->m_title->setVisible(true);
                    pWidget->m_date->setText(pWidget->m_sdate);
                    pWidget->m_num->setText(pWidget->m_snum);
                    pWidget->m_Chose->setVisible(true);
                }
                yList[i] = pWidget->y();
            } //add 3975
        }
    }
    return QListWidget::paintEvent(e);
}

bool TimelineList::eventFilter(QObject *obj, QEvent *e)
{
    Q_UNUSED(obj)
    if (e->type() == QEvent::Wheel && QApplication::keyboardModifiers () == Qt::ControlModifier) {
        return true;
    }

    return false;
}
