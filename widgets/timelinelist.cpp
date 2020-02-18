#include "timelinelist.h"
#include "controller/signalmanager.h"
#include <QScrollBar>

//TimelineListDelegate::TimelineListDelegate(QObject *parent)
//    : QStyledItemDelegate(parent)
//{
//}

//void TimelineListDelegate::paint(QPainter *painter,
//                                 const QStyleOptionViewItem &option,
//                                 const QModelIndex &index) const
//{
//    int row = index.row();
//    int x = option.rect.x();
//    int y = option.rect.y();
//    int width = option.rect.width();
//    int height = option.rect.height();

//    QStyleOptionViewItem loption = option;
//    if (row == 0 ) { //UE
//        //qDebug()<<"row == 0";

//        //选项
//        QStyleOptionFrame *FrameOption = new QStyleOptionFrame();
//        FrameOption->rect = QRect(x, y, width, 50);
//        //绘制
//        QApplication::style()->drawControl(QStyle::CE_ShapedFrame, FrameOption, painter);
//        loption.rect = QRect(x, y + 50, width, height - 50);
//    }

//    return QStyledItemDelegate::paint (painter, loption, index);
//}

TimelineList::TimelineList(QWidget *parent) : DListWidget(parent)
{
    setContentsMargins(0, 0, 0, 0);
    setResizeMode(QListView::Adjust);
    setViewMode(QListView::ListMode);
    setFlow(QListView::TopToBottom);
    setSpacing(0);
    setDragEnabled(false);
    connect(this->verticalScrollBar(), &QScrollBar::rangeChanged, this, [ = ](int min, int max) {
        QScrollBar *bar = this->verticalScrollBar();
        bar->setGeometry(bar->x(), /*bar->y() + */m_scrollbartopdistance, bar->width(), this->height() - m_scrollbartopdistance - m_scrollbarbottomdistance);
    });
//    TimelineListDelegate *m_delegate = new TimelineListDelegate;
//    setItemDelegate(m_delegate);

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
    QListWidget::paintEvent(e);
    QScrollBar *bar = this->verticalScrollBar();
    bar->setGeometry(bar->x(), /*bar->y() + */m_scrollbartopdistance, bar->width(), this->height() - m_scrollbartopdistance - m_scrollbarbottomdistance);

    int blankHeight = 0; //add 3975
    if (this->count() > 0) {
        int ccount = count();
        for (int i = 0; i < ccount; i++) {
            QListWidgetItem *pItem = this->item(i);
            TimelineItem *pWidget = (TimelineItem *)itemWidget(pItem);
            //add start 3975
            if (pWidget->m_type == "blank") {
                blankHeight = 47;
            }
            //add end 3975
            if (pWidget->m_type != "blank") { //add 3975
                //add 3975
                if ((pWidget->y() <= pWidget->m_title->height() + 47) && (pWidget->y() > blankHeight)) { //edit 3975
#if 1
                    QListWidgetItem *pLastItem;
                    pLastItem = this->item(i - 1);
                    TimelineItem *pLastWidget = (TimelineItem *)itemWidget(pLastItem);
                    emit sigMoveTime(pWidget->y() - pWidget->m_title->height() - 47, pLastWidget->m_sdate, pLastWidget->m_snum, pLastWidget->m_Chose->text());
#endif
                    pWidget->m_title->setVisible(true);
                    pWidget->m_date->setText(pWidget->m_sdate);
                    pWidget->m_num->setText(pWidget->m_snum);
                    pWidget->m_Chose->setVisible(true);

                    //            } else if ((pWidget->y() <= 0) && (pWidget->y() + pWidget->m_title->height() > 0)) {
                } else if ((pWidget->y() <= blankHeight + 47) ) { //edit 3975
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
}

bool TimelineList::eventFilter(QObject *obj, QEvent *e)
{
    Q_UNUSED(obj)
    if (e->type() == QEvent::Wheel && QApplication::keyboardModifiers () == Qt::ControlModifier) {
        return true;
    }

    return false;
}
