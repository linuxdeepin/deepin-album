#ifndef LEFTLISTWIDGET_H
#define LEFTLISTWIDGET_H
#include <DListWidget>
#include "utils/baseutils.h"

DWIDGET_USE_NAMESPACE

class LeftListWidget : public DListWidget
{
    Q_OBJECT
public:
    LeftListWidget();
    void mousePressEvent(QMouseEvent *e) override;
    QStyleOptionViewItem viewOptions() const override;
    QModelIndex getModelIndex(QListWidgetItem *pItem);

protected:
    void dragMoveEvent(QDragMoveEvent *event) Q_DECL_OVERRIDE;
    void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;

    void dragEnterEvent(QDragEnterEvent *event) override;

signals:
    void signalDropEvent(QModelIndex index);
    void sigMousePressIsNoValid();
};

#endif  // LEFTLISTWIDGET_H
