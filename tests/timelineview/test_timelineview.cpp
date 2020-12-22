#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "mainwindow.h"
#include "timelineview.h"
#include "../test_qtestDefine.h"

#include "utils/baseutils.h"
#include "imageengineapi.h"


#include <QTestEventList>
#include <QScrollBar>

TEST(TimeLineView, T1)
{
    qDebug() << "TimeLineView T1 count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->showEvent(nullptr);
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(1));
    event.clear();
    QTest::qWait(500);
    TimeLineView *t = w->m_pTimeLineView;
    t->m_pStatusBar->m_pSlider->setValue(1);
    t->getIBaseHeight();
    t->m_pStatusBar->m_pSlider->setValue(2);
    t->getIBaseHeight();
    t->m_pStatusBar->m_pSlider->setValue(3);
    t->getIBaseHeight();
    t->m_pStatusBar->m_pSlider->setValue(4);
    t->getIBaseHeight();
    t->m_pStatusBar->m_pSlider->setValue(5);
    t->getIBaseHeight();
    t->m_pStatusBar->m_pSlider->setValue(6);
    t->getIBaseHeight();
    t->m_pStatusBar->m_pSlider->setValue(7);
    t->getIBaseHeight();
    t->m_pStatusBar->m_pSlider->setValue(8);
    t->getIBaseHeight();
    t->m_pStatusBar->m_pSlider->setValue(9);
    t->getIBaseHeight();
    t->m_pStatusBar->m_pSlider->setValue(10);
    t->getIBaseHeight();
    t->onFinishLoad();

    dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Light);
    QTest::qWait(500);
    dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Dark);
    QStringList list;
    list << testPath_test + "2k9o1m.png";
    t->updataLayout(list);
    t->on_DCommandLinkButton();
    t->on_GetSelectedPaths(&list);
    t->on_KeyEvent(Qt::Key_PageDown);
    t->on_KeyEvent(Qt::Key_PageUp);
//    t->updateChoseText();

    QString jpgItemPath = testPath_test + "/2k9o1m.png";
    QString text = "xxxxxxxxxxxxxx";
    QIcon icon = QIcon(":/resources/images/other/deepin-album.svg");
    QIcon icon_hover = QIcon(":/resources/images/other/deepin-album.svg");
    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << text << icon << icon_hover;
    QMimeData mimedata;
    mimedata.setData(QStringLiteral("TestListView/text-icon-icon_hover"), itemData);
    QList<QUrl> li;
    li.append(QUrl::fromLocalFile(jpgItemPath));
    mimedata.setUrls(li);

    QPoint pos1 = t->pos();
    QDropEvent ed(pos1, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
    dApp->getDAppNew()->sendEvent(t, &ed);
    QTest::qWait(100);

    QTestEventList e;
    e.addMousePress(Qt::MouseButton::LeftButton, Qt::NoModifier, pos1, 10);
    e.simulate(t);
    e.clear();

    ASSERT_TRUE(t->getIBaseHeight());
}

TEST(TimeLineView, dragPhotoToAnAlbum)
{
    qDebug() << "TimeLineView dragPhotoToAnAlbum count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();

    w->timeLineBtnClicked();
    QTest::qWait(100);
    TimeLineView *a = w->m_pTimeLineView;
    QList<QWidget *> widgets = a->findChildren<QWidget *>("");
    for (int index = 0; index < widgets.count(); index++) {
        if (!strcmp(widgets.at(index)->metaObject()->className(), ("TimelineList"))) {
            TimelineList *li = static_cast<TimelineList *>(widgets.at(index));
            if (li) {
                QScrollBar *bar = li->verticalScrollBar();
                bar->setRange(1, 100);
                QPoint pos1 = li->rect().center();
                QString text = "xxxxxxxxxxxxxx";
                QIcon icon = QIcon(":/resources/images/other/deepin-album.svg");
                QIcon icon_hover = QIcon(":/resources/images/other/deepin-album.svg");
                QByteArray itemData;
                QDataStream dataStream(&itemData, QIODevice::WriteOnly);
                dataStream << text << icon << icon_hover;
                QMimeData mimedata;
                mimedata.setData(QStringLiteral("TestListView/text-icon-icon_hover"), itemData);
                QDragMoveEvent eMove(pos1, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
                dApp->getDAppNew()->sendEvent(a, &eMove);
                QTest::qWait(200);

                QTestEventList e;
                e.addMouseMove(pos1);
                e.addMouseMove(pos1 + QPoint(1, 1));
                e.simulate(li);
            }
        }
        if (!strcmp(widgets.at(index)->metaObject()->className(), ("ThumbnailListView"))) {
            QString jpgItemPath = testPath_test + "/2k9o1m.png";
            QString text = "xxxxxxxxxxxxxx";
            QIcon icon = QIcon(":/resources/images/other/deepin-album.svg");
            QIcon icon_hover = QIcon(":/resources/images/other/deepin-album.svg");
            QByteArray itemData;
            QDataStream dataStream(&itemData, QIODevice::WriteOnly);
            dataStream << text << icon << icon_hover;
            QMimeData mimedata;
            mimedata.setData(QStringLiteral("TestListView/text-icon-icon_hover"), itemData);
            QList<QUrl> li;
            li.append(QUrl::fromLocalFile(jpgItemPath));
            mimedata.setUrls(li);
            QTest::qWait(200);

            const QPoint pos1 = a->rect().center();
            QDragEnterEvent eEnter(pos1, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
            dApp->getDAppNew()->sendEvent(a, &eEnter);
            QTest::qWait(200);

            QDragMoveEvent eMove(pos1, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
            dApp->getDAppNew()->sendEvent(a, &eMove);
            QTest::qWait(200);

            QDropEvent e(pos1, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
//            dApp->getDAppNew()->sendEvent(a, &e);
            QTest::qWait(200);

            dApp->getDAppNew()->sendEvent(a, &eEnter);
            QTest::qWait(200);

            QDragLeaveEvent eLeave;
            dApp->getDAppNew()->sendEvent(a, &eLeave);
            QTest::qWait(500);
            break;
        }
    }
}
TEST(TimeLineView, SelectTimeLinesBtn)
{
    qDebug() << "TimeLineView SelectTimeLinesBtn count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    QTestEventList event;
    w->timeLineBtnClicked();
    QTest::qWait(500);
    TimeLineView *t = w->m_pTimeLineView;
    QList<QWidget *> widgets = t->findChildren<QWidget *>("");
    foreach (auto widget, widgets) {
        if (widget->objectName() == "TimeLineChooseButton") {
            DCommandLinkButton *temp = static_cast<DCommandLinkButton *>(widget);
            QPoint pos(10, 10);
            event.addMouseMove(pos);
            event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pos);
            event.simulate(temp);
            event.clear();
            QTest::qWait(500);

            event.addMouseMove(pos);
            event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pos);
            event.simulate(temp);
            event.clear();
            QTest::qWait(500);
            break;
        }
    }
}

TEST(TimeLineView, importImages)
{
//    qDebug() << "TimeLineView SelectTimeLinesBtn count = " << count_testDefine++;
//    MainWindow *w = dApp->getMainWindow();
//    QTestEventList event;
//    w->timeLineBtnClicked();
//    QTest::qWait(500);
//    TimeLineView *t = w->m_pTimeLineView;
//    QString testPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + "test";
//    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "", t, true);
//    QTest::qWait(500);
//    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "", t, true);
//    QTest::qWait(500);
}

TEST(TimeLineView, selectBtn)
{
    qDebug() << "TimeLineView selectBtn count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->timeLineBtnClicked();
    QTest::qWait(500);

    TimeLineView *t = w->m_pTimeLineView;
    QList<QWidget *> widgets = t->findChildren<QWidget *>("");
    for (int i = 0; i < widgets.count(); i++) {
        if (!strcmp(widgets.at(i)->metaObject()->className(), ("Dtk::Widget::DCommandLinkButton"))) {
            DCommandLinkButton *pDCmdBtnSelect = dynamic_cast<DCommandLinkButton *>(widgets.at(i));
            if (pDCmdBtnSelect->text() == QObject::tr("Select")) {
                QTestEventList event;
                event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, QPoint(10, 10));
                event.simulate(widgets.at(i));
                event.clear();
                break;
            }
        }
    }
    QTest::qWait(500);
    for (int i = 0; i < widgets.count(); i++) {
        if (!strcmp(widgets.at(i)->metaObject()->className(), ("Dtk::Widget::DCommandLinkButton"))) {
            DCommandLinkButton *pDCmdBtnUnselect = dynamic_cast<DCommandLinkButton *>(widgets.at(i));
            if (pDCmdBtnUnselect->text() == QObject::tr("Unselect")) {
                QTestEventList event;
                event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, QPoint(10, 10));
                event.simulate(widgets.at(i));
                event.clear();
                break;
            }
        }
    }
    QTest::qWait(500);
}

//TEST(TimeLineView, keyDeletePhotos)//roc
//{
//    qDebug() << "TimeLineView selectBtn count = " << count_testDefine++;
//    MainWindow *w = dApp->getMainWindow();
//    w->timeLineBtnClicked();
//    QTest::qWait(500);
//    TimeLineView *t = w->m_pTimeLineView;
//    t->setFocus();
//    QList<QWidget *> widgets = t->findChildren<QWidget *>("");
//    for (int i = 0; i < widgets.count(); i++) {
//        if (!strcmp(widgets.at(i)->metaObject()->className(), ("Dtk::Widget::DCommandLinkButton"))) {
//            DCommandLinkButton *pDCmdBtnSelect = dynamic_cast<DCommandLinkButton *>(widgets.at(i));
//            if (pDCmdBtnSelect->text() == QObject::tr("Select")) {
//                QTestEventList event;
//                event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, QPoint(10, 10));
//                event.simulate(widgets.at(i));
//                event.clear();
//                QTest::qWait(100);
//                dApp->signalM->sigShortcutKeyDelete();
//                break;
//            }
//        }
//    }
//    QTest::qWait(500);
//    QString testPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + "test";
//    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "", t, true);
//    QTest::qWait(500);
//}

TEST(TimeLineView, changeTheme)
{
    qDebug() << "TimeLineView changeTheme count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->timeLineBtnClicked();
    QTest::qWait(500);
    TimeLineView *t = w->m_pTimeLineView;
    t->setFocus();
    dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Dark);
    QTest::qWait(500);
}

TEST(TimeLineView, changSliderValue)
{
    qDebug() << "TimeLineView changSliderValue count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->timeLineBtnClicked();
    TimeLineView *t = w->m_pTimeLineView;
    int width = t->m_pStatusBar->m_pSlider->slider()->width();
    QPoint pos(width / 10 * t->m_pStatusBar->m_pSlider->slider()->sliderPosition(), 10);
    QTestEventList event;
    event.addMousePress(Qt::LeftButton, Qt::NoModifier, pos);
    pos = pos - QPoint(200, 0);
    event.addMouseMove(pos);
    QTest::qWait(500);
    event.simulate(t->m_pStatusBar->m_pSlider->slider());
    pos = QPoint(width / 10 * t->m_pStatusBar->m_pSlider->slider()->sliderPosition(), 10);
    for (int i = 0; i < 5; i++) {
        event.addMousePress(Qt::LeftButton, Qt::NoModifier, pos);
        pos = pos + QPoint(20, 0);
        event.addMouseMove(pos);
        event.simulate(t->m_pStatusBar->m_pSlider->slider());
        event.clear();
        QTest::qWait(400);
    }
    pos = QPoint(width / 10 * t->m_pStatusBar->m_pSlider->slider()->sliderPosition(), 10);
    for (int i = 0; i < 5; i++) {
        event.addMousePress(Qt::LeftButton, Qt::NoModifier, pos);
        pos = pos - QPoint(20, 0);
        event.addMouseMove(pos);
        event.simulate(t->m_pStatusBar->m_pSlider->slider());
        event.clear();
        QTest::qWait(400);
    }
}



