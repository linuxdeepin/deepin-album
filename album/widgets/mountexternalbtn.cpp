#include "mountexternalbtn.h"

MountExternalBtn::MountExternalBtn(DLabel *parent) : DLabel(parent)
{

}



void MountExternalBtn::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    emit sigMountExternalBtnClicked();
}
