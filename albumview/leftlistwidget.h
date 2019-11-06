#ifndef LEFTLISTWIDGET_H
#define LEFTLISTWIDGET_H
#include <DListWidget>

DWIDGET_USE_NAMESPACE

class LeftListWidget :public DListWidget
{
    Q_OBJECT
public:
    LeftListWidget();
    void mousePressEvent(QMouseEvent* e) override;
};

#endif // LEFTLISTWIDGET_H
