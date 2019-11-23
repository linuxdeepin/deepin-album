#ifndef IMPORTVIEW_H
#define IMPORTVIEW_H

#include "application.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "controller/configsetter.h"
#include "controller/signalmanager.h"
#include "dbmanager/dbmanager.h"
#include "widgets/thumbnaillistview.h"


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

DWIDGET_USE_NAMESPACE

class ImportView : public DWidget
{
    Q_OBJECT

public:
    ImportView();
    void setAlbumname(const QString &name);
    void onImprotBtnClicked();

private:
    void initConnections();
    void initUI();
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;

signals:
    void importFailedToView();

public:
    DPushButton* m_pImportBtn;
    QString m_albumname;
    DLabel *pLabel;
};

#endif // IMPORTVIEW_H
