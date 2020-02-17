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
