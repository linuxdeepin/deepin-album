#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QWidget>
#include <DLabel>
#include <DSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <DFontSizeManager>
#include <QStackedWidget>
#include <DSpinner>

#include "application.h"
#include "controller/signalmanager.h"
#include "dbmanager/dbmanager.h"
#include "widgets/loadingicon.h"
#include "widgets/thumbnaillistview.h"

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
    DLabel* m_pstacklabel;
    DWidget* m_pimporting;
    DLabel* TextLabel;
    QStringList imgpaths;
    QStackedWidget* m_pStackedWidget;
    DSpinner* loadingicon;

    int m_allPicNum;
    int interval;
private:
    int i = 0;


protected:
    void timerEvent(QTimerEvent *e) Q_DECL_OVERRIDE;

};



#endif // STATUSBAR_H

