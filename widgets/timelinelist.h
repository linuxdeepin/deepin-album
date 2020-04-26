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
//class TimelineListDelegate : public QStyledItemDelegate
//{
//    Q_OBJECT

//public:
//    explicit TimelineListDelegate(QObject *parent = nullptr);

//    void paint(QPainter *painter,
//               const QStyleOptionViewItem &option,
//               const QModelIndex &index) const Q_DECL_OVERRIDE;
//};
class TimelineList : public DListWidget
{
    Q_OBJECT
public:
    explicit TimelineList(QWidget *parent = nullptr);
    ~TimelineList()
    {

    }
    void addItemForWidget(QListWidgetItem *aitem);
protected:
//    void wheelEvent(QWheelEvent *event);
    void dragMoveEvent(QDragMoveEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *e) override;
    bool eventFilter(QObject *obj, QEvent *e) override;

signals:
    void sigNewTime(QString date, QString num, int index);
    void sigDelTime();
    void sigMoveTime(int y, QString date, QString num, QString choseText);

public slots:
private:
    bool has;
    QList<int> yList;
    int m_scrollbartopdistance;
    int m_scrollbarbottomdistance;
};

#endif // TIMELINELIST_H
