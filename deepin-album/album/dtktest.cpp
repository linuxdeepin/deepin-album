#include "dtktest.h"

#include <DGroupBox>
#include <DCheckBox>
#include <DDialog>
#include <DTextEdit>
#include <DArrowRectangle>
#include <DFloatingWidget>
#include <DIconButton>
#include <QVBoxLayout>
#include <QRadioButton>
#include <QLabel>
#include <QMessageBox>
#include <DTitlebar>
#include <QAction>
#include <QDebug>
#include <DSuggestButton>
#include "imageengine/imageengineapi.h"
#include "thumbnail/thumbnaillistview.h"

DtkTest::DtkTest(QWidget *parent)
    : DMainWindow (parent)
{
    ImageEngineApi::instance(this);
    resize(1080, 720);
    initUI();
}

void DtkTest::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);
    auto centralWidget = new QWidget;
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);
    ThumbnailListView *m_pThumbnailListView = new ThumbnailListView(ThumbnailDelegate::AllPicViewType);
    mainLayout->addWidget(m_pThumbnailListView);
//    QStringList list;
//    list << QString("/home/archermind/Desktop/testpic");
//    m_pThumbnailListView->importFilesFromLocal(list);
    m_pThumbnailListView->loadFilesFromDB();
}
