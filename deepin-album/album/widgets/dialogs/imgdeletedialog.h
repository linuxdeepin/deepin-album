#ifndef IMGDELETEDIALOG_H
#define IMGDELETEDIALOG_H

#include <DDialog>
#include "controller/signalmanager.h"
#include "application.h"

DWIDGET_USE_NAMESPACE

class ImgDeleteDialog : public DDialog
{
    Q_OBJECT
public:
    explicit ImgDeleteDialog(DWidget *parent, int count, bool bdeleteallonlyone = false);
signals:
    void imgdelete();
};

#endif // IMGDELETEDIALOG_H
