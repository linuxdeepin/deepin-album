#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "mainwindow.h"
#include "timelineview.h"
#include "../test_qtestDefine.h"

#include "utils/baseutils.h"
#include "imageengineapi.h"


#include <QTestEventList>

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

    dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Light);
    QTest::qWait(500);
    dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Dark);
    QTest::qWait(500);

    ASSERT_TRUE(t->getIBaseHeight());
}
TEST(TimeLineView, dragPhotoToAnAlbum)
{
    qDebug() << "AlbumView dragPhotoToAnAlbum count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();

    w->timeLineBtnClicked();
    QTest::qWait(100);
    TimeLineView *a = w->m_pTimeLineView;
    QList<QWidget *> widgets = a->findChildren<QWidget *>("");
    for (int index = 0; index < widgets.count(); index++) {
        if (!strcmp(widgets.at(index)->metaObject()->className(),("ThumbnailListView"))) {
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
            dApp->getDAppNew()->sendEvent(a, &e);
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
            DCommandLinkButton *temp = static_cast<DCommandLinkButton*>(widget);
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
    qDebug() << "TimeLineView SelectTimeLinesBtn count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    QTestEventList event;
    w->timeLineBtnClicked();
    QTest::qWait(500);
    TimeLineView *t = w->m_pTimeLineView;
    QString testPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + "test";
    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "", t, true);
    QTest::qWait(500);
    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "", t, true);
    QTest::qWait(500);
}

TEST(TimeLineView, selectBtn)
{
    qDebug() << "TimeLineView selectBtn count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->timeLineBtnClicked();
    QTest::qWait(500);

    TimeLineView *t = w->m_pTimeLineView;
    QList<QWidget*> widgets = t->findChildren<QWidget *>("");
    for (int i = 0; i < widgets.count(); i++){
        if (!strcmp(widgets.at(i)->metaObject()->className(),("Dtk::Widget::DCommandLinkButton"))) {
            DCommandLinkButton *pDCmdBtnSelect = dynamic_cast<DCommandLinkButton*>(widgets.at(i));
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
    for (int i = 0; i < widgets.count(); i++){
        if (!strcmp(widgets.at(i)->metaObject()->className(),("Dtk::Widget::DCommandLinkButton"))) {
            DCommandLinkButton *pDCmdBtnUnselect = dynamic_cast<DCommandLinkButton*>(widgets.at(i));
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

TEST(TimeLineView, keyDeletePhotos)
{
    qDebug() << "TimeLineView selectBtn count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->timeLineBtnClicked();
    QTest::qWait(500);
    TimeLineView *t = w->m_pTimeLineView;
    t->setFocus();
    QList<QWidget*> widgets = t->findChildren<QWidget *>("");
    for (int i = 0; i < widgets.count(); i++){
        if (!strcmp(widgets.at(i)->metaObject()->className(),("Dtk::Widget::DCommandLinkButton"))) {
            DCommandLinkButton *pDCmdBtnSelect = dynamic_cast<DCommandLinkButton*>(widgets.at(i));
            if (pDCmdBtnSelect->text() == QObject::tr("Select")) {
                QTestEventList event;
                event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, QPoint(10, 10));
                event.simulate(widgets.at(i));
                event.clear();
                QTest::qWait(100);
                dApp->signalM->sigShortcutKeyDelete();
                break;
            }
        }
    }
    QTest::qWait(500);
    QString testPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + "test";
    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "", t, true);
    QTest::qWait(500);
}

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



