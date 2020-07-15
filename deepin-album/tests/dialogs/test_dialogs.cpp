#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "dialogs/albumcreatedialog.h"
#include "dialogs/albumdeletedialog.h"

#include <QTestEventList>

TEST(albumcreatedialog, dia1)
{
    AlbumCreateDialog *a = new AlbumCreateDialog;
    a->hide();
    a->test_setEditText("");
    ASSERT_TRUE(a->getCreateAlbumName().isEmpty());
    AlbumDeleteDialog *d = new AlbumDeleteDialog;
    QTestEventList event;
    event.addKeyClick(Qt::Key_Escape);
    event.simulate(a);
}
