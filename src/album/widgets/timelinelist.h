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
    void mouseMoveEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *e) override;
    bool eventFilter(QObject *obj, QEvent *e) override;

signals:
    void sigNewTime(QString date, QString num, int index);
//    void sigDelTime();//未使用
    void sigMoveTime(int y, QString date, QString num, QString choseText);

private slots:
    void onRangeChanged(int min, int max);

private:
    bool has;
    QList<int> yList;
    int m_scrollbartopdistance;
    int m_scrollbarbottomdistance;
};

#endif // TIMELINELIST_H
