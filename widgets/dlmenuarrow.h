#ifndef DLMENUARROW_H
#define DLMENUARROW_H

#include <QApplication>
#include <QWidget>
#include <DPushButton>
#include <QEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QDebug>

DWIDGET_USE_NAMESPACE

enum ActId
{
    IdArrowUp,
    IdArrowDown
};

#define OFFSET 6

class DLMenuArrow : public DPushButton
{
    Q_OBJECT
public:
    explicit DLMenuArrow(QWidget *parent = nullptr);

    void setWgt(QWidget* wgt, ActId id);
    void setMouseEnter(bool bFlag);

protected:
    virtual void paintEvent(QPaintEvent *ev);
    virtual void enterEvent(QEvent *ev);
    virtual void leaveEvent(QEvent *ev);

signals:
    void sigMouseEnter();

public slots:
    void onClicked();

private:
    QWidget *m_pParentWgt;
    ActId m_actId;
    QPoint m_mousePos;
    bool m_bMouseEnter = false;

};

#endif // DLMENUARROW_H
