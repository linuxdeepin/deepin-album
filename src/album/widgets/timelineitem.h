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
#ifndef TIMELINEITEM_H
#define TIMELINEITEM_H

#include <QWidget>
#include <DLabel>
#include <QDebug>
#include <DCommandLinkButton>
#include <QMouseEvent>
DWIDGET_USE_NAMESPACE
class TimelineItem : public QWidget
{
    Q_OBJECT
public:
    explicit TimelineItem(QWidget *parent = nullptr);
    QWidget *m_title = nullptr;
    DLabel *m_date = nullptr;
    DLabel *m_num = nullptr;
    QString m_sdate;
    QString m_snum;
    QString m_type;  //add 3975
    DCommandLinkButton *m_Chose = nullptr;
    void mousePressEvent(QMouseEvent *e) override;
signals:
    void sigMousePress();
public slots:
};

#endif // TIMELINEITEM_H
