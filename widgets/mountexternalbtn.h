#ifndef MOUNTEXTERNALBTN_H
#define MOUNTEXTERNALBTN_H

#include <QObject>
#include <DLabel>

DWIDGET_USE_NAMESPACE

class MountExternalBtn : public DLabel
{
    Q_OBJECT
public:
    MountExternalBtn(DLabel *parent = nullptr);

    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    void sigMountExternalBtnClicked();
};

#endif // MOUNTEXTERNALBTN_H
