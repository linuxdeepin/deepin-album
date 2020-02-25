#ifndef DTKTEST_H
#define DTKTEST_H
#include <DMainWindow>

DWIDGET_USE_NAMESPACE
//DGUI_USE_NAMESPACE

class DtkTest: public DMainWindow
{
    Q_OBJECT
public:
    DtkTest(QWidget *parent = nullptr);
private:
    void initUI();
};

#endif // DTKTEST_H
