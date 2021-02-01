#ifndef DFMDARROWLINEEXPAND_H
#define DFMDARROWLINEEXPAND_H

#include <DArrowLineDrawer>

DWIDGET_USE_NAMESPACE

class DFMDArrowLineExpand : public DArrowLineDrawer
{
public:
    DFMDArrowLineExpand();
protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // DFMDARROWLINEEXPAND_H
