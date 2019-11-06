#ifndef ALBUMLEFTTABITEM_H
#define ALBUMLEFTTABITEM_H

#include <QWidget>
#include <DLabel>
#include <DLineEdit>
#include <DApplicationHelper>
#include <DIconButton>

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
    void setExternalDevicesMountPath(QString strPath);

private:
    void initConnections();
    void initUI();
    void unMountBtnClicked();

signals:
    void editingFinished();
    void unMountExternalDevices(QString mountName);

public slots:
    void onCheckNameValid();

public:
    QString m_albumNameStr;
    QString m_albumTypeStr;
    int m_opeMode;
    DLineEdit* m_pLineEdit;

    DLineEdit* m_pNewLineEdit;

private:
    QLabel *m_nameLabel;

    QLabel *pLabel;
    QLabel *pNewLabel;
    QLabel *m_newnameLabel;

    QLabel *pmainLabel;
    DIconButton *m_unMountBtn;
    QString m_mountPath;
};

#endif // ALBUMLEFTTABITEM_H
