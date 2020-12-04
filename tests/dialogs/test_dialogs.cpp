#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "dialogs/albumcreatedialog.h"
#include "dialogs/albumdeletedialog.h"
#include "widgets/dialogs/imgdeletedialog.h"
#include "controller/exporter.h"
#include "../test_qtestDefine.h"

#include <QTestEventList>

TEST(albumcreatedialog, dia1)
{
    qDebug() << "dialogs albumcreatedialog dia1 count = " << count_testDefine++;
    AlbumCreateDialog *a = new AlbumCreateDialog;
    ASSERT_TRUE(a->getCreateAlbumName().isEmpty());

    QTestEventList event;
    event.addKeyClick(Qt::Key_Escape);
    event.simulate(a);

    QTest::keyClicks(a->getEdit(), "test1");
    emit a->buttonClicked(0, "");
}

TEST(albumdeletedialog, deletdialog)
{
    qDebug() << "dialogs albumdeletedialog deletdialog count = " << count_testDefine++;
    AlbumDeleteDialog *d = new AlbumDeleteDialog;
    Q_UNUSED(d);
    QTestEventList event;
    event.addKeyClick(Qt::Key_Escape);
}

TEST(Exporter, exportdialog)
{
    CExportImageDialog *c = new CExportImageDialog;
//    c->getQuality();
//    c->getSavePath();
    c->getImageType();
    c->getImageFormate();
//    c->showDirChoseDialog();
//    c->showEmptyWarningDialog();
    c->deleteLater();
}
