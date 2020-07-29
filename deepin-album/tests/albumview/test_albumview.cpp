#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "mainwindow.h"
#include "albumview.h"

#include <QTestEventList>

TEST(AlbumView, ini)
{
    QThreadPool::globalInstance()->waitForDone();
    MainWindow *w = dApp->getMainWindow();
    w->showEvent(nullptr);
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(2));
    event.clear();
    AlbumView *a = w->m_pAlbumview;
    for (int i = 0; i < 10; i++) {
        a->m_pStatusBar->m_pSlider->setValue(i);
    }
    a->m_pStatusBar->m_pSlider->setValue(1);
}
