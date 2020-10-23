#ifndef IMPORTVIEW_H
#define IMPORTVIEW_H

#include "application.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "controller/configsetter.h"
#include "controller/signalmanager.h"
#include "dbmanager/dbmanager.h"
#include "thumbnail/thumbnaillistview.h"


#include <QWidget>
#include <QVBoxLayout>
#include <DLabel>
#include <QPixmap>
#include <QStandardPaths>
#include <QImageReader>
#include <DPushButton>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <DStackedWidget>
#include <DSlider>
#include <QMimeData>
#include <DFileDialog>

DWIDGET_USE_NAMESPACE

class ImportView : public DWidget, public ImageEngineImportObject
{
    Q_OBJECT

public:
    ImportView();
    bool imageImported(bool success) override;
    void setAlbumname(const QString &name);
    void onImprotBtnClicked(bool useDialog = true, QStringList list = QStringList());

private:
    void initConnections();
    void initUI();
    void mousePressEvent(QMouseEvent *e) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;

signals:

public:
    DPushButton *m_pImportBtn;
    QString m_albumname;
    DLabel *pLabel;
    DBImgInfoList m_dbInfos;
};

#endif // IMPORTVIEW_H
