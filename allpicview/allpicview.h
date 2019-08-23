#ifndef ALLPICVIEW_H
#define ALLPICVIEW_H
#include "application.h"
#include "utils/baseutils.h"
#include "controller/configsetter.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <DtkWidgets>
#include <QStandardPaths>
#include <QImageReader>

class AllPicView : public QWidget
{
public:
    AllPicView();

private:
    void initUI();
    void initConnections();

    void updateEmptyLayout();
    void updateNotEmptyLayout();

    void improtBtnClicked();

private:
    int m_allPicNum;

    DPushButton* m_pImportBtn;
    QVBoxLayout* m_pEmptyLayout;
    QVBoxLayout* m_pNotEmptyLayout;

};

#endif // ALLPICVIEW_H
