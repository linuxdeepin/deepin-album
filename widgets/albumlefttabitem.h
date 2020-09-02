#ifndef ALBUMLEFTTABITEM_H
#define ALBUMLEFTTABITEM_H

#include <QWidget>
#include <DLabel>
#include <DLineEdit>
#include <DApplicationHelper>
#include <DIconButton>
#include "albumview/leftlistwidget.h"
#include "mountexternalbtn.h"
#include "widgets/ddlabel.h"

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
    QString getalbumname();

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
    DLineEdit *m_pLineEdit;
    DLineEdit *m_pNewLineEdit;
    QString m_mountPath;
private:
    DDlabel *m_nameLabel;
    DLabel *pImageLabel;
    MountExternalBtn *m_unMountBtn;
    LeftListWidget *m_pListWidget;
    QListWidgetItem *m_pListWidgetItem;
};

#endif // ALBUMLEFTTABITEM_H
