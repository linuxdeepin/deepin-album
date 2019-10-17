#ifndef ALBUMLEFTTABITEM_H
#define ALBUMLEFTTABITEM_H

#include <QWidget>
#include <DLabel>
#include <DLineEdit>

DWIDGET_USE_NAMESPACE

class AlbumLeftTabItem : public QWidget
{
    Q_OBJECT

public:
    AlbumLeftTabItem(QString str);
    ~AlbumLeftTabItem();
    void editAlbumEdit();

private:
    void initConnections();
    void initUI();

signals:
    void editingFinished();

private slots:
    void onCheckNameValid();

public:
    QString m_albumNameStr;
    int m_opeMode;
    DLineEdit* m_pLineEdit;

private:
    QLabel *m_nameLabel;
    QString m_albumTypeStr;
};

#endif // ALBUMLEFTTABITEM_H
