/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     ZhangYong <zhangyong@uniontech.com>
 *
 * Maintainer: ZhangYong <ZhangYong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
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
    void setUID(int UID);
    void onImprotBtnClicked(bool useDialog = true, const QStringList &list = QStringList());

private:
    void initConnections();
    void initUI();
    void mousePressEvent(QMouseEvent *e) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
//    void dragLeaveEvent(QDragLeaveEvent *e) override;

private slots:
    void onThemeTypeChanged();

signals:

public:
    DPushButton *m_pImportBtn;
    QString m_albumname;
    int m_UID = -1;
    DLabel *pLabel;
    DBImgInfoList m_dbInfos;
    DLabel *pNoteLabel;
};

#endif // IMPORTVIEW_H
