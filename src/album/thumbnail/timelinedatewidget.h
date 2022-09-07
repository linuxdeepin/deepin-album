// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TIMELINEDATEWIDGET_H
#define TIMELINEDATEWIDGET_H

#include <DCommandLinkButton>
#include <DLabel>
#include <DWidget>
#include <DGuiApplicationHelper>

#include <QStandardItem>

DWIDGET_USE_NAMESPACE

/****时间线界面，图片数量、时间、选择按钮统一类****/

class TimeLineDateWidget : public DWidget
{
    Q_OBJECT
public:
    explicit TimeLineDateWidget(QStandardItem *item, const QString &time, const QString &num);

public slots:
    void onChooseBtnCliked();
    void onChangeChooseBtnVisible(bool visible = false);             //1050预留，默认隐藏按钮
    void onTimeLinePicSelectAll(bool selectall = true);              //响应本时间内所有照片的全选状态，改变btn
    QString onGetBtnStatus();                                        //获取按钮状态
    void onThemeChanged(DGuiApplicationHelper::ColorType themeType); //界面主题切换

signals:
    void sigIsSelectCurrentDatePic(bool isSelectAll, QStandardItem *item);

private:
    DCommandLinkButton *m_chooseBtn;
    DLabel *m_pDate;
    DLabel *m_pNum;
    QStandardItem *m_currentItem;
    DCommandLinkButton *m_pbtn;//占位btn
};



/****已导入时间线界面，图片数量、时间、选择按钮统一类****/

class importTimeLineDateWidget : public DWidget
{
    Q_OBJECT
public:
    explicit importTimeLineDateWidget(QStandardItem *item, const QString &time, const QString &num);

public slots:
    void onChooseBtnCliked();
    void onChangeChooseBtnVisible(bool visible = false);             //1050预留，默认隐藏按钮
    void onTimeLinePicSelectAll(bool selectall = true);              //响应本时间内所有照片的全选状态，改变btn
    QString onGetBtnStatus();                                        //获取按钮状态
    void onThemeChanged(DGuiApplicationHelper::ColorType themeType); //界面主题切换

signals:
    void sigIsSelectCurrentDatePic(bool isSelectAll, QStandardItem *item);

private:
    DCommandLinkButton *m_chooseBtn;
    DLabel *m_pDateandNum;
    QStandardItem *m_currentItem;
    DCommandLinkButton *m_pbtn;//占位btn

};

#endif // TIMELINEDATEWIDGET_H
