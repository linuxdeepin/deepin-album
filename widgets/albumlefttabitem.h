#ifndef ALBUMLEFTTABITEM_H
#define ALBUMLEFTTABITEM_H

#include <QWidget>
#include <DLabel>
#include <DLineEdit>
#include <DApplicationHelper>

DWIDGET_USE_NAMESPACE

class AlbumLeftTabItem : public QWidget
{
    Q_OBJECT

public:
    AlbumLeftTabItem(QString str, QString strAlbumType = "");
    ~AlbumLeftTabItem();
    void editAlbumEdit();
    void oriAlbumStatus();
    void newAlbumStatus();

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

    DLineEdit* m_pNewLineEdit;

private:
    QLabel *m_nameLabel;
    QString m_albumTypeStr;

    QLabel *pLabel;
    QLabel *pNewLabel;
    QLabel *m_newnameLabel;

    QLabel *pmainLabel;

};

#endif // ALBUMLEFTTABITEM_H
