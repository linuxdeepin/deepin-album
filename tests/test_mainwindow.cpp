#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <QMap>
#include <DFileDialog>
#include <QTestEventList>
#include <QObject>
#include <QDialog>
#include <QStringList>
#include <DSearchEdit>
#include <DIconButton>
#include <QTimer>
#include <DMenu>

#define private public
#define protected public

#include "mainwindow.h"
#include "albumcreatedialog.h"
#include "test_qtestDefine.h"
#include "imginfodialog.h"
#include "mainwidget.h"
#include "commandline.h"
#include "imageview.h"
#include "imgdeletedialog.h"

#include <stub-tool/cpp-stub/stub.h>
#include <stub-tool/stub-ext/stubext.h>

// 3个button界面主视图切换显示
TEST(MainWindow, BtnGroupClick)
{
    TEST_CASE_NAME("load")
    QStringList list = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    if (list.size() > 0)
        QString picdir = list.at(0);
    else {
        list << "/usr/share/wallpapers/deepin/abc-123.jpg" << "desktop.jpg";
    }
    qDebug() << "MainWindow BtnGroupClick count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->onLoadingFinished();
    w->getButG();
    w->allPicBtnClicked();

    AllPicView *allpicview = w->m_pAllPicView;
    StatusBar *allbar = allpicview->m_pStatusBar;
    ImageEngineApi::instance()->ImportImagesFromFileList(list, "", allpicview, true);

    QTest::qWait(400);

    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(1));
    QTest::qWait(300);
    event.simulate(w->getButG()->button(2));
    QTest::qWait(300);
    event.simulate(w->getButG()->button(0));
    event.clear();
    QTest::qWait(300);

    allbar->m_pSlider->slider()->setValue(0);
    QTest::qWait(300);
    allbar->m_pSlider->slider()->setValue(1);
    QTest::qWait(300);
    allbar->m_pSlider->slider()->setValue(2);
    QTest::qWait(300);
    allbar->m_pSlider->slider()->setValue(3);
    QTest::qWait(300);
    allbar->m_pSlider->slider()->setValue(5);
    QTest::qWait(300);
    allbar->m_pSlider->slider()->setValue(6);
    QTest::qWait(300);
    allbar->m_pSlider->slider()->setValue(7);
    QTest::qWait(300);
    allbar->m_pSlider->slider()->setValue(8);
    QTest::qWait(300);
    allbar->m_pSlider->slider()->setValue(9);
    QTest::qWait(300);
    allbar->m_pSlider->slider()->setValue(4);
    QTest::qWait(300);

    CommandLine *commandline = w->m_commandLine;
    MainWidget *pmainwidget = nullptr;
    ViewPanel *viewpanel = nullptr;
    TTBContent *ttbc = nullptr;
    ImageView *imageview = nullptr;

    if (commandline)
        pmainwidget = commandline->findChild<MainWidget *>("MainWidget");
    if (pmainwidget)
        viewpanel = pmainwidget->m_viewPanel;

    QTest::qWait(200);
    QPoint p1(30, 100);
    event.addMouseMove(p1);
    event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    event.addMouseDClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    event.simulate(allpicview->m_pThumbnailListView->viewport());
    QTest::qWait(300);
    event.clear();
    //全屏
    event.addMouseDClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    if (viewpanel) {
        imageview = viewpanel->m_viewB;
        ttbc = viewpanel->m_ttbc;
    }
    if (imageview)
        event.simulate(imageview->viewport());
    QTest::qWait(1000);
    event.clear();

    if (ttbc) {
        DIconButton *adapt = ttbc->findChild<DIconButton *>("TtbcontentAdaptImgButton");
        event.addMouseClick(Qt::MouseButton::LeftButton);
        if (adapt)
            event.simulate(adapt);
        QTest::qWait(500);

        DIconButton *adaptscreen = ttbc->findChild<DIconButton *>("TtbcontentAdaptScreenButton");
        if (adaptscreen)
            event.simulate(adaptscreen);
        QTest::qWait(500);

        DIconButton *next = ttbc->findChild<DIconButton *>("TtbcontentNextButton");
        if (next)
            event.simulate(next);
        QTest::qWait(500);

        DIconButton *pre = ttbc->findChild<DIconButton *>("TtbcontentPreButton");
        if (pre)
            event.simulate(pre);
        QTest::qWait(500);

        DIconButton *collect = ttbc->findChild<DIconButton *>("TtbcontentCollectButton");
        if (collect)
            event.simulate(collect);
        QTest::qWait(500);

        DIconButton *rotateR = ttbc->findChild<DIconButton *>("TtbcontentRotateRightButton");
        if (rotateR)
            event.simulate(rotateR);
        QTest::qWait(1500);

        DIconButton *rotateL = ttbc->findChild<DIconButton *>("TtbcontentRotateLeftButton");
        if (rotateL)
            event.simulate(rotateL);
        QTest::qWait(1500);

        DIconButton *back = ttbc->findChild<DIconButton *>("TtbcontentBackButton");
        if (back)
            event.simulate(back);
        QTest::qWait(500);
        event.clear();

        event.addMouseMove(p1);
        event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
        event.simulate(allpicview->m_pThumbnailListView->viewport());
        event.clear();
        QTest::qWait(500);

        //TODO 删除
    }
    //------右键菜单start---------
    QContextMenuEvent menuEvent(QContextMenuEvent::Mouse, p1);
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent);
    QTest::qWait(300);

    QTestEventList e;

    //查看
    DMenu *menuWidget = static_cast<DMenu *>(qApp->activePopupWidget());
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(1000);
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    QTest::qWait(300);

    //全屏
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(1000);
    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    QTest::qWait(300);

    //幻灯片
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(300);

    SlideShowPanel *slideshowpanel = w->m_slidePanel;
    if (slideshowpanel) {
        SlideShowBottomBar *sliderbar = slideshowpanel->slideshowbottombar;
        if (sliderbar) {
            e.addMouseClick(Qt::MouseButton::LeftButton);
            DIconButton *preButton = sliderbar->findChild<DIconButton *>("SliderPreButton");
            DIconButton *NextButton = sliderbar->findChild<DIconButton *>("SliderNextButton");
            DIconButton *Play_PauseButton = sliderbar->findChild<DIconButton *>("SliderPlayPauseButton");
            DIconButton *ExitButton = sliderbar->findChild<DIconButton *>("SliderExitButton");
            if (NextButton) {
                e.simulate(NextButton);
                QTest::qWait(500);
            }
            if (preButton) {
                e.simulate(preButton);
                QTest::qWait(500);
            }
            if (Play_PauseButton) {
                e.simulate(Play_PauseButton);
                QTest::qWait(500);
                e.simulate(Play_PauseButton);
                QTest::qWait(500);
            }
            if (ExitButton) {
                e.simulate(ExitButton);
                QTest::qWait(300);
            }
            e.clear();
        }
    }

    //复制7
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(300);

    //删除8-d
//    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent);
//    QTest::qWait(300);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
//    e.simulate(menuWidget);

//    e.clear();
//    QTest::qWait(1000);

    e.addMouseMove(p1);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(allpicview->m_pThumbnailListView->viewport());
    e.clear();
    QTest::qWait(200);

    //收藏9
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(100);

    //顺时针10
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(1500);

    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(allpicview->m_pThumbnailListView->viewport());
    e.clear();
    QTest::qWait(200);

    //逆时针11
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent);;
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(1500);

    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(allpicview->m_pThumbnailListView->viewport());
    e.clear();
    QTest::qWait(200);

    //设为壁纸12
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(100);

    //文管显示13
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(300);
    w->raise();
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.simulate(allpicview->m_pThumbnailListView->viewport());
    e.clear();
    QTest::qWait(200);

    //照片信息14
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget);
    e.clear();
    QTest::qWait(500);

    QList<QWidget *> lis =  dApp->getDAppNew()->topLevelWidgets();
    auto iter = lis.cbegin();
    while (iter != lis.cend()) {
        if ((*iter)->objectName() == "ImgInfoDialog") {
            (*iter)->hide();
            break;
        }
        ++iter;
    }
    //------右键菜单end---------

    //多选
    QPoint p2(280, 100);
    QContextMenuEvent menuEvent2(QContextMenuEvent::Mouse, p2);

    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
    e.addMouseMove(p2);
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::ControlModifier, p2, 50);
    e.simulate(allpicview->m_pThumbnailListView->viewport());
    e.clear();
    QTest::qWait(300);

    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent2);
    QTest::qWait(300);
    DMenu *menuWidget2 = static_cast<DMenu *>(qApp->activePopupWidget());

    //1 2 4 5 6
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
//    e.simulate(menuWidget2);
//    e.clear();
//    QTest::qWait(500);
    //幻灯片
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent2);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget2);
    e.clear();
    QTest::qWait(1000);

    e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
    e.simulate(imageview->viewport());
    e.clear();
    //导出-d
//    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent2);
//    QTest::qWait(300);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
//    e.simulate(menuWidget2);
//    e.clear();
//    QTest::qWait(500);
    //复制
    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent2);
    QTest::qWait(300);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget2);
    e.clear();
    QTest::qWait(500);
    //删除-d
//    qApp->sendEvent(allpicview->m_pThumbnailListView->viewport(), &menuEvent2);
//    QTest::qWait(300);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
//    e.simulate(menuWidget2);
//    e.clear();
//    QTest::qWait(500);

    w->resize(980, 600);
}

TEST(MainWindow, timelineview)
{
    TEST_CASE_NAME("timelineview")
    MainWindow *w = dApp->getMainWindow();
    TimeLineView *timelineview = w->m_pTimeLineView;
    w->timeLineBtnClicked();

    QTestEventList e;
    e.addMouseClick(Qt::MouseButton::LeftButton);
    e.simulate(w->getButG()->button(1));
    e.clear();

    CommandLine *commandline = w->m_commandLine;
    ImageView *imageview = commandline->findChild<MainWidget *>("MainWidget")->m_viewPanel->m_viewB;

    QPoint p3(100, 50);
    QContextMenuEvent menuEvent2(QContextMenuEvent::Mouse, p3);

    if (timelineview) {
        QList<DCommandLinkButton *> btns = timelineview->m_allChoseButton;
        QTestEventList e1;
        e1.addMouseClick(Qt::MouseButton::LeftButton);
        for (int i = 0; i < btns.size(); i++) {
            e1.simulate(btns.at(i));
            QTest::qWait(200);
        }
        e1.clear();

        e1.addMouseMove(p3);
        e1.simulate(timelineview->m_allThumbnailListView[0]->viewport());
        e1.clear();
        QTest::qWait(300);

        qApp->sendEvent(timelineview->m_allThumbnailListView[0]->viewport(), &menuEvent2);
        QTest::qWait(300);
        DMenu *menuWidget3 = static_cast<DMenu *>(qApp->activePopupWidget());

//        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
//        e.simulate(menuWidget3);
//        e.clear();
//        QTest::qWait(500);
        //幻灯片
        qApp->sendEvent(timelineview->m_allThumbnailListView[0]->viewport(), &menuEvent2);
        QTest::qWait(300);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
        e.simulate(menuWidget3);
        e.clear();
        QTest::qWait(1000);


        commandline->findChild<MainWidget *>("MainWidget")->m_extensionPanel->setContent(nullptr);
        commandline->findChild<MainWidget *>("MainWidget")->m_extensionPanel->updateRectWithContent();
        e.addMouseMove(commandline->findChild<MainWidget *>("MainWidget")->m_extensionPanel->pos());
        e.simulate(commandline->findChild<MainWidget *>("MainWidget")->m_extensionPanel);
        commandline->findChild<MainWidget *>("MainWidget")->m_extensionPanel->update();
        e.clear();
        QTest::qWait(300);
        commandline->findChild<MainWidget *>("MainWidget")->onBackToMainPanel();
        commandline->findChild<MainWidget *>("MainWidget")->onActiveWindow();
        commandline->findChild<MainWidget *>("MainWidget")->onShowInFileManager("");
        commandline->findChild<MainWidget *>("MainWidget")->onMouseMove();
        commandline->findChild<MainWidget *>("MainWidget")->onShowFullScreen();

        e.addKeyClick(Qt::Key_Escape, Qt::NoModifier, 50);
        e.simulate(imageview->viewport());
        e.clear();
        //导出-d
//        qApp->sendEvent(timelineview->m_allThumbnailListView[0]->viewport(), &menuEvent2);
//        QTest::qWait(300);
//        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
//        e.simulate(menuWidget3);
//        e.clear();
//        QTest::qWait(500);
        //复制
        qApp->sendEvent(timelineview->m_allThumbnailListView[0]->viewport(), &menuEvent2);
        QTest::qWait(300);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
        e.simulate(menuWidget3);
        e.clear();
        QTest::qWait(500);
        //删除-d
//        qApp->sendEvent(timelineview->m_allThumbnailListView[0]->viewport(), &menuEvent2);
//        QTest::qWait(300);
//        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//        e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
//        e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
//        e.simulate(menuWidget3);
//        e.clear();
//        QTest::qWait(500);
    }
    StatusBar *bar1 = timelineview->m_pStatusBar;
    bar1->m_pSlider->slider()->setValue(0);
    QTest::qWait(300);
    bar1->m_pSlider->slider()->setValue(1);
    QTest::qWait(300);
    bar1->m_pSlider->slider()->setValue(2);
    QTest::qWait(300);
    bar1->m_pSlider->slider()->setValue(3);
    QTest::qWait(300);
    bar1->m_pSlider->slider()->setValue(5);
    QTest::qWait(300);
    bar1->m_pSlider->slider()->setValue(6);
    QTest::qWait(300);
    bar1->m_pSlider->slider()->setValue(7);
    QTest::qWait(300);
    bar1->m_pSlider->slider()->setValue(8);
    QTest::qWait(300);
    bar1->m_pSlider->slider()->setValue(9);
    QTest::qWait(300);
    bar1->m_pSlider->slider()->setValue(4);
    QTest::qWait(300);

    AlbumView *albumview = w->m_pAlbumview;
    w->albumBtnClicked();
    e.simulate(w->getButG()->button(2));
    e.clear();

    StatusBar *bar = albumview->m_pStatusBar;
    bar->m_pSlider->slider()->setValue(0);
    QTest::qWait(300);
    bar->m_pSlider->slider()->setValue(1);
    QTest::qWait(300);
    bar->m_pSlider->slider()->setValue(2);
    QTest::qWait(300);
    bar->m_pSlider->slider()->setValue(3);
    QTest::qWait(300);
    bar->m_pSlider->slider()->setValue(5);
    QTest::qWait(300);
    bar->m_pSlider->slider()->setValue(6);
    QTest::qWait(300);
    bar->m_pSlider->slider()->setValue(7);
    QTest::qWait(300);
    bar->m_pSlider->slider()->setValue(8);
    QTest::qWait(300);
    bar->m_pSlider->slider()->setValue(9);
    QTest::qWait(300);
    bar->m_pSlider->slider()->setValue(4);
    QTest::qWait(300);

//    QPoint p1(30, 102); //已导入
//    QPoint p2(30, 142);
//    QPoint p3(30, 182);
//    QPoint p4(260, 230);//已导入图

//    ThumbnailListView *cou = albumview->m_pRightThumbnailList;
//    ThumbnailListView *tra = albumview->m_pRightTrashThumbnailList;
//    ThumbnailListView *fav = albumview->m_pRightFavoriteThumbnailList;
//    DWidget *ImportTimeLineWidget = albumview->pImportTimeLineWidget;
//    ImportTimeLineView *im = albumview->m_pImpTimeLineWidget;
//    e.clear();
//    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p1, 50);
//    e.simulate(albumview);
//    e.clear();

//    e.addMouseMove(p4);
//    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p4, 50);
//    e.addMouseDClick(Qt::MouseButton::LeftButton, Qt::NoModifier, p4, 50);
//    e.simulate(ImportTimeLineWidget);
//    QTest::qWait(30000);
//    e.clear();
}

// 从菜单创建相册
TEST(MainWindow, createalbumFromTitlebarMenu)
{
    TEST_CASE_NAME("createalbumFromTitlebarMenu")
    qDebug() << "MainWindow createalbumFromTitlebarMenu count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);
    QList<QAction *> actions = w->actions();
    foreach (auto act, actions) {
        if (act->text() == QObject::tr("New album")) {
            act->trigger();
            break;
        }
    }
    QTest::qWait(500);
    QList<QWidget *> widgets = w->findChildren<QWidget *>();
    foreach (auto widget, widgets) {
        if (!strcmp(widget->metaObject()->className(), "AlbumCreateDialog")) {
            AlbumCreateDialog *tempDlg = dynamic_cast<AlbumCreateDialog *>(widget);
            tempDlg->getEdit()->setText("TestAlbum");
            emit tempDlg->buttonClicked(1, "");
            break;
        }
    }
    w->albumBtnClicked();
    QTest::qWait(500);
}

TEST(MainWindow, addpictoalbum)
{
    TEST_CASE_NAME("addpictoalbum")
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QPoint p1(100, 100);
    QTestEventList e;
    e.addMouseMove(p1);
    QContextMenuEvent menuEvent(QContextMenuEvent::Mouse, p1);
    qApp->sendEvent(w->m_pAllPicView->m_pThumbnailListView->viewport(), &menuEvent);
    e.clear();
    QTest::qWait(300);

    DMenu *menuWidget = static_cast<DMenu *>(qApp->activePopupWidget());
    e.addMouseMove(p1);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    QTest::qWait(300);
    e.simulate(menuWidget);
    e.clear();

    DMenu *menuWidget2 = static_cast<DMenu *>(qApp->activePopupWidget());
    e.addKeyClick(Qt::Key_Tab, Qt::NoModifier, 50);
    QTest::qWait(100);
    e.addKeyClick(Qt::Key_Enter, Qt::NoModifier, 50);
    e.simulate(menuWidget2);
    e.clear();
    QTest::qWait(1000);
}

TEST(MainWindow, albumview)
{
    TEST_CASE_NAME("albumview")
    MainWindow *w = dApp->getMainWindow();
//    AlbumView *albumview = w->m_pAlbumview;

    w->albumBtnClicked();

    QTestEventList e;
    e.addMouseClick(Qt::MouseButton::LeftButton);
    e.simulate(w->getButG()->button(2));
    e.clear();
//    CommandLine *commandline = w->m_commandLine;
//    ImageView *imageview = commandline->findChild<MainWidget *>("MainWidget")->m_viewPanel->m_viewB;

}

// 从菜单导入照片
TEST(MainWindow, ImportPhotosFromTitlebarMenu)
{
    TEST_CASE_NAME("ImportPhotosFromTitlebarMenu")
    qDebug() << "MainWindow ImportPhotosFromTitlebarMenu count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    int (*dlgexec)() = []() {return 1;};
    typedef int (*fptr)(QDialog *);
    fptr fptrexec = (fptr)(&QDialog::exec);   //obtaining an address
    Stub stub;
    stub.set(fptrexec, dlgexec);

    stub_ext::StubExt stu;
    stu.set_lamda(ADDR(DFileDialog, selectedFiles), []() {
        QStringList filelist;
        filelist << ":/2e5y8y.jpg" << ":/2ejqyx.jpg" << ":/2k9o1m.png";
        return filelist;
    });
    QList<QAction *> actions = w->actions();
    foreach (auto act, actions) {
        if (act->text() == QObject::tr("Import photos")) {
            act->trigger();
            break;
        }
    }
    QTest::qWait(500);
}


TEST(MainWindow, setWaitDialogColor_test)
{
    TEST_CASE_NAME("setWaitDialogColor_test")
    qDebug() << "MainWindow setWaitDialogColor_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    stub_ext::StubExt stu;
    stu.set_lamda(ADDR(DGuiApplicationHelper, themeType), []() {
        return DGuiApplicationHelper::DarkType;
    });

    w->setWaitDialogColor();
    QTest::qWait(500);
    stu.set_lamda(ADDR(DGuiApplicationHelper, themeType), []() {
        return DGuiApplicationHelper::LightType;
    });
    w->setWaitDialogColor();
    QTest::qWait(500);
}

TEST(MainWindow, onShowImageInfo_test)
{
    TEST_CASE_NAME("onShowImageInfo_test")
    qDebug() << "MainWindow onShowImageInfo_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    w->onShowImageInfo(":/2e5y8y.jpg");
    QTest::qWait(500);
    w->onShowImageInfo(":/lq6rmy.png");
    QTest::qWait(500);
}

TEST(MainWindow, loadZoomRatio_test)
{
    TEST_CASE_NAME("loadZoomRatio_test")
    qDebug() << "MainWindow loadZoomRatio_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    w->loadZoomRatio();
    QTest::qWait(500);
}

TEST(MainWindow, createShorcutJson_test)
{
    TEST_CASE_NAME("createShorcutJson_test")
    qDebug() << "MainWindow createShorcutJson_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    w->createShorcutJson();
    QTest::qWait(500);
}

TEST(MainWindow, thumbnailZoomIn_test)
{
    TEST_CASE_NAME("thumbnailZoomIn_test")
    qDebug() << "MainWindow thumbnailZoomIn_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    w->thumbnailZoomIn();
    QTest::qWait(500);
}

TEST(MainWindow, thumbnailZoomOut_test)
{
    TEST_CASE_NAME("thumbnailZoomOut_test")
    qDebug() << "MainWindow thumbnailZoomOut_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    w->thumbnailZoomOut();
    QTest::qWait(500);
}

TEST(MainWindow, saveWindowState_test)
{
    TEST_CASE_NAME("saveWindowState_test")
    qDebug() << "MainWindow saveWindowState_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    w->saveWindowState();
    QTest::qWait(500);
}

TEST(MainWindow, compareVersion_test)
{
    TEST_CASE_NAME("compareVersion_test")
    qDebug() << "MainWindow compareVersion_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    w->compareVersion();
    QTest::qWait(500);
}

TEST(MainWindow, onSearchEditFinished_test)
{
    TEST_CASE_NAME("onSearchEditFinished_test")
    qDebug() << "MainWindow onSearchEditFinished_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);
    w->onSearchEditFinished();
    w->timeLineBtnClicked();
    QTest::qWait(500);
    w->onSearchEditFinished();
    w->albumBtnClicked();
    QTest::qWait(500);
    w->onSearchEditFinished();

    QTest::qWait(500);
}

TEST(MainWindow, onNewAPPOpen_test)
{
    TEST_CASE_NAME("onNewAPPOpen_test")
    qDebug() << "MainWindow onNewAPPOpen_test count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);

    stub_ext::StubExt stu;
    stu.set_lamda(ADDR(DSearchEdit, text), []() {
        return "hello";
    });

    w->timeLineBtnClicked();
    w->albumBtnClicked();
    w->allPicBtnClicked();

    QStringList filelist;
    filelist << "noid" << ":/2e5y8y.jpg" << ":/2ejqyx.jpg" << ":/2k9o1m.png";
    w->onNewAPPOpen(1, filelist);
    QTest::qWait(500);
    w->onSearchEditFinished();
    QTest::qWait(500);
    emit dApp->signalM->SearchEditClear();
    emit dApp->signalM->ImportFailed();
    emit dApp->signalM->ImportSomeFailed(2, 1);
    QTestEventList e;
    e.addKeyPress(Qt::Key_Slash, Qt::ControlModifier | Qt::ShiftModifier, 10);
    e.simulate(w);

    QPoint p = w->pos();
    int wi = w->width();
    int h = w->height();
    e.addMouseMove(p + QPoint(wi / 2, h / 2));
    e.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier);
    e.simulate(w);
    e.clear();
}
