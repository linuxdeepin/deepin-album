// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "exporter.h"
#include "utils/imageutils.h"
#include "utils/unionimage.h"

#include <QFileDialog>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QMimeDatabase>

Exporter *Exporter::m_exporter = nullptr;
Exporter *Exporter::instance()
{
    if (!m_exporter) {
        m_exporter = new Exporter();
    }

    return m_exporter;
}


Exporter::Exporter(QObject *parent)
    : QObject(parent)
    , m_exportImageDialog(nullptr)
{
    m_exportImageDialog = new CExportImageDialog();
}

//TODO: if some format is valid to read, but can't support to export, should add some process ?
//Such as: gif, svg, pbm, pgm
void Exporter::exportImage(const QStringList &imagePaths)
{
    if (imagePaths.isEmpty()) {
        return;
    } else if (imagePaths.length() == 1) {
        initValidFormatMap();
        //QFileDialog exportDialog;
        //Todo: need to filter the format of images.
        QString imageName = QString("%1.%2").arg(QFileInfo(imagePaths.at(0)).baseName())
                            .arg(QFileInfo(imagePaths.at(0)).completeSuffix());
        m_exportImageDialog->setPicFileName(imageName);
        m_exportImageDialog->removeGifType();
        QFileInfo info(imagePaths.at(0));
        QMimeDatabase db;
        QMimeType mt = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchContent);
        QMimeType mt1 = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchExtension);

        if (mt.name().startsWith("image/gif")) {
            m_exportImageDialog->setGifType(imagePaths.at(0));
        }

        QImage tImg;
        QString errMsg;
        QPixmap pixmap;
        using namespace UnionImage_NameSpace;
        loadStaticImageFromFile(imagePaths.at(0), tImg, errMsg);
        pixmap = QPixmap::fromImage(tImg);
        m_exportImageDialog->showMe(pixmap);
    } else {
        popupDialogSaveImage(imagePaths);
    }
}

void Exporter::exportAlbum(const QStringList &albumPaths, const QString &albumname)
{
    QFileDialog exportDialog;
    exportDialog.setFileMode(QFileDialog::DirectoryOnly);
    exportDialog.setLabelText(QFileDialog::Accept, tr("Save"));
    exportDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).at(0));

    if (exportDialog.exec() == QDialog::Accepted) {
        QString exportdir = exportDialog.directory().absolutePath();

        QDir dir;
        dir.mkdir(exportdir + "/" + albumname);
        exportdir = exportdir + "/" + albumname;

        int failcount = 0;
        for (int j(0); j < albumPaths.length(); j++) {

            if (utils::image::imageSupportRead(albumPaths[j])) {
                QString savePath =  QString("%1/%2.%3").arg(exportdir).arg(QFileInfo(albumPaths[j])
                                                                           .baseName()).arg(QFileInfo(albumPaths[j]).completeSuffix());
                bool isSucceed = QFile::copy(albumPaths[j], savePath);
                emit dApp->signalM->sigExporting(albumPaths[j]);

                if (!isSucceed) {
                    // qDebug() << "Export failed";
                }
            } else {
                failcount++;
                continue;
            }
        }
        if (failcount == albumPaths.length()) {
            emit dApp->signalM->AlbExportFailed();
        } else {
            emit dApp->signalM->AlbExportSuccess();
            emit dApp->signalM->sigRestoreStatus();
        }

    }
}

void Exporter::popupDialogSaveImage(const QStringList &imagePaths)
{
    QFileDialog exportDialog;
    exportDialog.setWindowTitle(tr("Export Photos"));

    exportDialog.setFileMode(QFileDialog::Directory);
    exportDialog.setLabelText(QFileDialog::Reject, tr("Cancel"));
    exportDialog.setLabelText(QFileDialog::Accept, tr("Save"));
    exportDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).at(0));

    if (exportDialog.exec() == QDialog::Accepted) {
        QString exportdir = exportDialog.directory().absolutePath();

        int failcount = 0;
        bool bnewpath = false;
        for (int j(0); j < imagePaths.length(); j++) {
            if (utils::image::imageSupportRead(imagePaths[j])) {
                QString savePath =  QString("%1/%2.%3").arg(exportdir).arg(QFileInfo(imagePaths[j])
                                                                           .baseName()).arg(QFileInfo(imagePaths[j]).completeSuffix());
                QFileInfo fileinfo(savePath);
                if (fileinfo.exists()) {
                    if (!fileinfo.isDir()) {
                        m_exportImageDialog->showQuestionDialogs(savePath);
                    }
                    if (!m_exportImageDialog->getIsCover()) {
                        continue;
                    }
                }

                bool isSucceed = QFile::copy(imagePaths[j], savePath);
                emit dApp->signalM->sigExporting(imagePaths[j]);
                if (!isSucceed) {
                    failcount ++;
                } else {
                    bnewpath =  true;
                }

            } else {
                failcount ++;
                continue;
            }
        }
        if (failcount == imagePaths.length()) {
            emit dApp->signalM->ImgExportFailed();
        } else {
            if (bnewpath)
                emit dApp->signalM->ImgExportSuccess();
            emit dApp->signalM->sigRestoreStatus();
        }
    }
}

void Exporter::initValidFormatMap()
{
    m_picFormatMap.insert("jpeg", "JPEG (*.jpeg)");
    m_picFormatMap.insert("jpg", "JPG (*.jpg)");
    m_picFormatMap.insert("bmp", "BMP (*.bmp)");
    m_picFormatMap.insert("png", "PNG (*.png)");

    m_picFormatMap.insert("ppm", "PGM (*.ppm)");
    m_picFormatMap.insert("xbm", "XBM (*.xbm)");
    m_picFormatMap.insert("xpm", "XPM (*.xpm)");

}
