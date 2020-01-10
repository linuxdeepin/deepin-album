/*
 * Copyright (C) 2019 ~ %YEAR% Deepin Technology Co., Ltd.
 *
 * Author:     Renran
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

#include <DFileDialog>
#include <DDialog>
#include <DMessageBox>

#include <QFormLayout>
#include <QImageWriter>
#include <QStandardPaths>
#include <QPdfWriter>
#include <QPainter>

#include <QDebug>
#include <QDateTime>


const QSize DIALOG_SIZE = QSize(380, 280);
const QSize LINE_EDIT_SIZE = QSize(250, 35);

CExportImageDialog::CExportImageDialog(DWidget *parent)
    : DDialog(parent)
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

int CExportImageDialog::getImageType() const
{
    return  m_formatCombox->currentIndex();
}

QString CExportImageDialog::getSavePath() const
{
    return m_savePath + "/" + m_fileNameEdit->text().trimmed();
}

QString CExportImageDialog::getImageFormate() const
{
    return  m_saveFormat;
}

int CExportImageDialog::getQuality() const
{
    return m_quality;
}

void CExportImageDialog::setPicFileName(QString strFileName)
{
    QString name = strFileName.mid(0, strFileName.lastIndexOf("."));
    m_fileNameEdit->setText(name);
}

void CExportImageDialog::setGifType(QString strFilePath)
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

//    DLabel *titleLabel = new DLabel(tr("Export"), this);
//    titleLabel->setFixedSize(DIALOG_SIZE.width(), 40);

//    QHBoxLayout *titleLayout = new QHBoxLayout(this);
//    titleLayout->setSpacing(0);
//    titleLayout->setMargin(0);
//    titleLayout->addWidget(logoLable, 0, Qt::AlignLeft);
//    titleLayout->addWidget(titleLabel, 0, Qt::AlignHCenter);


//    titleLabel->move(0, 0);
//    titleLabel->setAlignment(Qt::AlignCenter);


    m_fileNameEdit = new DLineEdit(this);
    m_fileNameEdit->setFixedSize(LINE_EDIT_SIZE);
    m_fileNameEdit->setClearButtonEnabled(false);


    m_savePathCombox = new DComboBox(this);
    m_savePathCombox->insertItem(Pictures, tr("Pictures"));
    m_savePathCombox->insertItem(Documents, tr("Documents"));
    m_savePathCombox->insertItem(Downloads, tr("Downloads"));
    m_savePathCombox->insertItem(Desktop, tr("Desktop"));
    m_savePathCombox->insertItem(Videos, tr("Videos"));
    m_savePathCombox->insertItem(Music, tr("Music"));
    m_savePathCombox->insertItem(UsrSelect, tr("Select other directories"));

    //暂时使用中文字符串，后续需要做翻译
//    m_savePathCombox->insertItem(Pictures, tr("图片"));
//    m_savePathCombox->insertItem(Documents, tr("文档"));
//    m_savePathCombox->insertItem(Downloads, tr("下载"));
//    m_savePathCombox->insertItem(Desktop, tr("桌面"));
//    m_savePathCombox->insertItem(Videos, tr("视频"));
//    m_savePathCombox->insertItem(Music, tr("音乐"));
//    m_savePathCombox->insertItem(UsrSelect, tr("选择其他文件夹"));
    m_savePathCombox->setFixedSize(LINE_EDIT_SIZE);


    m_formatCombox = new DComboBox(this);
    m_formatCombox->insertItem(JPG, "jpg");
    m_formatCombox->insertItem(JPG, "jpeg");
    m_formatCombox->insertItem(PNG, "png");
    m_formatCombox->insertItem(BMP, "bmp");
    m_formatCombox->insertItem(BMP, "pgm");
    m_formatCombox->insertItem(TIF, "xbm");
    m_formatCombox->insertItem(PDF, "xpm");
    m_formatCombox->setFixedSize(LINE_EDIT_SIZE);

    m_qualitySlider = new DSlider(Qt::Horizontal, this);
    m_qualitySlider->setMinimum(1);
    m_qualitySlider->setMaximum(100);
    m_qualitySlider->setValue(100);
    m_qualitySlider->setFixedSize(QSize(100, LINE_EDIT_SIZE.height()));

    m_qualityLabel = new DLabel(this);
    m_qualityLabel->setFixedHeight(LINE_EDIT_SIZE.height());

    QHBoxLayout *qualityHLayout = new QHBoxLayout;
    qualityHLayout->setMargin(0);
    qualityHLayout->setSpacing(0);
    qualityHLayout->addSpacing(3);
    qualityHLayout->addWidget(m_qualitySlider);
    qualityHLayout->addSpacing(20);
    qualityHLayout->addWidget(m_qualityLabel);

    DWidget *contentWidget = new DWidget(this);
//    contentWidget->setStyleSheet("background-color: rgb(255, 0, 0);");
    contentWidget->setContentsMargins(0, 0, 0, 0);
    QFormLayout *fLayout = new QFormLayout(contentWidget);
    fLayout->setFormAlignment(Qt::AlignJustify);
    fLayout->setHorizontalSpacing(10);
    fLayout->addRow(tr("Name:"), m_fileNameEdit);
    fLayout->addRow(tr("Save to:"), m_savePathCombox);
    fLayout->addRow(tr("Format:"), m_formatCombox);
    fLayout->addRow(tr("Quality:"), qualityHLayout);
//    fLayout->addRow(tr("文件名:"), m_fileNameEdit);
//    fLayout->addRow(tr("保存到:"), m_savePathCombox);
//    fLayout->addRow(tr("文件格式:"), m_formatCombox);
//    fLayout->addRow(tr("图片质量:"), qualityHLayout);
    addContent(contentWidget);

//    addButton(tr("取消"), false, DDialog::ButtonNormal);
//    addButton(tr("保存"), true, DDialog::ButtonRecommend);
    addButton(tr("Cancel"), false, DDialog::ButtonNormal);
    addButton(tr("Save"), true, DDialog::ButtonRecommend);

    m_questionDialog = new DDialog(this);
    m_questionDialog->setModal(true);
    m_questionDialog->addButtons(QStringList() << tr("Cancel") << tr("Replace"));
    m_questionDialog->setFixedSize(400, 170);

    m_emptyWarningDialog = new DDialog(this);
    m_emptyWarningDialog->setModal(true);
    m_emptyWarningDialog->addButtons(QStringList() << tr("OK"));
    m_emptyWarningDialog->setFixedSize(400, 170);



//    setLayout(titleLayout);
}

void CExportImageDialog::initConnection()
{
    connect(m_savePathCombox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotOnSavePathChange(int)));
    connect(m_formatCombox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotOnFormatChange(int)));
    connect(this, SIGNAL(buttonClicked(int, const QString & )), this, SLOT(slotOnDialogButtonClick(int, const QString & )));
    connect(m_qualitySlider, SIGNAL(valueChanged(int)), this, SLOT(slotOnQualityChanged(int)));
    connect(m_questionDialog, SIGNAL(buttonClicked(int, const QString & )), this, SLOT(slotOnQuestionDialogButtonClick(int, const QString & )));
    connect(m_emptyWarningDialog, SIGNAL(buttonClicked(int, const QString & )), this, SLOT(slotOnEmptyWarningDialogButtonClick(int, const QString & )));

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
    switch (index) {
    case PDF:
    case BMP:
    case TIF:
        m_qualitySlider->setValue(100);
        m_qualitySlider->setEnabled(false);
        break;
    case JPG:
    case PNG:
        m_qualitySlider->setEnabled(true);
        break;
    default:
        break;
    }

    m_saveFormat = m_formatCombox->itemText(index);

//    QString name = m_fileNameEdit->text().trimmed();

//    if ("" != name) {

//        name = name.mid(0, name.lastIndexOf("."));

//        name += m_saveFormat;

//        m_fileNameEdit->setText(name);
//    }
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
        QString completePath = m_savePath + "/" + filename;
        if (QFileInfo(completePath).exists()) {
            hide();
            showQuestionDialog(completePath);
        } else {
            bool savere = doSave();
            hide();
            if (savere) {
                emit dApp->signalM->ImgExportSuccess();
            } else {
                emit dApp->signalM->ImgExportFailed();

            }
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
        QString completePath = m_savePath + "/" + m_fileNameEdit->text().trimmed();
        if (doSave()) {
            emit dApp->signalM->ImgExportSuccess();
        } else {
            emit dApp->signalM->ImgExportFailed();
        }
    }
    m_questionDialog->hide();
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
        m_savePathCombox->setCurrentText(fileDir);
    }
}

void CExportImageDialog::showEmptyWarningDialog()
{
    m_emptyWarningDialog->clearContents();
    DWidget *wid = new DWidget();
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

//    m_questionDialog->setMessage((QString(tr("%1 \already exists, do you want to replace?")).arg(path)));
    m_emptyWarningDialog->show();
}

void CExportImageDialog::showQuestionDialog(const QString &path)
{
    m_questionDialog->clearContents();
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

//    m_questionDialog->setMessage((QString(tr("%1 \already exists, do you want to replace?")).arg(path)));
    m_questionDialog->show();
}

bool CExportImageDialog::doSave()
{
    QString completePath = m_savePath + "/" + m_fileNameEdit->text().trimmed() + "." + m_saveFormat;
    if (tr("gif") == m_saveFormat) {
        if ("" != gifpath) {
            QFile::copy(gifpath, completePath);
        }
    } else {
        bool isSuccess = m_saveImage.save(completePath, m_saveFormat.toUpper().toLocal8Bit().data(), m_quality);
        qDebug() << "!!!!!!!!!" << isSuccess << "::" << completePath << "::" << m_saveFormat;
        return isSuccess;
    }
    return true;
}
