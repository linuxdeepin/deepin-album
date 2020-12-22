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
    void keyPressEvent(QKeyEvent *e) override;
signals:
    void deleteAlbum();

private slots:
    void onButtonClicked(int index, const QString & text);
};

#endif // ALBUMDELETEDIALOG_H
