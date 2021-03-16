/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     ZhangYong <zhangyong@uniontech.com>
 *
 * Maintainer: ZhangYong <ZhangYong@uniontech.com>
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
#include "timelinelist.h"
#include "controller/signalmanager.h"
#include <QScrollBar>

TimelineListWidget::TimelineListWidget(QWidget *parent)
    : DListWidget(parent), has(false), m_scrollbartopdistance(50)
    , m_scrollbarbottomdistance(27)
{
    setContentsMargins(0, 0, 0, 0);
    setResizeMode(QListView::Adjust);
    setViewMode(QListView::ListMode);
    setFlow(QListView::TopToBottom);
    setSpacing(0);
    setDragEnabled(false);
    connect(this->verticalScrollBar(), &QScrollBar::rangeChanged, this, &TimelineListWidget::onRangeChanged);
    installEventFilter(this);
}

void TimelineListWidget::addItemForWidget(QListWidgetItem *aitem)
{
    int y = 1;
    yList.append(y);
    this->addItem(aitem);
}

void TimelineListWidget::onRangeChanged(int min, int max)
{
    Q_UNUSED(max);
    Q_UNUSED(min);
    QScrollBar *bar = this->verticalScrollBar();
    bar->setGeometry(bar->x(), /*bar->y() + */m_scrollbartopdistance, bar->width(), this->height() - m_scrollbartopdistance - m_scrollbarbottomdistance);
}

void TimelineListWidget::dragMoveEvent(QDragMoveEvent *event)
{
    return QListWidget::dragMoveEvent(event);
}

void TimelineListWidget::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
//    return QListWidget::mouseMoveEvent(event); //不返回给QListWidget
}

void TimelineListWidget::paintEvent(QPaintEvent *e)
{
    QListWidget::paintEvent(e);
    QScrollBar *bar = this->verticalScrollBar();
    bar->setGeometry(bar->x(), /*bar->y() + */m_scrollbartopdistance, bar->width(), this->height() - m_scrollbartopdistance - m_scrollbarbottomdistance);

    if (this->count() > 0) {
        int ccount = count();
        int blankHeight = 0; //add 3975
        for (int i = 0; i < ccount; i++) {
            QListWidgetItem *pItem = this->item(i);
            TimelineItem *pWidget = static_cast<TimelineItem *>(itemWidget(pItem));
            //add start 3975
            if (pWidget->m_type == "blank") {
                blankHeight = 47;
            }
            //add end 3975
            if (pWidget->m_type != "blank") { //add 3975
                // 50 windows title height
                if ((50 <= pWidget->y() && pWidget->y() <= pWidget->m_title->height() + 47) && (pWidget->y() > blankHeight)) { //edit 3975
#if 1
                    QListWidgetItem *pLastItem;
                    pLastItem = this->item(i - 1);
                    TimelineItem *pLastWidget = static_cast<TimelineItem *>(itemWidget(pLastItem));
                    emit sigMoveTime(pWidget->y() - pWidget->m_title->height() - 47, pLastWidget->m_sdate, pLastWidget->m_snum, pLastWidget->m_Chose->text());
#endif
                    pWidget->m_title->setVisible(true);
                    pWidget->m_date->setText(pWidget->m_sdate);
                    pWidget->m_num->setText(pWidget->m_snum);
                    pWidget->m_Chose->setVisible(true);

                    //            } else if ((pWidget->y() <= 0) && (pWidget->y() + pWidget->m_title->height() > 0)) {
                } else if ((pWidget->y() <= blankHeight + 47)) {  //edit 3975
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

bool TimelineListWidget::eventFilter(QObject *obj, QEvent *e)
{
    Q_UNUSED(obj)
    if (e->type() == QEvent::Wheel && QApplication::keyboardModifiers() == Qt::ControlModifier) {
        return true;
    }
    return false;
}
