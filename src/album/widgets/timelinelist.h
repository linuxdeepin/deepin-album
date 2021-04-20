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
#ifndef TIMELINELIST_H
#define TIMELINELIST_H

#include <QWidget>
#include <DListWidget>
#include <QLabel>
#include "timelineitem.h"
#include <QDebug>
#include "controller/signalmanager.h"
#include "application.h"

DWIDGET_USE_NAMESPACE

class TimelineListWidget : public DListWidget
{
    Q_OBJECT
public:
    explicit TimelineListWidget(QWidget *parent = nullptr);
    ~TimelineListWidget()
    {

    }
    void addItemForWidget(QListWidgetItem *aitem);

protected:
    void dragMoveEvent(QDragMoveEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *e) override;
    bool eventFilter(QObject *obj, QEvent *e) override;

signals:
    void sigNewTime(QString date, QString num, int index);
//    void sigDelTime();//未使用
    void sigMoveTime(int y, QString date, QString num, QString choseText);
    void sigNeedMoveScorll(int distence);

private slots:
    void onRangeChanged(int min, int max);

private:
    bool has;
    QList<int> yList;
    int m_scrollbartopdistance;
    int m_scrollbarbottomdistance;
    QPoint lastTouchBeginPos;
};

#endif // TIMELINELIST_H
