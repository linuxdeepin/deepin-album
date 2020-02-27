#ifndef ALBUMDELETEDIALOG_H
#define ALBUMDELETEDIALOG_H

#include "dialog.h"


class AlbumDeleteDialog : public Dialog
{
    Q_OBJECT
public:
    explicit AlbumDeleteDialog();
protected:
    void Key_Enter();

signals:
    void deleteAlbum();

};

#endif // ALBUMDELETEDIALOG_H
