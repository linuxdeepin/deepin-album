#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QWidget>
#include <DLabel>
#include <DSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <DFontSizeManager>

#include "application.h"
#include "controller/signalmanager.h"
#include "dbmanager/dbmanager.h"

DWIDGET_USE_NAMESPACE


class StatusBar : public DWidget
{
    Q_OBJECT

public:
    StatusBar();

public:
    void initUI();
    void initConnections();
    void onUpdateAllpicsNumLabel();
    void resizeEvent(QResizeEvent *e) override;

public:
    DLabel* m_pAllPicNumLabel;
    DSlider* m_pSlider;

    int m_allPicNum;
};



#endif // STATUSBAR_H

