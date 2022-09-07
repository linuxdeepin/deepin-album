// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef EXPORTER_H
#define EXPORTER_H

#include <QObject>
#include <QMap>

#include "controller/signalmanager.h"
#include "dbmanager/dbmanager.h"
#include "widgets/cexportimagedialog.h"
#include "application.h"

class Exporter : public QObject
{
    Q_OBJECT
public:
    static Exporter *instance();

public slots:
    void exportImage(const QStringList &imagePaths);
    void exportAlbum(const QStringList &albumPaths, const QString &albumname);
    void popupDialogSaveImage(const QStringList &imagePaths);
private:
    explicit Exporter(QObject *parent = nullptr);
    static Exporter *m_exporter;
    QMap<QString, QString> m_picFormatMap;

    void initValidFormatMap();

private:

    CExportImageDialog *m_exportImageDialog;
};

#endif // EXPORTER_H
