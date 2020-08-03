#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "imageengineapi.h"
#include "utils/imageutils.h"
#include "mainwindow.h"
#include "QTestEventList"

QString testPath = "/home/djh/Pictures/test";

TEST(ImportImagesFromFileList, filelist1)
{
    QTime t;
    t.start();
    while (t.elapsed() < 3000)
        dApp->processEvents();
    MainWindow *w = dApp->getMainWindow();
    QThreadPool::globalInstance()->waitForDone();
    w->showEvent(nullptr);
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(0));
    event.clear();
    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "", w->m_pAllPicView, false);
    QThreadPool::globalInstance()->waitForDone();
}
