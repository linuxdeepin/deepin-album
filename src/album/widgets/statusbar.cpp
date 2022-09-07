// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "statusbar.h"
#include "ac-desktop-define.h"
#include <QGraphicsDropShadowEffect>
#include <QItemSelectionModel>


const int MAINWINDOW_MIN_WIDTH = 600;
const int MINADJUST_WIDTH = 720;
const int RIGHTMARGIN_MAX = 100;
const int RIGHTMARGIN_NORMAL = 0;
const int BOTTOMMARGIN_NORMAL = 3;
StatusBar::StatusBar(QWidget *parent)
    : DBlurEffectWidget(parent), m_pAllPicNumLabel(nullptr), m_pSlider(nullptr)
    , m_pstacklabel(nullptr), m_pimporting(nullptr), TextLabel(nullptr)
    , m_pStackedWidget(nullptr), loadingicon(nullptr), m_allPicNum(0)
    , interval(0), pic_count(0), m_index(0)
{
    initUI();
}

void StatusBar::initUI()
{
    setFixedHeight(27);

    m_allPicNum = DBManager::instance()->getImgsCount();

    m_pAllPicNumLabel = new DLabel();
    AC_SET_OBJECT_NAME(m_pAllPicNumLabel, All_Pic_Count);
    AC_SET_ACCESSIBLE_NAME(m_pAllPicNumLabel, All_Pic_Count);
    m_pAllPicNumLabel->setEnabled(false);
    m_pAllPicNumLabel->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T8, QFont::Normal));
    m_pAllPicNumLabel->setAlignment(Qt::AlignCenter);

    m_pimporting = new DWidget(this);
    TextLabel = new DLabel();
    TextLabel->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T8));

    TextLabel->setText("");
    TextLabel->adjustSize();
    TextLabel->setEnabled(false);
    loadingicon = new DSpinner(m_pimporting);
    loadingicon->hide();
    loadingicon->setFixedSize(20, 20);

    m_pStackedWidget = new DStackedWidget(this);
    m_pStackedWidget->addWidget(m_pAllPicNumLabel);
    m_pStackedWidget->addWidget(TextLabel);

    m_pSlider = new DSlider(Qt::Horizontal, this);
    m_pSlider->slider()->setFocusPolicy(Qt::NoFocus);
    AC_SET_OBJECT_NAME(m_pSlider, Thumbnail_Slider);
    AC_SET_ACCESSIBLE_NAME(m_pSlider, Thumbnail_Slider);
    m_pSlider->setFixedWidth(180);
    m_pSlider->setFixedHeight(27);
    m_pSlider->setMinimum(0);
    m_pSlider->setMaximum(9);
    m_pSlider->slider()->setSingleStep(1);
    m_pSlider->slider()->setTickInterval(1);
    m_pSlider->setValue(2);

    QHBoxLayout *pHBoxLayout = new QHBoxLayout();
    pHBoxLayout->setContentsMargins(0, 0, 0, 3);
    pHBoxLayout->addWidget(m_pStackedWidget, Qt::AlignCenter);
    this->setLayout(pHBoxLayout);

    initConnections();
}

void StatusBar::initConnections()
{
    qRegisterMetaType<QStringList>("QStringList &");
    connect(dApp->signalM, &SignalManager::updateStatusBarImportLabel, this, [ = ](QStringList paths, int count, QString album) {
        if (isVisible()) {
            imgpaths = paths;
            pic_count = count;
            m_bcustalbum = !album.isEmpty();
            m_alubm = album;

            TextLabel->setAlignment(Qt::AlignCenter);
            TextLabel->adjustSize();
            m_pStackedWidget->setCurrentIndex(1);
            interval = startTimer(3);

        }
    });
    // 处理导入图片完成后，弹出重复照片提示
    connect(dApp->signalM, &SignalManager::RepeatImportingTheSamePhotos, this, [ = ](QStringList importPaths, QStringList duplicatePaths, int UID) {
        Q_UNUSED(UID)
        // 导入的照片不全是重复照片提示
        if (importPaths.size() > 0 && duplicatePaths.size() > 0) {
            m_baddDuplicatePhotos = true;
        }
    });

    connect(dApp->signalM, &SignalManager::sigExporting, this, [ = ](QString path) {
        if (isVisible()) {
            m_pStackedWidget->setCurrentIndex(1);
            QString string = tr("Exporting: %1");
            TextLabel->setAlignment(Qt::AlignCenter);
            TextLabel->setText(string.arg(path));
            TextLabel->adjustSize();
            QTime time;
            time.start();
            while (time.elapsed() < 5)
                QCoreApplication::processEvents();
        }
    });
    connect(dApp->signalM, &SignalManager::sigExporting, this, [ = ](QString path) {
        Q_UNUSED(path);
        if (isVisible()) {
            m_pStackedWidget->setCurrentIndex(0);
        }
    });
}

void StatusBar::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED(e);
    if (width() < MAINWINDOW_MIN_WIDTH) {
        if (topLevelWidget()->width() < MINADJUST_WIDTH)
            layout()->setContentsMargins(0, 0, RIGHTMARGIN_MAX, BOTTOMMARGIN_NORMAL);
        else
            layout()->setContentsMargins(0, 0, RIGHTMARGIN_NORMAL, BOTTOMMARGIN_NORMAL);
    }
    m_pSlider->move(width() - 214, -1);
}

void StatusBar::paintEvent(QPaintEvent *event)
{
    setMaskColor(MaskColorType::AutoColor);

    QPalette palette = m_pAllPicNumLabel->palette();
    QPalette palettebackground = this->palette();
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::DarkType) {
        QColor backcolor(192, 198, 212);
        backcolor.setAlphaF(0.7);
        palette.setColor(QPalette::WindowText, backcolor);

    } else {
        QColor backcolor(98, 110, 136);
        backcolor.setAlphaF(0.7);
        palette.setColor(QPalette::WindowText, backcolor);

    }

    m_pAllPicNumLabel->setPalette(palette);
    return DBlurEffectWidget::paintEvent(event);
}
//根据照片与视频数量刷新提示
void StatusBar::resetSelectedStatue(int photosCount, int videosCount)
{
    //只选择照片
    if (photosCount > 0 && videosCount == 0) {
        QString photosStr;
        if (photosCount == 1) {
            photosStr = QObject::tr("1 photo selected");
            m_pAllPicNumLabel->setText(photosStr);
        } else {
            photosStr = QObject::tr("%n photos selected", "", photosCount);
            m_pAllPicNumLabel->setText(photosStr);
        }
    } else if (photosCount == 0 && videosCount > 0) {
        QString videosStr;
        if (videosCount == 1) {
            videosStr = QObject::tr("1 video selected");
            m_pAllPicNumLabel->setText(videosStr);
        } else {
            videosStr = QObject::tr("%n videos selected", "", videosCount);
            m_pAllPicNumLabel->setText(videosStr);
        }
    } else if (photosCount > 0 && videosCount > 0) {
        QString totalStr = QObject::tr("%n items selected", "", (videosCount + photosCount));
        m_pAllPicNumLabel->setText(totalStr);
    }
}

void StatusBar::resetUnselectedStatue(int photosCount, int videosCount)
{
    QString photosStr;
    if (photosCount == 1) {
        photosStr = tr("1 photo");
    } else if (photosCount > 1) {
        photosStr = tr("%n photos", "", photosCount);
    }

    QString videosStr;
    if (videosCount == 1) {
        videosStr = tr("1 video");
    } else if (videosCount > 1) {
        videosStr = tr("%n videos", "", videosCount);
    }

    if (photosCount > 0 && videosCount == 0) {
        m_pAllPicNumLabel->setText(photosStr);
    } else if (photosCount == 0 && videosCount > 0) {
        m_pAllPicNumLabel->setText(videosStr);
    } else if (photosCount > 0 && videosCount > 0) {
        m_pAllPicNumLabel->setText(photosStr + " " + videosStr);
    }
}

void StatusBar::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == interval) {
        loadingicon->move(TextLabel->x() + 102, 0);
        m_pStackedWidget->setCurrentIndex(1);
        QString string = tr("Importing: %1");

        if (imgpaths.count() == 1) {
            m_index = 0;
            killTimer(interval);
            interval = 0;
            m_pStackedWidget->setCurrentIndex(0);
            if (m_bcustalbum) {
                emit dApp->signalM->sigAddToAlbToast(m_alubm);
                if (m_baddDuplicatePhotos) {
                    m_baddDuplicatePhotos = false;
                }
            }
        } else {
            if (m_index >= imgpaths.count() - 1) {
                m_index = 0;
                killTimer(interval);
                interval = 0;
                if (m_bcustalbum) {
                    emit dApp->signalM->sigAddToAlbToast(m_alubm);
                    if (m_baddDuplicatePhotos) {
                        m_baddDuplicatePhotos = false;
                    }
                } else if (1 == pic_count) {
                    if (m_baddDuplicatePhotos) {
                        m_baddDuplicatePhotos = false;
                    }
                } else {
                    emit dApp->signalM->ImportFailed();
                }

                QTime time;
                time.start();
                while (time.elapsed() < 500)
                    QCoreApplication::processEvents();

                m_pStackedWidget->setCurrentIndex(0);
            } else {
                m_pStackedWidget->setCurrentIndex(0);
                m_index++;
            }
        }
    }
}
