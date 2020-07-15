#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "imageengineapi.h"
#include "utils/imageutils.h"
#include "mainwindow.h"


TEST(ImportImagesFromFileList, filelist1)
{
    MainWindow *w = dApp->getMainWindow();

    QString TESTPIC_PATH = "/home/ut-djh/Pictures/test";
    QFileInfoList fileInfoLis1 = utils::image::getImagesInfo(TESTPIC_PATH, true);
    QStringList filelist;
    for (QFileInfo inf : fileInfoLis1) {
        filelist.append(inf.absoluteFilePath());
    }
    ImageEngineApi::instance()->ImportImagesFromFileList(filelist, "", w->m_pAllPicView);
    QThreadPool::globalInstance()->waitForDone();
}
