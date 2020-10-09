#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "mainwindow.h"
#include "albumview.h"
#include "baseutils.h"
#include "dbmanager.h"
#include "albumcreatedialog.h"
#include "../test_qtestDefine.h"

#include <QTestEventList>

TEST(AlbumView, deleteAll)
{
    if (!switch_on_test) {
        return;
    }
    ImageEngineApi::instance()->load80Thumbnails();
    QTest::qWait(500);
    qDebug() << "AlbumView deleteAll count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->loadZoomRatio();

    QDir dir(testPath_test);
    if (!dir.exists()) {
        dir.mkdir(testPath_test);
    }
    QFileInfo fileinfo;
    QPixmap pix;
    fileinfo.setFile(testPath_test + "/13lzwv.png");
    if (!fileinfo.exists()) {
        pix = QPixmap(":/13lzwv.png");
        pix.save(testPath_test + "/13lzwv.png");
    }
    fileinfo.setFile(testPath_test + "/2e5y8y.png");
    if (!fileinfo.exists()) {
        pix = QPixmap(":/2e5y8y.png");
        pix.save(testPath_test + "/2e5y8y.png");
    }
    fileinfo.setFile(testPath_test + "/2ejqyx.png");
    if (!fileinfo.exists()) {
        pix = QPixmap(":/2ejqyx.png");
        pix.save(testPath_test + "/2ejqyx.png");
    }
    fileinfo.setFile(testPath_test + "/2k9o1m.png");
    if (!fileinfo.exists()) {
        pix = QPixmap(":/2k9o1m.png");
        pix.save(testPath_test + "/2k9o1m.png");
    }
    fileinfo.setFile(testPath_test + "/3333.png");
    if (!fileinfo.exists()) {
        pix = QPixmap(":/3333.png");
        pix.save(testPath_test + "/3333.png");
    }
    fileinfo.setFile(testPath_test + "/39elz3.png");
    if (!fileinfo.exists()) {
        pix = QPixmap(":/39elz3.png");
        pix.save(testPath_test + "/39elz3.png");
    }
    fileinfo.setFile(testPath_test + "/3kp6yv.png");
    if (!fileinfo.exists()) {
        pix = QPixmap(":/3kp6yv.png");
        pix.save(testPath_test + "/3kp6yv.png");
    }
    fileinfo.setFile(testPath_test + "/4l6r5y.png");
    if (!fileinfo.exists()) {
        pix = QPixmap(":/4l6r5y.png");
        pix.save(testPath_test + "/4l6r5y.png");
    }
    fileinfo.setFile(testPath_test + "/4v9ml0.png");
    if (!fileinfo.exists()) {
        pix = QPixmap(":/4v9ml0.png");
        pix.save(testPath_test + "/4v9ml0.png");
    }
}

TEST(AlbumView, removeTestImagesInfo)
{
    if (!switch_on_test) {
        return;
    }
    qDebug() << "AlbumView removeTestImagesInfo count = " << count_testDefine++;
    QStringList image_list;
    auto finfos = utils::image::getImagesInfo(testPath_test);
    for (auto info : finfos) {
        image_list << info.absoluteFilePath();
    }
    QStringList res = DBManager::instance()->getAllPaths();
    QTest::qWait(1000);
    DBManager::instance()->removeImgInfos(res);
    DBImgInfoList dbinfos;
    for (auto i : image_list) {
        DBImgInfo info;
        info.filePath = i;
        dbinfos << info;
    }
    MainWindow *w = dApp->getMainWindow();
    AllPicView *a = w->m_pAllPicView;
    a->updatePicsThumbnailView();
    QTest::qWait(1000);
}

TEST(AlbumView, clickImportViewBtn)
{
    if (!switch_on_test) {
        return;
    }
    qDebug() << "AlbumView clickImportViewBtn count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    AllPicView *a = w->m_pAllPicView;
    ImportView *importView = a->m_pImportView;

    QStringList image_list;
    auto finfos = utils::image::getImagesInfo(testPath_test);
    for (auto info : finfos) {
        image_list << info.absoluteFilePath();
    }
    QTest::qWait(500);
    importView->onImprotBtnClicked(false, image_list);
    QTest::qWait(500);

    ImageEngineApi::instance()->insertImage(image_list.first(), "");
}

TEST(AlbumView, ini)
{
    if (!switch_on_test) {
        return;
    }
    qDebug() << "AlbumView ini count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->loadZoomRatio();

    QPoint pos(10, 10);
    QTestEventList event;
    event.addMouseMove(pos);
    event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pos);
    event.simulate(w->getButG()->button(0));
    event.clear();
    QTest::qWait(500);

    event.addMouseMove(pos);
    event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pos);
    event.simulate(w->getButG()->button(1));
    event.clear();
    QTest::qWait(500);

    event.addMouseMove(pos);
    event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pos);
    event.simulate(w->getButG()->button(2));
    event.clear();
    QTest::qWait(500);
}

TEST(AlbumView, iniAlbum)
{
    if (!switch_on_test) {
        return;
    }
    qDebug() << "AlbumView ini count = " << count_testDefine++;
    QTest::qWait(500);
    MainWindow *w = dApp->getMainWindow();
    w->showEvent(nullptr);
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(2));
    event.clear();
    QTest::qWait(500);

    AlbumView *a = w->m_pAlbumview;
    int width = a->m_pStatusBar->m_pSlider->slider()->width();
    QPoint pos(width / 10 * a->m_pStatusBar->m_pSlider->slider()->sliderPosition(), 10);
    event.addMousePress(Qt::LeftButton, Qt::NoModifier, pos);
    pos = pos - QPoint(200, 0);
    event.addMouseMove(pos);
    QTest::qWait(500);
    event.simulate(a->m_pStatusBar->m_pSlider->slider());
    pos = QPoint(width / 10 * a->m_pStatusBar->m_pSlider->slider()->sliderPosition(), 10);
    for (int i = 0; i < 5; i++) {
        event.addMousePress(Qt::LeftButton, Qt::NoModifier, pos);
        pos = pos + QPoint(20, 0);
        event.addMouseMove(pos);
        event.simulate(a->m_pStatusBar->m_pSlider->slider());
        event.clear();
        QTest::qWait(400);
    }
    pos = QPoint(width / 10 * a->m_pStatusBar->m_pSlider->slider()->sliderPosition(), 10);
    for (int i = 0; i < 5; i++) {
        event.addMousePress(Qt::LeftButton, Qt::NoModifier, pos);
        pos = pos - QPoint(20, 0);
        event.addMouseMove(pos);
        event.simulate(a->m_pStatusBar->m_pSlider->slider());
        event.clear();
        QTest::qWait(400);
    }
}

TEST(AlbumView, leftMenu)
{
    if (!switch_on_test) {
        return;
    }
    qDebug() << "AlbumView leftMenu count = " << count_testDefine++;
    QTest::qWait(500);
    MainWindow *w = dApp->getMainWindow();
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(2));
    event.clear();
    QTest::qWait(300);
    AlbumView *a = w->m_pAlbumview;
    event.addMousePress(Qt::MouseButton::LeftButton, Qt::NoModifier, QPoint(10, 10));
    event.addMouseRelease(Qt::MouseButton::LeftButton, Qt::NoModifier, QPoint(10, 10));
    event.simulate(a->m_pLeftListView->m_pPhotoLibListView->viewport());
    QTest::qWait(500);
    event.simulate(a->m_pLeftListView->m_pCustomizeListView->viewport());
    QTest::qWait(500);
    dApp->processEvents();
    event.simulate(a->m_pLeftListView->m_pMountListWidget->viewport());
    QTest::qWait(500);
    event.simulate(a->m_waitDeviceScandialog->m_closeDeviceScan);
    QTest::qWait(500);
    dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Light);
    dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Dark);
    QTest::qWait(500);
}

TEST(AlbumView, imageOpen)
{
    if (!switch_on_test) {
        return;
    }
    qDebug() << "AlbumView imageOpen count = " << count_testDefine++;
    QString testPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + "test";
    MainWindow *w = dApp->getMainWindow();
    AlbumView *a = w->m_pAlbumview;
    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "", a, false);
    QTest::qWait(500);
    QStringList testPathlist = ImageEngineApi::instance()->get_AllImagePath();
    if (!testPathlist.isEmpty()) {
        qDebug() << "test ImageView Success ";
        w->m_pSearchEdit->setText("1");
        w->m_pSearchEdit->editingFinished();
        QTest::qWait(300);
        emit a->m_pRightThumbnailList->menuOpenImage(testPathlist.first(), testPathlist, false);
        QTest::qWait(300);
        QTestEventList e;
        e.addKeyClick(Qt::Key_Minus, Qt::ControlModifier, 100);
        e.addKeyClick(Qt::Key_Minus, Qt::ControlModifier, 100);
        e.addKeyClick(Qt::Key_Plus, Qt::ControlModifier, 100);
        e.addKeyClick(Qt::Key_Plus, Qt::ControlModifier, 100);
        e.addKeyClick(Qt::Key_Plus, Qt::ControlModifier, 100);
        e.addKeyClick(Qt::Key_Plus, Qt::ControlModifier, 100);
        e.addKeyClick(Qt::Key_Plus, Qt::ControlModifier, 100);
        e.simulate(w);
        emit dApp->signalM->showImageInfo(testPathlist.first());
    }
}

TEST(AlbumView, deleteImgRecovery)
{
    if (!switch_on_test) {
        return;
    }
    qDebug() << "AlbumView deleteImgRecovery count = " << count_testDefine++;
    QTest::qWait(500);
    MainWindow *w = dApp->getMainWindow();
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(2));
    event.clear();
    AlbumView *a = w->m_pAlbumview;
    a->m_currentType = COMMON_STR_TRASH;
    a->updateRightView();
    ImageEngineApi::instance()->recoveryImagesFromTrash(a->m_pRightThumbnailList->getAllPaths());
}

TEST(AlbumView, exportAlbum)
{
    if (!switch_on_test) {
        return;
    }
    qDebug() << "AlbumView exportAlbum count = " << count_testDefine++;
    AlbumCreateDialog *ad = new AlbumCreateDialog;
    QTest::keyClicks(ad->getEdit(), "newtestalbum1");
    emit ad->buttonClicked(1, "");
    QTest::qWait(200);
    QString testPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + "test";

    MainWindow *w = dApp->getMainWindow();
    AlbumView *a = w->m_pAlbumview;
    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "newtestalbum1", a, false);
    QTest::qWait(500);
    QStringList testImage = ImageEngineApi::instance()->get_AllImagePath();
    QString exportFile = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + "exportTest";
    QDir Dir(exportFile);
    if (!Dir.isEmpty()) {
        QDirIterator DirsIterator(exportFile, QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDirIterator::NoIteratorFlags);
        while (DirsIterator.hasNext()) {
            if (!Dir.remove(DirsIterator.next())) { // 删除文件操作如果返回否，那它就是目录
                QDir(DirsIterator.filePath()).removeRecursively(); // 删除目录本身以及它下属所有的文件及目录
            }
        }
    }

    Exporter::instance()->instance()->exportAlbum(testImage, "newtestalbum1");
    Exporter::instance()->instance()->exportImage(testImage.mid(0, 1));
}

TEST(AlbumView, device)
{

}
