#ifndef TIMELINEITEM_H
#define TIMELINEITEM_H

#include <QWidget>
#include <DLabel>
#include <QDebug>
#include <DCommandLinkButton>
DWIDGET_USE_NAMESPACE
class TimelineItem : public QWidget
{
    Q_OBJECT
public:
    explicit TimelineItem(QWidget *parent = nullptr);
    QWidget *m_title;
    DLabel *m_date;
    DLabel *m_num;
    QString m_sdate;
    QString m_snum;
    DCommandLinkButton *m_Chose;
signals:

public slots:
};

#endif // TIMELINEITEM_H
