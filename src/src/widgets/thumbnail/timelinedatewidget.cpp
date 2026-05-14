// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "timelinedatewidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFontMetrics>

#include <DFontSizeManager>
#include <dpalettehelper.h>
#include <QDebug>

static const int CONTROL_TEXT_PAD = 6;
static const int TITLE_LINE_SPACING = 4;
static const int TITLE_TOP_PAD = 10;
static const int IMPORT_TIMELINE_TOP_PAD = 8;
static const int TIMELINE_TITLEHEIGHT = 36;

TimeLineDateWidget::TimeLineDateWidget(QStandardItem *item, const QString &time, const QString &num)
    :  m_chooseBtn(nullptr), m_pDate(nullptr), m_pNumCheckBox(nullptr), m_currentItem(item)
{
    qDebug() << "Creating TimeLineDateWidget - time:" << time << "num:" << num;
    this->setContentsMargins(0, 0, 0, 0);

    //时间线日期
    m_pDate = new DLabel(this);
    DFontSizeManager::instance()->bind(m_pDate, DFontSizeManager::T3, QFont::DemiBold);
    QFont ft1 = DFontSizeManager::instance()->get(DFontSizeManager::T3);
    ft1.setFamily("Noto Sans CJK SC");
    m_pDate->setFont(ft1);
    m_pDate->setContentsMargins(0, 0, 0, 0);
    m_pDate->setText(time);
    int dateH = QFontMetrics(ft1).tightBoundingRect("Ayjg").height();
    int dateFixedH = dateH + CONTROL_TEXT_PAD;
    // bug76892 藏语占用更大高度
    if (QLocale::system().language() == QLocale::Tibetan)
        dateFixedH = qMax(dateFixedH, TIMELINE_TITLEHEIGHT + 25);
    m_pDate->setFixedHeight(dateFixedH);

    //数量
    m_pNumCheckBox = new DCheckBox(this);
    connect(m_pNumCheckBox, &DCheckBox::clicked, this, &TimeLineDateWidget::onCheckBoxCliked);
    DFontSizeManager::instance()->bind(m_pNumCheckBox, DFontSizeManager::T6, QFont::Normal);
    QFont ft2 = DFontSizeManager::instance()->get(DFontSizeManager::T6);
    ft2.setFamily("Noto Sans CJK SC");

    m_pNumCheckBox->setFont(ft2);
    m_pNumCheckBox->setContentsMargins(0, 0, 0, 0);
    m_pNumCheckBox->setText(num);
    int numH = QFontMetrics(ft2).tightBoundingRect("Ayjg").height();
    m_pNumCheckBox->setFixedHeight(numH + CONTROL_TEXT_PAD);

    m_pNum = new DLabel(this);
    DFontSizeManager::instance()->bind(m_pNum, DFontSizeManager::T6, QFont::Normal);
    m_pNum->setFont(ft2);
    m_pNum->setContentsMargins(0, 0, 0, 0);
    m_pNum->setText(num);
    m_pNum->setFixedHeight(numH + CONTROL_TEXT_PAD);

    onShowCheckBox(false);

    //选择按钮
    m_chooseBtn = new DCommandLinkButton(QObject::tr("Select"));
    DFontSizeManager::instance()->bind(m_chooseBtn, DFontSizeManager::T5);
    m_chooseBtn->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
    m_chooseBtn->setFocusPolicy(Qt::NoFocus);
    m_chooseBtn->setVisible(false);
    qDebug() << "Created select button";
    //占位btn，防止显影选择按钮时，ui变化
    m_pbtn = new DCommandLinkButton(" ");
    m_pbtn->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
    m_pbtn->setFocusPolicy(Qt::NoFocus);

    QHBoxLayout *NumandBtnLayout = new QHBoxLayout();
    NumandBtnLayout->setContentsMargins(0, TITLE_LINE_SPACING, 0, 0);
    NumandBtnLayout->addWidget(m_pNumCheckBox);
    NumandBtnLayout->addWidget(m_pNum);
    NumandBtnLayout->addStretch();
    NumandBtnLayout->addWidget(m_pbtn);
    NumandBtnLayout->addWidget(m_chooseBtn);

    QVBoxLayout *TitleViewLayout = new QVBoxLayout(this);
    TitleViewLayout->setContentsMargins(6, TITLE_TOP_PAD, 23, 0);
    TitleViewLayout->setSpacing(0);
    TitleViewLayout->addWidget(m_pDate);
    TitleViewLayout->addLayout(NumandBtnLayout);
    TitleViewLayout->addStretch();
    this->setLayout(TitleViewLayout);

    onThemeChanged(DGuiApplicationHelper::instance()->themeType());
    qDebug() << "TimeLineDateWidget initialization completed";

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &TimeLineDateWidget::onThemeChanged);
}

void TimeLineDateWidget::onThemeChanged(DGuiApplicationHelper::ColorType themeType)
{
    qDebug() << "Theme changed to:" << (themeType == DGuiApplicationHelper::LightType ? "Light" : "Dark");
    DPalette pal = DPaletteHelper::instance()->palette(m_pNumCheckBox);
    QColor color_BT = pal.color(DPalette::BrightText);
    if (themeType == DGuiApplicationHelper::LightType) {
        qDebug() << "Theme changed to Light";
        pal.setBrush(DPalette::Text, lightTextColor);
        m_pNumCheckBox->setForegroundRole(DPalette::Text);
        m_pNumCheckBox->setPalette(pal);
        m_pNum->setForegroundRole(DPalette::Text);
        m_pNum->setPalette(pal);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        qDebug() << "Theme changed to Dark";
        color_BT.setAlphaF(0.75);
        pal.setBrush(DPalette::Text, darkTextColor);
        m_pNumCheckBox->setForegroundRole(DPalette::Text);
        m_pNumCheckBox->setPalette(pal);
        m_pNum->setForegroundRole(DPalette::Text);
        m_pNum->setPalette(pal);
    }

    DPalette color = DPaletteHelper::instance()->palette(m_pDate);
    color.setBrush(DPalette::Text, themeType == DGuiApplicationHelper::LightType ? lightTextColor : darkTextColor);
    m_pDate->setPalette(color);
    qDebug() << "Theme changed to:" << (themeType == DGuiApplicationHelper::LightType ? "Light" : "Dark");
}

void TimeLineDateWidget::onShowCheckBox(bool bShow)
{
    qDebug() << "Setting checkbox visibility to:" << bShow;
    m_pNumCheckBox->setVisible(bShow);
    m_pNum->setVisible(!bShow);
}

void TimeLineDateWidget::onCheckBoxCliked()
{
    qDebug() << "Checkbox clicked - Entry";
    bool isChecked = m_pNumCheckBox->isChecked();
    qDebug() << "Checkbox clicked - checked:" << isChecked;
    emit sigIsSelectCurrentDatePic(isChecked, m_currentItem);
}

void TimeLineDateWidget::onChangeChooseBtnVisible(bool visible)
{
    qDebug() << "Setting choose button visibility to:" << visible;
    m_chooseBtn->setVisible(visible);
}

void TimeLineDateWidget::onTimeLinePicSelectAll(bool selectall)
{
    qDebug() << "Setting checkbox checked state to:" << selectall;
    m_pNumCheckBox->setChecked(selectall);
}

QString TimeLineDateWidget::onGetBtnStatus()
{
    qDebug() << "Getting button status - Entry";
    QString status = m_chooseBtn->text();
    qDebug() << "Getting button status:" << status;
    return status;
}

importTimeLineDateWidget::importTimeLineDateWidget(QStandardItem *item, const QString &time, const QString &num)
    : m_chooseBtn(nullptr), m_pDateandNumCheckBox(nullptr), m_currentItem(item)
{
    qDebug() << "Creating importTimeLineDateWidget - time:" << time << "num:" << num;
    this->setContentsMargins(6, 0, 0, 0);

    //时间+照片数量
    m_pDateandNumCheckBox = new DCheckBox(this);
    connect(m_pDateandNumCheckBox, &DCheckBox::clicked, this, &importTimeLineDateWidget::onCheckBoxCliked);
    DFontSizeManager::instance()->bind(m_pDateandNumCheckBox, DFontSizeManager::T6, QFont::Normal);
    QFont ft1 = DFontSizeManager::instance()->get(DFontSizeManager::T6);
    ft1.setFamily("Noto Sans CJK SC");
    m_pDateandNumCheckBox->setFont(ft1);
    QString tempTimeAndNumber = time + " " + num;
    m_pDateandNumCheckBox->setText(tempTimeAndNumber);
    int dateNumH = QFontMetrics(ft1).tightBoundingRect("Ayjg").height();
    m_pDateandNumCheckBox->setFixedHeight(dateNumH + CONTROL_TEXT_PAD);

    m_pDateandNum = new DLabel(this);
    DFontSizeManager::instance()->bind(m_pDateandNum, DFontSizeManager::T6, QFont::Normal);
    m_pDateandNum->setFont(ft1);
    m_pDateandNum->setText(tempTimeAndNumber);
    m_pDateandNum->setFixedHeight(dateNumH + CONTROL_TEXT_PAD);

    onShowCheckBox(false);

    //选择按钮
    m_chooseBtn = new DCommandLinkButton(QObject::tr("Select"));
    DFontSizeManager::instance()->bind(m_chooseBtn, DFontSizeManager::T5);
    m_chooseBtn->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
    m_chooseBtn->setFocusPolicy(Qt::NoFocus);
    m_chooseBtn->setVisible(false);
    qDebug() << "Created select button";

    //占位btn，防止显影选择按钮时，ui变化
    m_pbtn = new DCommandLinkButton(" ");
    m_pbtn->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
    m_pbtn->setFocusPolicy(Qt::NoFocus);

    //开始布局
    // 顶部边距8px为与上一时间段图片的间距，底部通过m_timelineTitleHeight控制
    QHBoxLayout *TitleViewLayout = new QHBoxLayout(this);
    TitleViewLayout->setContentsMargins(0, IMPORT_TIMELINE_TOP_PAD, 25, 0);
    TitleViewLayout->addWidget(m_pDateandNumCheckBox);
    TitleViewLayout->addWidget(m_pDateandNum);
    TitleViewLayout->addStretch();
    TitleViewLayout->addWidget(m_pbtn);
    TitleViewLayout->addWidget(m_chooseBtn);
    this->setLayout(TitleViewLayout);

    onThemeChanged(DGuiApplicationHelper::instance()->themeType());
    qDebug() << "importTimeLineDateWidget initialization completed";

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &importTimeLineDateWidget::onThemeChanged);
}

void importTimeLineDateWidget::onCheckBoxCliked()
{
    qDebug() << "Checkbox clicked - Entry";
    bool isChecked = m_pDateandNumCheckBox->isChecked();
    qDebug() << "Checkbox clicked - checked:" << isChecked;
    emit sigIsSelectCurrentDatePic(isChecked, m_currentItem);
}

void importTimeLineDateWidget::onChangeChooseBtnVisible(bool visible)
{
    qDebug() << "Setting choose button visibility to:" << visible;
    m_chooseBtn->setVisible(visible);
}

void importTimeLineDateWidget::onTimeLinePicSelectAll(bool selectall)
{
    qDebug() << "Setting checkbox checked state to:" << selectall;
    m_pDateandNumCheckBox->setChecked(selectall);
}

QString importTimeLineDateWidget::onGetBtnStatus()
{
    qDebug() << "Getting button status - Entry";
    QString status = m_chooseBtn->text();
    qDebug() << "Getting button status:" << status;
    return status;
}

void importTimeLineDateWidget::onThemeChanged(DGuiApplicationHelper::ColorType themeType)
{
    qDebug() << "Theme changed to:" << (themeType == DGuiApplicationHelper::LightType ? "Light" : "Dark");
    DPalette pal = DPaletteHelper::instance()->palette(m_pDateandNumCheckBox);
    if (themeType == DGuiApplicationHelper::LightType) {
        qDebug() << "Theme changed to Light";
        pal.setBrush(DPalette::Text, lightTextColor);
        m_pDateandNumCheckBox->setForegroundRole(DPalette::Text);
        m_pDateandNumCheckBox->setPalette(pal);
        m_pDateandNum->setForegroundRole(DPalette::Text);
        m_pDateandNum->setPalette(pal);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        qDebug() << "Theme changed to Dark";
        pal.setBrush(DPalette::Text, darkTextColor);
        m_pDateandNumCheckBox->setForegroundRole(DPalette::Text);
        m_pDateandNumCheckBox->setPalette(pal);
        m_pDateandNum->setForegroundRole(DPalette::Text);
        m_pDateandNum->setPalette(pal);
    }
    qDebug() << "Theme changed to:" << (themeType == DGuiApplicationHelper::LightType ? "Light" : "Dark");
}

void importTimeLineDateWidget::onShowCheckBox(bool bShow)
{
    qDebug() << "Setting checkbox visibility to:" << bShow;
    m_pDateandNumCheckBox->setVisible(bShow);
    m_pDateandNum->setVisible(!bShow);
}
