#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "mainwindow.h"
#include "allpicview.h"
#include "imgdeletedialog.h"
#include "wallpapersetter.h"
#include "ttbcontent.h"
#include "viewpanel.h"
#include "mainwidget.h"
#include "signalmanager.h"
#include "../test_qtestDefine.h"

#include <QTestEventList>

TEST(allpicview, test_ini)
{
    qDebug() << "allpicview test_ini count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();

    QPoint pos(20, 20);
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton, Qt::NoModifier, pos);
    event.simulate(w->getButG()->button(0));
    AllPicView *a = w->m_pAllPicView;

    int width = a->m_pStatusBar->m_pSlider->slider()->width();
    pos = QPoint(width / 10 * a->m_pStatusBar->m_pSlider->slider()->sliderPosition(), 10);
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
        QTest::qWait(500);
    }
    pos = QPoint(width / 10 * a->m_pStatusBar->m_pSlider->slider()->sliderPosition(), 10);
    for (int i = 0; i < 5; i++) {
        event.addMousePress(Qt::LeftButton, Qt::NoModifier, pos);
        pos = pos - QPoint(20, 0);
        event.addMouseMove(pos);
        event.simulate(a->m_pStatusBar->m_pSlider->slider());
        event.clear();
        QTest::qWait(500);
    }
    event.addMousePress(Qt::LeftButton, Qt::NoModifier, pos);
    pos = pos - QPoint(20, 0);
    event.addMouseMove(pos);
    event.addMouseRelease(Qt::LeftButton, Qt::NoModifier, pos);
    event.simulate(a->m_pStatusBar->m_pSlider->slider());
    event.clear();
}

TEST(allpicview, resize)
{
    qDebug() << "allpicview test_ini count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();

    QTest::qWait(200);
    w->resize(w->size() + QSize(300, 300));
    QTest::qWait(200);
    QResizeEvent resize2(w->size() - QSize(100, 100), w->size());
    w->resize(w->size() - QSize(300, 300));
    QTest::qWait(200);
}

TEST(allpicview, keypress)
{
    QTest::qWait(200);
    qDebug() << "allpicview test_ini count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    AllPicView *a = w->m_pAllPicView;
    w->show();
    QTestEventList event;
    event.addKeyPress(Qt::Key_A, Qt::ControlModifier, 100);
    event.simulate(a->getThumbnailListView());
    QTest::qWait(200);
}

TEST(allpicview, dragevent)
{
    QTest::qWait(200);
    qDebug() << "allpicview test_ini count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    AllPicView *a = w->m_pAllPicView;
    w->show();

    // 打开保存绘制的 ddf
    QString jpgItemPath = testPath_test + "/2e5y8y.jpg";

    QString text = "xxxxxxxxxxxxxx";
    QIcon icon = QIcon(":/resources/images/other/deepin-album.svg");
    QIcon icon_hover = QIcon(":/resources/images/other/deepin-album.svg");
    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << text << icon << icon_hover;
//    QMimeData *mimeData = new QMimeData;
    QMimeData mimedata;
    mimedata.setData(QStringLiteral("TestListView/text-icon-icon_hover"), itemData);
    QList<QUrl> li;
    li.append(QUrl::fromLocalFile(jpgItemPath));
    mimedata.setUrls(li);

    const QPoint pos = a->getThumbnailListView()->viewport()->rect().center();
    QDragEnterEvent eEnter(pos, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
    dApp->getDAppNew()->sendEvent(a->getThumbnailListView()->viewport(), &eEnter);
    QTest::qWait(200);

    QDragMoveEvent eMove(pos + QPoint(100, 100), Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
    dApp->getDAppNew()->sendEvent(a->getThumbnailListView()->viewport(), &eMove);
    QTest::qWait(200);

    QDropEvent e(pos, Qt::IgnoreAction, &mimedata, Qt::LeftButton, Qt::NoModifier);
    dApp->getDAppNew()->sendEvent(a->getThumbnailListView()->viewport(), &e);
    QTest::qWait(200);

    dApp->getDAppNew()->sendEvent(a->getThumbnailListView()->viewport(), &eEnter);
    QTest::qWait(200);

    QDragLeaveEvent eLeave;
    dApp->getDAppNew()->sendEvent(a->getThumbnailListView()->viewport(), &eLeave);
    QTest::qWait(2000);
}

TEST(allpicview, mousePress)
{
    qDebug() << "allpicview mousePress count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();

    QPoint pos(40, 60);
    AllPicView *a = w->m_pAllPicView;
    QTestEventList event;
    event.addMousePress(Qt::LeftButton, Qt::NoModifier, pos);
    event.addMouseMove(pos + QPoint(200, 200));
    event.addMouseRelease(Qt::LeftButton, Qt::NoModifier, pos + QPoint(200, 200));
    event.simulate(a->getThumbnailListView()->viewport());
    event.clear();
    QTest::qWait(500);
}

TEST(allpicview, test_open)
{
    qDebug() << "allpicview test_open count = " << count_testDefine++;
    QString publicTestPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + "test";
    QString testPath = publicTestPath;
    MainWindow *w = dApp->getMainWindow();
    QStringList testPathlist = ImageEngineApi::instance()->get_AllImagePath();
    if (!testPathlist.isEmpty()) {
        ImageEngineApi::instance()->moveImagesToTrash(testPathlist);
        QTest::qWait(500);
    }
    AllPicView *a = w->m_pAllPicView;
    QTestEventList tl;
    tl.addMouseMove(QPoint(0, 0), 100);
    tl.addMousePress(Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier, QPoint(0, 0), 100);
    tl.addMouseMove(a->m_pImportView->geometry().center());
    tl.simulate(a->m_pImportView);
    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "", a, false);
    QTest::qWait(500);

    testPathlist = ImageEngineApi::instance()->get_AllImagePath();
    if (!testPathlist.isEmpty()) {
        qDebug() << "test ImageView Success ";
        w->m_pSearchEdit->setText("1");
        w->m_pSearchEdit->editingFinished();
        QTest::qWait(300);
        emit a->getThumbnailListView()->menuOpenImage(testPathlist.first(), testPathlist, false);
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

        CommandLine *commandline = CommandLine::instance();
        QString jpgItemPath = testPath_test + "/2e5y8y.jpg";
        QStringList list;
        list << jpgItemPath;
        commandline->viewImage(jpgItemPath, list);

        MainWidget *mw = CommandLine::instance()->findChild<MainWidget *>("MainWidget");
        if (mw) {
            TTBContent *t = mw->findChild<TTBContent *>("TTBContent");
            if (t) {
                MyImageListWidget *ml = t->findChild<MyImageListWidget *>("MyImageListWidget");
                if (ml) {
//                    QTestEventList el;
//                    QPoint p(ml->pos().x() - 50, ml->pos().y() + 10);
//                    el.addMouseMove(p);
//                    el.addMousePress(Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier, p, 100);
//                    el.addMouseMove(QPoint(p.x() + 100, p.y()), 100);
//                    el.addMouseRelease(Qt::LeftButton, Qt::NoModifier, QPoint(p.x() + 100, p.y()), 100);
//                    el.simulate(ml);
                }
            }
        }
    }
    QTest::qWait(500);
    QTestEventList e;
    e.addKeyClick(Qt::Key_Escape);
    e.simulate(w);

    ImgDeleteDialog *dialog = new ImgDeleteDialog(a->getThumbnailListView(), testPathlist.length());
    ImageEngineApi::instance()->moveImagesToTrash(testPathlist, true, false);
    dialog->deleteLater();
}

TEST(allpicview, test_select)
{
    qDebug() << "allpicview test_select count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    AllPicView *a = w->m_pAllPicView;
    QTestEventList e;
    QPoint p = a->getThumbnailListView()->pos();
    p.setX(p.x() + 10);
    p.setY(p.y() + 10);
    e.addMouseMove(p);
    e.addKeyPress(Qt::LeftButton);
    e.addMouseMove(QPoint(p.x() + 400, p.y() + 200));
    e.clear();
    e.simulate(a->getThumbnailListView());

    dApp->signalM->sigShortcutKeyDelete();
    QTest::qWait(100);
    a->getThumbnailListView()->sigLoad80ThumbnailsFinish();
    QTest::qWait(100);
    a->updateStackedWidget();
    a->restorePicNum();
    a->updatePicNum();
    dApp->signalM->imagesRemoved();
    QTest::qWait(100);
    a->getThumbnailListView()->openImage(0);
    QTest::qWait(100);
}

TEST(allpicview, test_shortCut)
{
    qDebug() << "allpicview test_shortCut count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->createShorcutJson();
    QTestEventList e;
    e.addKeyClick(Qt::Key_Question, Qt::ControlModifier | Qt::ShiftModifier);
    e.simulate(w);
    QTest::qWait(500);
}

TEST(allpicview, test_showInFileManagerAndBackGrond)
{
    qDebug() << "allpicview test_showInFileManagerAndBackGrond count = " << count_testDefine++;
    using namespace utils::base;
    using namespace utils::image;
    MainWindow *w = dApp->getMainWindow();
    AllPicView *a = w->m_pAllPicView;
    QString testPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + "test";
    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "", a, false);
    QTime t;
    QTest::qWait(500);
    QStringList paths = ImageEngineApi::instance()->get_AllImagePath();
    if (!paths.isEmpty()) {
        QString testImage = paths.first();
//        showInFileManager(testImage);//roc
//        imageSupportSave(testImage);
//        checkFileType(testImage);
        copyImageToClipboard(paths);
//        copyOneImageToClipboard(testImage);
        WallpaperSetter::instance()->setBackground(testImage);
        QTest::qWait(500);
        QMimeData *newMimeData = new QMimeData();
        QByteArray gnomeFormat = QByteArray("copy\n");
        QString text;
        QList<QUrl> dataUrls;

        for (QString path : paths) {
            if (!path.isEmpty())
                text += path + '\n';
            dataUrls << QUrl::fromLocalFile(path);
            gnomeFormat.append(QUrl::fromLocalFile(path).toEncoded()).append("\n");
        }
        newMimeData->setUrls(dataUrls);
        checkMimeData(newMimeData);
        QTest::qWait(500);
    }
}

TEST(ttbcontent, test_ini)
{
    qDebug() << "ttbcontent test_ini count = " << count_testDefine++;
    QString publicTestPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + "test";
    QString testPath = publicTestPath;
    MainWindow *w = dApp->getMainWindow();
    AllPicView *a = w->m_pAllPicView;
    QTestEventList tl;
    tl.addMouseMove(QPoint(0, 0), 100);
    tl.addMousePress(Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier, QPoint(0, 0), 100);
    tl.addMouseMove(a->m_pImportView->geometry().center());
    tl.simulate(a->m_pImportView);
    ImageEngineApi::instance()->ImportImagesFromFileList((QStringList() << testPath), "", a, false);
    QTest::qWait(500);

    QStringList testPathlist = ImageEngineApi::instance()->get_AllImagePath();
    if (!testPathlist.isEmpty()) {
        qDebug() << "test ImageView Success ";
        w->m_pSearchEdit->setText("1");
        w->m_pSearchEdit->editingFinished();
        QTest::qWait(300);

        emit a->getThumbnailListView()->menuOpenImage(testPathlist.first(), testPathlist, false);
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
        e.clear();

        emit dApp->signalM->showImageInfo(testPathlist.first());

        QTest::qWait(500);
        ImgInfoDialog *dialog = w->findChild<ImgInfoDialog *>("ImgInfoDialog");
        dialog->deleteLater();
        QTest::qWait(500);
        ViewPanel *viewPanel = w->findChild<ViewPanel *>("ViewPanel");
        if (viewPanel) {
            viewPanel->getImageView();
        }

        MainWidget *mw = CommandLine::instance()->findChild<MainWidget *>("MainWidget");
        if (mw) {
            TTBContent *t = mw->findChild<TTBContent *>("TTBContent");
            if (t == nullptr) {
                return;
            }
            t->onNextButton();
            QTest::qWait(500);
            t->onPreButton();
            QTest::qWait(500);
            t->updateFilenameLayout();
            QTest::qWait(500);

            DWidget *imgList = t->findChild<DWidget *>("imageListObj");
            if (imgList) {
                QObjectList list = dynamic_cast<DWidget *>(imgList)->children();
                for (int i = 0; i < list.size(); i++) {
                    QList<ImageItem *> labelList = dynamic_cast<DWidget *>(imgList)->findChildren<ImageItem *>(QString("%1").arg(i));
                    if (labelList.size() <= 0) {
                        continue;
                    }
                    ImageItem *img = labelList.at(0);
                    if (nullptr == img) {
                        continue;
                    }
                    img->emitClickSig();
                    QTest::qWait(400);
                }
            }

            MyImageListWidget *imgListView = t->findChild<MyImageListWidget *>("MyImageListWidget");
            if (imgListView) {
                imgListView->animationFinished();
                imgListView->findSelectItem();
                imgListView->animationStart(true, 0, 300);
                imgListView->stopAnimation();
//                imgListView->isAnimationStart();
//                imgListView->animationTimerTimeOut();
            }

            QTestEventList el;
            QPoint p(150, 20);
            el.addMouseMove(p);
            el.addMousePress(Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier, p, 100);
            p = p + QPoint(-20, 0);
            el.addMouseMove(p, 100);
//            QTest::qWait(300);
            p = p + QPoint(-20, 0);
            el.addMouseMove(p, 100);
//            QTest::qWait(300);
            p = p + QPoint(-20, 0);
            el.addMouseMove(p, 100);
//            QTest::qWait(300);
            p = p + QPoint(-20, 0);
            el.addMouseMove(p, 100);
//            QTest::qWait(300);
            p = p + QPoint(-20, 0);
            el.addMouseMove(p, 100);
//            QTest::qWait(300);
            el.addMouseRelease(Qt::LeftButton, Qt::NoModifier, p, 100);
            el.simulate(imgList);
            el.clear();

            if (imgListView) {
                QPoint p2(imgListView->pos().x() - 50, imgListView->pos().y() + 10);
                el.addMouseMove(p2);
                el.addMousePress(Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier, p2, 100);
                el.addMouseMove(QPoint(p2.x() + 100, p2.y()), 100);
                el.addMouseRelease(Qt::LeftButton, Qt::NoModifier, QPoint(p2.x() + 100, p2.y()), 100);
                el.simulate(imgListView);
            }
        }
    }

    QTestEventList e;
    e.addKeyClick(Qt::Key_Escape);
    e.simulate(w);
}

TEST(allpicview, deleteTips)
{
    qDebug() << "allpicview deleteTips count = " << count_testDefine++;
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(500);
    AllPicView *a = w->m_pAllPicView;
    QStringList testPathlist = ImageEngineApi::instance()->get_AllImagePath();
    if (testPathlist.count() > 0) {
        QStringList tempDel;
        tempDel << testPathlist.last();
        ImgDeleteDialog *delDlg= new ImgDeleteDialog(a->getThumbnailListView(), tempDel.length());
        delDlg->show();
        QTest::qWait(500);
        ImageEngineApi::instance()->moveImagesToTrash(tempDel);
        delDlg->deleteLater();
        QTest::qWait(500);
    }
}
