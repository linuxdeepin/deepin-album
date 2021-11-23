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
#ifndef CEXPORTIMAGEDIALOG_H
#define CEXPORTIMAGEDIALOG_H


#include <DWidget>
#include <DDialog>
#include <DLineEdit>
#include <DComboBox>
#include <DLabel>
#include <DSlider>

#include "controller/signalmanager.h"
#include "application.h"

DWIDGET_USE_NAMESPACE

class CExportImageDialog : public DDialog
{
    Q_OBJECT
public:
    enum ESaveFormat {
        JPG,
        JPEG,
        PNG,
        BMP,
        PGM,
        XBM,
        XPM
    };
    enum ESavePath {
        Pictures,
        Documents,
        Downloads,
        Desktop,
        Videos,
        Music,
        UsrSelect,
        Other
    };

public:
    explicit CExportImageDialog(DWidget *parent = nullptr);
    ~CExportImageDialog() override;
    void showMe(const QPixmap &pixmap);
//    int getImageType() const;
//    QString getSavePath() const;
//    QString getImageFormate() const;
//    int getQuality() const;
    void setPicFileName(QString strFileName);
    void setGifType(const QString &strFilePath);
    void removeGifType();
    void showEvent(QShowEvent *evet) override;
    void keyPressEvent(QKeyEvent *e) override;
    void showQuestionDialog(const QString &path, const QString &srcpath = "");
    //多选时的弹窗
    void showQuestionDialogs(const QString &path);
    //获取是否已经覆盖
    bool getIsCover();
    void showDirChoseDialog();
    void showEmptyWarningDialog();
private slots:
    void slotOnSavePathChange(int index);
    void slotOnFormatChange(int index);
    void slotOnDialogButtonClick(int index, const QString &text);
    void slotOnQuestionDialogButtonClick(int index, const QString &text);
    //多选
    void slotOnQuestionDialogButtonClicks(int index, const QString &text);
    void slotOnEmptyWarningDialogButtonClick(int, const QString &);
    void slotOnQualityChanged(int value);

private:
    DLineEdit *m_fileNameEdit;
    DComboBox *m_savePathCombox;
    DComboBox *m_formatCombox;
    DSlider *m_qualitySlider;
    DLabel *m_qualityLabel;

//    QString m_fileName;
    QString m_savePath;
    QString m_saveFormat;
    int m_quality;

    DDialog *m_questionDialog;
    DDialog *m_questionDialogs; //多选导出时，文件存在覆盖确认窗
    DDialog *m_emptyWarningDialog;
    QPixmap m_saveImage;
    QString gifpath;
    QString m_CoverFilepath;//待覆盖的路径
    bool m_isCover;//确认是否覆盖

private:
    void initUI();
    void initConnection();
    bool doSave();
};

#endif // CEXPORTIMAGEDIALOG_H
