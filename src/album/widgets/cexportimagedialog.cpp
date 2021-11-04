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
#include "cexportimagedialog.h"
#include "mainwindow.h"

#include <DFileDialog>
#include <DDialog>
#include <DMessageBox>
#include <DTitlebar>
#include <DComboBox>

#include <QFormLayout>
#include <QImageWriter>
#include <QStandardPaths>
#include <QPdfWriter>
#include <QPainter>
#include <QDesktopWidget>

#include <QDebug>
#include <QDateTime>
#include <QTimer>


const QSize DIALOG_SIZE = QSize(380, 280);
const QSize LINE_EDIT_SIZE = QSize(250, 35);

CExportImageDialog::CExportImageDialog(DWidget *parent)
    : DDialog(parent), m_fileNameEdit(nullptr), m_savePathCombox(nullptr)
    , m_formatCombox(nullptr), m_qualitySlider(nullptr), m_qualityLabel(nullptr)
    , m_quality(0), m_questionDialog(nullptr), m_questionDialogs(nullptr), m_emptyWarningDialog(nullptr)
{
    initUI();
    initConnection();
}

CExportImageDialog::~CExportImageDialog()
{

}

void CExportImageDialog::showMe(const QPixmap &pixmap)
{
    m_saveImage = pixmap;

    if (m_savePathCombox->count() == Other + 1) {
        m_savePathCombox->blockSignals(true);
        m_savePathCombox->removeItem(Other);
    }
    m_savePathCombox->blockSignals(false);

    m_savePathCombox->setCurrentIndex(Pictures);
    m_formatCombox->setCurrentIndex(JPG);
    m_qualitySlider->setValue(100);


    slotOnSavePathChange(Pictures);
    slotOnFormatChange(JPG);
    slotOnQualityChanged(m_qualitySlider->value());

    show();
}

void CExportImageDialog::setPicFileName(QString strFileName)
{
    QString name = strFileName.mid(0, strFileName.lastIndexOf("."));
    m_fileNameEdit->setText(name);
}

void CExportImageDialog::setGifType(const QString &strFilePath)
{
    gifpath = strFilePath;
    m_formatCombox->addItem(tr("gif"));
    m_formatCombox->setCurrentText(tr("gif"));
}

void CExportImageDialog::removeGifType()
{
    gifpath = "";
    for (int i = 0; i < m_formatCombox->count(); i++) {
        if (tr("gif") == m_formatCombox->itemText(i)) {
            m_formatCombox->removeItem(i);
            return;
        }
    }
}

void CExportImageDialog::showEvent(QShowEvent *evet)
{
    // 初始编辑框焦点设置
    m_fileNameEdit->lineEdit()->selectAll();
    m_fileNameEdit->lineEdit()->setFocus();     //设置焦点
    return QWidget::showEvent(evet);
}

void CExportImageDialog::keyPressEvent(QKeyEvent *e)
{
    // combox空间enter展开选项
    if (e->key() == Qt::Key_Return) {
        if (m_savePathCombox->hasFocus()) {
            m_savePathCombox->showPopup();
        } else if (m_formatCombox->hasFocus()) {
            m_formatCombox->showPopup();
        }
        e->accept();
    } else if (e->key() == Qt::Key_Escape) {
        this->close();
    }
    QWidget::keyPressEvent(e);
}

void CExportImageDialog::initUI()
{
    setFixedSize(DIALOG_SIZE);
    setModal(true);

    setContentsMargins(0, 0, 0, 0);

    DLabel *logoLable = new DLabel(this);
    QIcon icon = QIcon::fromTheme("deepin-album");
    logoLable->setPixmap(icon.pixmap(QSize(30, 30)));

    logoLable->move(12, 7);
    logoLable->setAlignment(Qt::AlignLeft);

    setWindowTitle(tr("Export"));

    m_fileNameEdit = new DLineEdit(this);
    //m_fileNameEdit->setFixedSize(LINE_EDIT_SIZE);
    m_fileNameEdit->setClearButtonEnabled(false);
//    m_fileNameEdit->setFocusPolicy(Qt::ClickFocus);

    connect(m_fileNameEdit, &DLineEdit::editingFinished, this, [ = ] {
        QString arg = m_fileNameEdit->text();
        int len = arg.toLocal8Bit().size();
        int filemaxlen = 251;
        if (m_saveFormat.toUpper().compare("JPEG") == 0)
            filemaxlen -= 1;
        QString Interceptstr;
        if (len > filemaxlen)
        {
            int num = 0;
            int pos = 0;
            for (; pos < arg.size(); pos++) {
                if (arg.at(pos) >= 0x4e00 && arg.at(pos) <= 0x9fa5) {
                    num += 3;
                    if (num >= filemaxlen) {
                        break;
                    }
                } else if (num < filemaxlen) {
                    num += 1;
                } else {
                    break;
                }
            }
            Interceptstr = arg.left(pos);
            m_fileNameEdit->setText(Interceptstr);
        }
    });

    m_savePathCombox = new DComboBox(this);
    m_savePathCombox->setFocusPolicy(Qt::TabFocus);
    m_savePathCombox->insertItem(Pictures, tr("Pictures"));
    m_savePathCombox->insertItem(Documents, tr("Documents"));
    m_savePathCombox->insertItem(Downloads, tr("Downloads"));
    m_savePathCombox->insertItem(Desktop, tr("Desktop"));
    m_savePathCombox->insertItem(Videos, tr("Videos"));
    m_savePathCombox->insertItem(Music, tr("Music"));
    m_savePathCombox->insertItem(UsrSelect, tr("Select other directories"));

    //m_savePathCombox->setFixedSize(LINE_EDIT_SIZE);


    m_formatCombox = new DComboBox(this);
    m_formatCombox->setFocusPolicy(Qt::TabFocus);
//    m_formatCombox->insertItem(JPG, "jpg");
//    m_formatCombox->insertItem(JPG, "jpeg");
//    m_formatCombox->insertItem(PNG, "png");
//    m_formatCombox->insertItem(BMP, "bmp");
//    m_formatCombox->insertItem(BMP, "pgm");
//    m_formatCombox->insertItem(TIF, "xbm");
//    m_formatCombox->insertItem(PDF, "xpm");
    m_formatCombox->insertItem(JPG, "jpg");
    m_formatCombox->insertItem(JPEG, "jpeg");
    m_formatCombox->insertItem(PNG, "png");
    m_formatCombox->insertItem(BMP, "bmp");
    m_formatCombox->insertItem(PGM, "pgm");
    m_formatCombox->insertItem(XBM, "xbm");
    m_formatCombox->insertItem(XPM, "xpm");
    //m_formatCombox->setFixedSize(LINE_EDIT_SIZE);

    m_qualitySlider = new DSlider(Qt::Horizontal, this);
    m_qualitySlider->slider()->setFocusPolicy(Qt::TabFocus);
    m_qualitySlider->setMinimum(1);
    m_qualitySlider->setMaximum(100);
    m_qualitySlider->setValue(100);
    //m_qualitySlider->setFixedSize(QSize(100, LINE_EDIT_SIZE.height()));

    m_qualityLabel = new DLabel(this);
    //m_qualityLabel->setFixedHeight(LINE_EDIT_SIZE.height());

    QHBoxLayout *qualityHLayout = new QHBoxLayout;
    qualityHLayout->setMargin(0);
    qualityHLayout->setSpacing(0);
    qualityHLayout->addSpacing(3);
    qualityHLayout->addWidget(m_qualitySlider);
    qualityHLayout->addSpacing(20);
    qualityHLayout->addWidget(m_qualityLabel);

    DWidget *contentWidget = new DWidget(this);
    contentWidget->setContentsMargins(0, 0, 0, 0);
    QFormLayout *fLayout = new QFormLayout(contentWidget);
    fLayout->setFormAlignment(Qt::AlignJustify);
    fLayout->setHorizontalSpacing(10);
    fLayout->addRow(tr("Name:"), m_fileNameEdit);
    fLayout->addRow(tr("Save to:"), m_savePathCombox);
    fLayout->addRow(tr("Format:"), m_formatCombox);
    fLayout->addRow(tr("Quality:"), qualityHLayout);

    addContent(contentWidget);

    addButton(tr("Cancel"), false, DDialog::ButtonNormal);
    addButton(tr("Save"), true, DDialog::ButtonRecommend);

    m_emptyWarningDialog = new DDialog(this);
    m_emptyWarningDialog->setModal(true);
    m_emptyWarningDialog->addButtons(QStringList() << tr("OK"));
    m_emptyWarningDialog->setFixedSize(400, 170);

    //剔除Titlebar焦点
    DTitlebar *titlebar = findChild<DTitlebar *>();
    if (titlebar) {
        titlebar->setFocusPolicy(Qt::ClickFocus);
    }

    QWidget *closeButton = findChild<QWidget *>("DTitlebarDWindowCloseButton");
    if (closeButton) {
        closeButton->setFocusPolicy(Qt::TabFocus);
    }
}

void CExportImageDialog::initConnection()
{
    connect(m_savePathCombox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotOnSavePathChange(int)));
    connect(m_formatCombox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotOnFormatChange(int)));
    connect(this, SIGNAL(buttonClicked(int, const QString &)), this, SLOT(slotOnDialogButtonClick(int, const QString &)));
    connect(m_qualitySlider, SIGNAL(valueChanged(int)), this, SLOT(slotOnQualityChanged(int)));
    connect(m_emptyWarningDialog, SIGNAL(buttonClicked(int, const QString &)), this, SLOT(slotOnEmptyWarningDialogButtonClick(int, const QString &)));
}

void CExportImageDialog::slotOnSavePathChange(int index)
{
    switch (index) {
    case Pictures:
        m_savePath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        break;
    case Documents:
        m_savePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        break;
    case Downloads:
        m_savePath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        break;
    case Desktop:
        m_savePath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        break;
    case Videos:
        m_savePath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
        break;
    case Music:
        m_savePath = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
        break;
    case UsrSelect:
        showDirChoseDialog();
        break;
    case Other:
        m_savePath = m_savePathCombox->itemText(index);
        break;
    default:
        m_savePath = "";
        break;
    }
}

void CExportImageDialog::slotOnFormatChange(int index)
{
    m_saveFormat = m_formatCombox->itemText(index);
    //when jpeg format,recalculate file name length ,because ".jpeg"==5
    if (m_saveFormat.toUpper().compare("JPEG") == 0) {
        QString arg = m_fileNameEdit->text();
        int len = arg.toLocal8Bit().size();
        QString Interceptstr;
        if (len > 250) {
            unsigned num = 0;
            int pos = 0;
            for (; pos < arg.size(); pos++) {
                if (arg.at(pos) >= 0x4e00 && arg.at(pos) <= 0x9fa5) {
                    num += 3;
                    if (num >= 250) {
                        break;
                    }
                } else if (num < 250) {
                    num += 1;
                } else {
                    break;
                }
            }
            Interceptstr = arg.left(pos);
            m_fileNameEdit->setText(Interceptstr);
        }
    }
}

void CExportImageDialog::slotOnDialogButtonClick(int index, const QString &text)
{
    Q_UNUSED(text)

    if (index == 1) {
        QString filename = m_fileNameEdit->text().trimmed();
        if ("" == filename) {
            hide();
            showEmptyWarningDialog();
            return;
        }
        QString completePath = m_savePath + "/" + filename.trimmed() + "." + m_saveFormat;
        QFileInfo fileinfo(completePath);
        if (fileinfo.exists()) {
            if (!fileinfo.isDir()) {
                hide();
                showQuestionDialog(completePath);
                return;
            }
        }
        bool savere = doSave();
        hide();
        if (savere) {
            emit dApp->signalM->ImgExportSuccess();
        } else {
            emit dApp->signalM->ImgExportFailed();

        }

    }
}

void CExportImageDialog::slotOnEmptyWarningDialogButtonClick(int index, const QString &text)
{
    Q_UNUSED(text);
    Q_UNUSED(index);
    m_emptyWarningDialog->hide();
}

void CExportImageDialog::slotOnQuestionDialogButtonClick(int index, const QString &text)
{
    Q_UNUSED(text);
    if (index == 1) {
        if (doSave()) {
            emit dApp->signalM->ImgExportSuccess();
        } else {
            emit dApp->signalM->ImgExportFailed();
        }
    }
}

void CExportImageDialog::slotOnQuestionDialogButtonClicks(int index, const QString &text)
{
    Q_UNUSED(text);
    m_isCover = false;
    if (index == 1) {
        QFile::remove(m_CoverFilepath);
        m_isCover = true;
    }
}

void CExportImageDialog::slotOnQualityChanged(int value)
{
    m_qualityLabel->setText(QString("%1%").arg(value));
    m_quality = value;
}

void CExportImageDialog::showDirChoseDialog()
{
    DFileDialog dialog(this);
    dialog.setDirectory(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    dialog.setViewMode(DFileDialog::Detail);
    dialog.setFileMode(DFileDialog::DirectoryOnly);
    if (dialog.exec()) {
        QString fileDir = dialog.selectedFiles().first();
        if (m_savePathCombox->count() < Other + 1) {
            m_savePathCombox->insertItem(Other, fileDir);
        } else {
            m_savePathCombox->setItemText(Other, fileDir);
        }
        QTimer::singleShot(50, this, [ = ] {
            m_savePathCombox->setCurrentText(fileDir);
        });
    }
}

void CExportImageDialog::showEmptyWarningDialog()
{
    m_emptyWarningDialog->clearContents();
    DWidget *wid = new DWidget();
    wid->setFocusPolicy(Qt::NoFocus);
//    DLabel *lab1 = new DLabel();
//    QFontMetrics elideFont(lab1->font());
    DLabel *lab2 = new DLabel();
    lab2->setText(tr("File name cannot be empty"));
    lab2->setAlignment(Qt::AlignCenter);

    QVBoxLayout *lay = new QVBoxLayout();
    lay->setContentsMargins(0, 0, 0, 0);
//    lay->addWidget(lab1);
    lay->addWidget(lab2);
    lay->addSpacing(100);
    wid->setLayout(lay);
    m_emptyWarningDialog->addContent(wid, Qt::AlignCenter);

    m_emptyWarningDialog->show();
}

void CExportImageDialog::showQuestionDialog(const QString &path, const QString &srcpath)
{
    //BUG#90251 规避焦点消失问题
    delete m_questionDialog;
    m_questionDialog = new DDialog(this);
    m_questionDialog->setFocusPolicy(Qt::NoFocus);
    m_questionDialog->setModal(true);
    m_questionDialog->addButtons({tr("Cancel"), tr("Replace")});
    m_questionDialog->setFixedSize(400, 170);
    connect(m_questionDialog, SIGNAL(buttonClicked(int, const QString &)), this, SLOT(slotOnQuestionDialogButtonClick(int, const QString &)));
    //BUG#90251

    DWidget *wid = new DWidget();
    DLabel *lab1 = new DLabel();
    QFontMetrics elideFont(lab1->font());
    lab1->setText(elideFont.elidedText(path, Qt::ElideRight, 255));
    lab1->setToolTip(path);

    DLabel *lab2 = new DLabel();
    lab2->setText(tr("already exists. Do you want to replace it?"));
    lab2->setAlignment(Qt::AlignCenter);

    QVBoxLayout *lay = new QVBoxLayout();
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(lab1);
    lay->addWidget(lab2);
    lay->addSpacing(100);
    wid->setLayout(lay);
    m_questionDialog->addContent(wid, Qt::AlignCenter);
    if (!srcpath.isEmpty()) {
        m_saveImage = QPixmap(srcpath);
    }
    m_savePath = path.left(path.lastIndexOf("/"));
//    m_questionDialog->setMessage((QString(tr("%1 \already exists, do you want to replace?")).arg(path)));
    m_questionDialog->move(dApp->getMainWindow()->x() + (dApp->getMainWindow()->width() - m_questionDialog->width()) / 2,
                           dApp->getMainWindow()->y() + (dApp->getMainWindow()->height() - m_questionDialog->height()) / 2);
    m_questionDialog->exec();
}

void CExportImageDialog::showQuestionDialogs(const QString &path)
{
    delete m_questionDialogs;
    m_questionDialogs = new DDialog(this);
    m_questionDialogs->setFocusPolicy(Qt::NoFocus);
    m_questionDialogs->setModal(true);
    m_questionDialogs->addButtons({tr("Cancel"), tr("Replace")});
    m_questionDialogs->setFixedSize(400, 170);
    m_CoverFilepath = path;
    connect(m_questionDialogs, SIGNAL(buttonClicked(int, const QString &)), this, SLOT(slotOnQuestionDialogButtonClicks(int, const QString &)));
    //BUG#90251

    DWidget *wid = new DWidget();
    DLabel *lab1 = new DLabel();
    QFontMetrics elideFont(lab1->font());
    lab1->setText(elideFont.elidedText(path, Qt::ElideRight, 255));
    lab1->setToolTip(path);

    DLabel *lab2 = new DLabel();
    lab2->setText(tr("already exists. Do you want to replace it?"));
    lab2->setAlignment(Qt::AlignCenter);

    QVBoxLayout *lay = new QVBoxLayout();
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(lab1);
    lay->addWidget(lab2);
    lay->addSpacing(100);
    wid->setLayout(lay);
    m_questionDialogs->addContent(wid, Qt::AlignCenter);
    m_questionDialogs->move(dApp->getMainWindow()->x() + (dApp->getMainWindow()->width() - m_questionDialogs->width()) / 2,
                            dApp->getMainWindow()->y() + (dApp->getMainWindow()->height() - m_questionDialogs->height()) / 2);
    m_questionDialogs->exec();
}

bool CExportImageDialog::getIsCover()
{
    return m_isCover;
}

bool CExportImageDialog::doSave()
{
    bool isSuccess = false;
    m_saveFormat = m_formatCombox->currentText();
    QString completePath = m_savePath + "/" + m_fileNameEdit->text().trimmed() + "." + m_saveFormat;
    if (tr("gif") == m_saveFormat) {
        if (gifpath.isEmpty()) {
            return isSuccess;
        }

        QFileInfo fileinfo(completePath);
        if (fileinfo.exists() && !fileinfo.isDir()) {
            //目标位置与原图位置相同则直接返回
            if (gifpath == completePath) {
                return true;
            }
            //目标位置与原图位置不同则先删除再复制
            if (QFile::remove(completePath)) {
                isSuccess = QFile::copy(gifpath, completePath);
            }
        } else {
            isSuccess = QFile::copy(gifpath, completePath);
        }
    } else {
        isSuccess = m_saveImage.save(completePath, m_saveFormat.toUpper().toLocal8Bit().data(), m_quality);
        qDebug() << "!!!!!!!!!" << isSuccess << "::" << completePath << "::" << m_saveFormat;
    }
    return isSuccess;
}
