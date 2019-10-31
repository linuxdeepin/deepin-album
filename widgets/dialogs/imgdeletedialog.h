#ifndef IMGDELETEDIALOG_H
#define IMGDELETEDIALOG_H

#include <DDialog>

DWIDGET_USE_NAMESPACE

class ImgDeleteDialog : public DDialog
{
    Q_OBJECT
public:
    explicit ImgDeleteDialog(int count);
signals:
    void imgdelete();
};

#endif // IMGDELETEDIALOG_H
