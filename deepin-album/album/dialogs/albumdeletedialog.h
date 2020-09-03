#ifndef ALBUMDELETEDIALOG_H
#define ALBUMDELETEDIALOG_H

#include <DWidget>
#include <DDialog>


DWIDGET_USE_NAMESPACE

class AlbumDeleteDialog : public DDialog
{
    Q_OBJECT
public:
    explicit AlbumDeleteDialog(DWidget *parent = nullptr);
    void iniUI();
    DPushButton *m_Cancel;
    DPushButton *m_Delete;
    void keyPressEvent(QKeyEvent *e) override;
signals:
    void deleteAlbum();

};

#endif // ALBUMDELETEDIALOG_H
