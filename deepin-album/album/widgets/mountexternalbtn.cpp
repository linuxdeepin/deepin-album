#include "mountexternalbtn.h"
#include <QMouseEvent>

MountExternalBtn::MountExternalBtn(DLabel *parent) : DLabel(parent)
{

}



void MountExternalBtn::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    emit sigMountExternalBtnClicked();
    event->accept();
}
