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
#include "videoinfodialog.h"
#include "controller/signalmanager.h"
#include "utils/imageutils.h"
#include "imageengineapi.h"
#include "utils/unionimage.h"
#include "widgets/formlabel.h"

#include "dfmdarrowlineexpand.h"

#include <QFormLayout>
#include <QLocale>
#include <QApplication>
#include <QScrollBar>

#include <DTitlebar>
#include <DApplicationHelper>
#include <DFontSizeManager>
#include "application.h"
#include "mainwindow.h"
#include "movieservice.h"

struct MetaData {
    QString key;
    const char *name;
};

VideoInfoDialog::VideoInfoDialog(const QString &path, const QString &displayName, bool isTrash, QWidget *parent): DDialog(parent)
{
    QFont font;
    m_currentFontSize = DFontSizeManager::instance()->fontPixelSize(font);
    QLocale locale;
    if (locale.language() == QLocale::Chinese)  //语言为中文
        m_title_maxwidth = 60;
    else
        m_title_maxwidth = 108;
    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_displayName = displayName;
    m_isTrash = isTrash;
    initUI();
    setVideoInfo(path);
}

void VideoInfoDialog::setVideoInfo(const QString &path)
{
    //获取视频信息
    m_movieInfo = MovieService::instance()->getMovieInfo(QUrl::fromLocalFile(path));
    m_maxFieldWidth = width() - m_title_maxwidth - 20 * 2 - 10 * 2;
    updateBasicInfo();

    if (MovieService::instance()->ffmpegIsExist()) {
        updateCodecInfo();
        updateAudioInfo();
    }

    m_expandGroup.clear();

    if (m_containBaseInfo) {
        appendExpandWidget(tr("Basic info"));
        m_expandGroup.at(0)->setContent(m_basicInfoFrame);
        m_expandGroup.at(0)->setExpand(true);
    }

    if (MovieService::instance()->ffmpegIsExist()) {
        if (m_containCodecInfo) {
            appendExpandWidget(tr("Codec info"));
            int index = m_expandGroup.size() > 0 ? (m_expandGroup.size() - 1) : 0;
            m_expandGroup.at(index)->setContent(m_codecInfoFrame);
            m_expandGroup.at(index)->setExpand(true);
            //音频编码等信息默认隐藏
            m_expandGroup.at(index)->setExpand(false);//疑似DDrawer的BUG，直接false会显示不全
        }

        if (m_containAudioInfo) {
            appendExpandWidget(tr("Audio info"));
            int index = m_expandGroup.size() > 0 ? (m_expandGroup.size() - 1) : 0;
            m_expandGroup.at(index)->setContent(m_audioInfoFrame);
            m_expandGroup.at(index)->setExpand(true);
            m_expandGroup.at(index)->setExpand(false);//疑似DDrawer的BUG，直接false会显示不全
        }
    }

    //剔除多余不需要的焦点
    QList<QWidget *>allwidgetlist = this->findChildren<QWidget *>();
    for (int i = 0; i < allwidgetlist.size() ; i++) {
        allwidgetlist.at(i)->setFocusPolicy(Qt::NoFocus);
    }
    for (int j = 0; j < m_expandGroup.size(); j++) {
        QList<DIconButton *> expandlist = m_expandGroup.at(j)->findChildren<DIconButton *>();
        for (int k = 0; k < expandlist.size(); k++) {
            expandlist.at(k)->setFocusPolicy(Qt::TabFocus);
        }
    }
    QWidget *closeButton = this->findChild<QWidget *>("DTitlebarDWindowCloseButton");
    if (closeButton) {
        closeButton->setFocusPolicy(Qt::TabFocus);
    }
}

void VideoInfoDialog::updateCodecInfo()
{
//    clearLayout(m_basicInfoFrameLayout);
    QMap<QString, QString> codecInfoMap;
    QList<QString> codecInfoKeys;
    codecInfoMap[tr("Video CodecID")] = m_movieInfo.vCodecID;
    codecInfoKeys.append(tr("Video CodecID"));

    codecInfoMap[tr("Video CodeRate")] = m_movieInfo.vCodeRate == 0 ? "-" : QString::number(m_movieInfo.vCodeRate) + " kbps";
    codecInfoKeys.append(tr("Video CodeRate"));

    codecInfoMap[tr("FPS")] = m_movieInfo.fps == 0 ? "-" : QString::number(m_movieInfo.fps) + " fps";
    codecInfoKeys.append(tr("FPS"));

    codecInfoMap[tr("Proportion")] = m_movieInfo.proportion <= 0 ? "-" : QString::number(m_movieInfo.proportion);
    codecInfoKeys.append(tr("Proportion"));

    codecInfoMap[tr("Resolution")] = m_movieInfo.resolution;
    codecInfoKeys.append(tr("Resolution"));


    for (int i = 0; i < codecInfoKeys.size(); i++) {
        QString key = codecInfoKeys.at(i);
        QString value = codecInfoMap[key];
        if (value.isEmpty()) {
            continue;
        }

        m_containCodecInfo = true;
        SimpleFormField *field = new SimpleFormField;
        field->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        DFontSizeManager::instance()->bind(field, DFontSizeManager::T8);
        DPalette pa1 = DApplicationHelper::instance()->palette(field);
        pa1.setBrush(DPalette::Text, pa1.color(DPalette::TextTitle));
        field->setPalette(pa1);
        field->setText(SpliteText(value, field->font(), m_maxFieldWidth));

        SimpleFormLabel *title = new SimpleFormLabel(key);
        title->setMinimumHeight(field->minimumHeight());
//        title->setFixedWidth(m_title_maxwidth);
        title->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        //2020/03/13-xiaolong
        DFontSizeManager::instance()->bind(title, DFontSizeManager::T8);
        DPalette pa2 = DApplicationHelper::instance()->palette(title);
        pa2.setBrush(DPalette::Text, pa2.color(DPalette::TextTitle));
        title->setPalette(pa2);
        title->setText(SpliteText((key), title->font(), m_title_maxwidth));

        m_codecInfoFrameLayout->addRow(title, field);
        QFontMetrics fm(field->font());
    }
}

void VideoInfoDialog::updateAudioInfo()
{
    QMap<QString, QString> audioInfoMap;
    QList<QString> audioInfoKeys;
    audioInfoMap[tr("Audio CodecID")] = m_movieInfo.aCodeID;
    audioInfoKeys.append(tr("Audio CodecID"));

    audioInfoMap[tr("Audio CodeRate")] = m_movieInfo.aCodeRate == 0 ? "-" : QString::number(m_movieInfo.aCodeRate) + " kbps";
    audioInfoKeys.append(tr("Audio CodeRate"));

    audioInfoMap[tr("Audio digit")] = m_movieInfo.aDigit;
    audioInfoKeys.append(tr("Audio digit"));

    audioInfoMap[tr("Channels")] = m_movieInfo.channels == 0 ? "-" : QString::number(m_movieInfo.channels) + tr("Channel");
    audioInfoKeys.append(tr("Channels"));

    audioInfoMap[tr("Sampling")] = m_movieInfo.sampling == 0 ? "-" : QString::number(m_movieInfo.sampling) + " hz";
    audioInfoKeys.append(tr("Sampling"));

    for (int i = 0; i < audioInfoKeys.size(); i++) {
        QString key = audioInfoKeys.at(i);
        QString value = audioInfoMap[key];
        if (value.isEmpty()) {
            continue;
        }

        m_containAudioInfo = true;
        SimpleFormField *field = new SimpleFormField;
        field->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        DFontSizeManager::instance()->bind(field, DFontSizeManager::T8);
        DPalette pa1 = DApplicationHelper::instance()->palette(field);
        pa1.setBrush(DPalette::Text, pa1.color(DPalette::TextTitle));
        field->setPalette(pa1);
        field->setText(SpliteText(value, field->font(), m_maxFieldWidth));

        SimpleFormLabel *title = new SimpleFormLabel(key);
        title->setMinimumHeight(field->minimumHeight());
//        title->setFixedWidth(m_title_maxwidth);
        title->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        //2020/03/13-xiaolong
        DFontSizeManager::instance()->bind(title, DFontSizeManager::T8);
        DPalette pa2 = DApplicationHelper::instance()->palette(title);
        pa2.setBrush(DPalette::Text, pa2.color(DPalette::TextTitle));
        title->setPalette(pa2);
        title->setText(SpliteText((key), title->font(), m_title_maxwidth));

        m_audioInfoFrameLayout->addRow(title, field);
        QFontMetrics fm(field->font());
    }
}

void VideoInfoDialog::updateBasicInfo()
{
//    clearLayout(m_basicInfoFrameLayout);
    QMap<QString, QString> basicInfoMap;
    QList<QString> basicInfoKeys;
    basicInfoMap[tr("Name")] = m_displayName;
    basicInfoKeys.append(tr("Name"));

    QDateTime dateTime = m_movieInfo.creation;
//    basicInfoMap[tr("Date captured")] = m_movieInfo.creation;
    basicInfoMap[tr("Date captured")] = dateTime.toString("yyyy/MM/dd HH:mm");
    basicInfoKeys.append(tr("Date captured"));

    QFileInfo info(m_movieInfo.filePath);
    if (info.lastModified().isValid()) {
        basicInfoMap[tr("Date modified")] = info.lastModified().toString("yyyy/MM/dd HH:mm");
        basicInfoKeys.append(tr("Date modified"));
    } else if (info.birthTime().isValid()) {
        basicInfoMap[tr("Date modified")] = info.birthTime().toString("yyyy/MM/dd HH:mm");
        basicInfoKeys.append(tr("Date modified"));
    }
    basicInfoMap[tr("Type")] = m_movieInfo.fileType.toLower();
    basicInfoKeys.append(tr("Type"));

    basicInfoMap[tr("File size")] = m_movieInfo.sizeStr();
    basicInfoKeys.append(tr("File size"));

    basicInfoMap[tr("Duration")] = m_movieInfo.duration;
    basicInfoKeys.append(tr("Duration"));

    //trash目录不显示路径
    if (!m_isTrash) {
        basicInfoMap[tr("Path")] = m_movieInfo.filePath;
        basicInfoKeys.append(tr("Path"));
    }

    for (int i = 0; i < basicInfoKeys.size(); i++) {
        QString key = basicInfoKeys.at(i);
        QString value = basicInfoMap[key];
        if (value.isEmpty()) {
            continue;
        }

        m_containBaseInfo = true;
        SimpleFormField *field = new SimpleFormField;
        field->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        DFontSizeManager::instance()->bind(field, DFontSizeManager::T8);
        DPalette pa1 = DApplicationHelper::instance()->palette(field);
        pa1.setBrush(DPalette::Text, pa1.color(DPalette::TextTitle));
        field->setPalette(pa1);
        field->setText(SpliteText(value, field->font(), m_maxFieldWidth));

        SimpleFormLabel *title = new SimpleFormLabel(key);
        title->setMinimumHeight(field->minimumHeight());
        title->setFixedWidth(m_title_maxwidth);
        title->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        DFontSizeManager::instance()->bind(title, DFontSizeManager::T8);
        DPalette pa2 = DApplicationHelper::instance()->palette(title);
        pa2.setBrush(DPalette::Text, pa2.color(DPalette::TextTitle));
        title->setPalette(pa2);
        title->setText(SpliteText((key), title->font(), m_title_maxwidth));

        m_basicInfoFrameLayout->addRow(title, field);
    }
}

void VideoInfoDialog::appendExpandWidget(const QString &title)
{
    DFMDArrowLineExpand *expand = new DFMDArrowLineExpand;//DArrowLineExpand;
    expand->setTitle(title);
    initExpand(m_scrollAreaLayout, expand);
    m_expandGroup.push_back(expand);
}

void VideoInfoDialog::initExpand(QVBoxLayout *layout, DDrawer *expand)
{
    expand->setFixedHeight(22);
    QMargins cm = layout->contentsMargins();
    QRect rc1 = contentsRect();
    expand->setFixedWidth(rc1.width() - cm.left() - cm.right());
    // 去掉界面的横线
    expand->setExpandedSeparatorVisible(false);
    expand->setSeparatorVisible(false);
    layout->addWidget(expand, 0, Qt::AlignTop);

    DEnhancedWidget *hanceedWidget = new DEnhancedWidget(expand, this);
    connect(hanceedWidget, &DEnhancedWidget::heightChanged, hanceedWidget, [ = ]() {
        QRect rc = geometry();
        if (m_expandGroup.count() == 2) {
            rc.setHeight(qMin(615, contentHeight() + 15));
            setGeometry(rc);
        } else {
            rc.setHeight(qMin(615, contentHeight() + 25));
            setGeometry(rc);
        }
    });
}

int VideoInfoDialog::contentHeight() const
{
    int expandsHeight = 10;
    QList<DDrawer *>::const_iterator expand = m_expandGroup.cbegin();
    while (expand != m_expandGroup.cend()) {
        expandsHeight += (*expand)->height();
        ++expand;
    }
    return (50 + expandsHeight + contentsMargins().top() +
            contentsMargins().bottom());
}

QString VideoInfoDialog::SpliteText(const QString &text, const QFont &font, int nLabelSize)
{
    QFontMetrics fm(font);
    double dobuleTextSize = fm.horizontalAdvance(text);
    double dobuleLabelSize = nLabelSize;
    if (dobuleTextSize > dobuleLabelSize && dobuleLabelSize > 0 && dobuleTextSize < 10000) {
        double splitCount = dobuleTextSize / dobuleLabelSize;
        int nCount = int(splitCount + 1);
        QString textSplite;
        QString textTotal = text;
        for (int index = 0; index < nCount; ++index) {
            int nPos = 0;
            long nOffset = 0;
            for (int i = 0; i < text.size(); i++) {
                nOffset += fm.width(text.at(i));
                if (nOffset >= nLabelSize) {
                    nPos = i;
                    break;
                }
            }
            nPos = (nPos - 1 < 0) ? 0 : nPos - 1;
            QString qstrLeftData;
            if (nCount - 1 == index) {
                qstrLeftData = textTotal;
                textSplite += qstrLeftData;
            } else {
                qstrLeftData = textTotal.left(nPos);
                textSplite += qstrLeftData + "<br>";
            }
            textTotal = textTotal.mid(nPos);
        }
        return textSplite;
    } else {
        return text;
    }
}

bool VideoInfoDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (QApplication::activeWindow() != this) {
            this->close();
        }
    }
    return QWidget::event(event);
}

void VideoInfoDialog::initUI()
{
    //wayland背景透明问题
    DPalette imgInfoDlgPl = this->palette();
    QColor imgInfoDlgColor("#F7F7F7");
    imgInfoDlgColor.setAlphaF(0.8);
    imgInfoDlgPl.setColor(DPalette::Window, imgInfoDlgColor);
    this->setBackgroundRole(DPalette::Window);

    setFixedWidth(320);
    setMaximumHeight(540);
    setContentLayoutContentsMargins(QMargins(0, 0, 0, 0));
    //标题
    DLabel *title = new DLabel(this);
    title->setText(tr("Video info"));
    title->setGeometry(this->x() + (this->width() - title->width()) / 2, this->y(), 112, 50);
    title->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    DFontSizeManager::instance()->bind(title, DFontSizeManager::T6);
    DPalette pa = DApplicationHelper::instance()->palette(title);
    pa.setBrush(DPalette::Text, pa.color(DPalette::TextTitle));
    title->setPalette(pa);
    setContentsMargins(0, 0, 0, 0);
    //基本信息
    m_basicInfoFrame = new QFrame(this);
    m_basicInfoFrame->setFixedWidth(280);
    m_basicInfoFrameLayout = new QFormLayout();
    m_basicInfoFrameLayout->setVerticalSpacing(7);
    m_basicInfoFrameLayout->setHorizontalSpacing(16);
    m_basicInfoFrameLayout->setContentsMargins(10, 1, 7, 10);
    m_basicInfoFrameLayout->setLabelAlignment(Qt::AlignLeft);
    m_basicInfoFrame->setLayout(m_basicInfoFrameLayout);

    if (MovieService::instance()->ffmpegIsExist()) {
        //编码信息
        m_codecInfoFrame = new QFrame(this);
        m_codecInfoFrame->setFixedWidth(280);
        m_codecInfoFrameLayout = new QFormLayout();
        m_codecInfoFrameLayout->setVerticalSpacing(7);
        m_codecInfoFrameLayout->setHorizontalSpacing(16);
        m_codecInfoFrameLayout->setContentsMargins(10, 1, 7, 10);
        m_codecInfoFrameLayout->setLabelAlignment(Qt::AlignLeft);
        m_codecInfoFrame->setLayout(m_codecInfoFrameLayout);
        //音频流信息
        m_audioInfoFrame = new QFrame(this);
        m_audioInfoFrame->setFixedWidth(280);
        m_audioInfoFrameLayout = new QFormLayout();
        m_audioInfoFrameLayout->setVerticalSpacing(7);
        m_audioInfoFrameLayout->setHorizontalSpacing(16);
        m_audioInfoFrameLayout->setContentsMargins(10, 1, 7, 10);
        m_audioInfoFrameLayout->setLabelAlignment(Qt::AlignLeft);
        m_audioInfoFrame->setLayout(m_audioInfoFrameLayout);
    }

    DWidget *content = new DWidget();
    addContent(content);
    m_mainLayout = new QVBoxLayout;
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setMargin(0);
    m_mainLayout->setSpacing(0);
    content->setLayout(m_mainLayout);

    m_scrollArea = new DScrollArea();
    DPalette palette = m_scrollArea->viewport()->palette();
    palette.setBrush(DPalette::Background, Qt::NoBrush);
    m_scrollArea->viewport()->setPalette(palette);
    m_scrollArea->setFrameShape(QFrame::Shape::NoFrame);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    m_scrollArea->viewport()->installEventFilter(this);

    QFrame *infoframe = new QFrame;
    m_scrollAreaLayout = new QVBoxLayout;
    m_scrollAreaLayout->setContentsMargins(10, 0, 10, 0);
    m_scrollAreaLayout->setSpacing(10);
    infoframe->setLayout(m_scrollAreaLayout);
    m_scrollArea->setWidget(infoframe);

    QVBoxLayout *scrolllayout = new QVBoxLayout;
    scrolllayout->addWidget(m_scrollArea);
    m_mainLayout->insertLayout(1, scrolllayout, 1);
}

