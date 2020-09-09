#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "mainwindow.h"
#include "albumview.h"

#include <QTestEventList>

TEST(AlbumView, ini)
{
    QTime t;
    t.start();
    while (t.elapsed() < 3000)
        dApp->processEvents();
    MainWindow *w = dApp->getMainWindow();
    w->showEvent(nullptr);
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(2));
    event.clear();
    QTime time;
    time.start();
    while (time.elapsed() < 2000)
        dApp->processEvents();
    AlbumView *a = w->m_pAlbumview;
    for (int i = 0; i < 10; i++) {
        a->m_pStatusBar->m_pSlider->setValue(i);
    }
    a->m_pStatusBar->m_pSlider->setValue(1);
    time.restart();
    while (time.elapsed() < 2000)
        dApp->processEvents();
}

TEST(AlbumView, leftMenu)
{
    QTime time;
    time.start();
    while (time.elapsed() < 3000)
        dApp->processEvents();
    MainWindow *w = dApp->getMainWindow();
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(2));
    event.clear();
    time.restart();
    while (time.elapsed() < 2000)
        dApp->processEvents();
    AlbumView *a = w->m_pAlbumview;
    event.addMousePress(Qt::MouseButton::LeftButton,Qt::NoModifier,QPoint(10,10));
    event.addMouseRelease(Qt::MouseButton::LeftButton,Qt::NoModifier,QPoint(10,10));
    event.simulate(a->m_pLeftListView->m_pPhotoLibListView->viewport());
    time.restart();
    while (time.elapsed() < 1000)
        dApp->processEvents();
    event.simulate(a->m_pLeftListView->m_pCustomizeListView->viewport());
    time.restart();
    while (time.elapsed() < 1000)
        dApp->processEvents();
    event.simulate(a->m_pLeftListView->m_pMountListWidget->viewport());
    time.restart();
    while (time.elapsed() < 1000)
        dApp->processEvents();
    event.simulate(a->m_waitDeviceScandialog->m_closeDeviceScan);
    while (time.elapsed() < 1000)
        dApp->processEvents();
    dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Light);
    time.restart();
    while (time.elapsed() < 200)
        dApp->processEvents();
    dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Dark);
    time.restart();
    while (time.elapsed() < 1000)
        dApp->processEvents();
}
