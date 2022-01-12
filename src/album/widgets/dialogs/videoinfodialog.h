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
#ifndef VIDEOINFODIALOG_H
#define VIDEOINFODIALOG_H

#include <DDialog>
#include <DScrollArea>
#include <DDrawer>
#include <denhancedwidget.h>

#include "movieservice.h"

DWIDGET_USE_NAMESPACE

class QFormLayout;
class QVBoxLayout;
class QWidget;

class VideoInfoDialog : public DDialog
{
    Q_OBJECT
public:
    explicit VideoInfoDialog(const QString &path, const QString &displayName, bool isTrash, QWidget *parent = nullptr);
private:
    void initUI();
    void setVideoInfo(const QString &path);

    //基本信息
    void        updateBasicInfo();
    //编码信息
    void        updateCodecInfo();
    //音频流信息
    void        updateAudioInfo();
//    void        clearLayout(QLayout *layout);

//    QList<DDrawer *> addExpandWidget(const QStringList &titleList);
    void        appendExpandWidget(const QString &title);
    void        initExpand(QVBoxLayout *layout, DDrawer *expand);
    int         contentHeight() const;
    QString     SpliteText(const QString &text, const QFont &font, int nLabelSize);
protected:
    bool event(QEvent *event)override;
    void showEvent(QShowEvent *e) override;
private:
    MovieInfo m_movieInfo;

    QList<DDrawer *> m_expandGroup;
    QVBoxLayout *m_scrollAreaLayout = nullptr;
    int m_title_maxwidth;
    int m_maxFieldWidth;
    int m_currentFontSize;

    QFrame *m_basicInfoFrame = nullptr;            //基本信息
    QFormLayout *m_basicInfoFrameLayout = nullptr;
    bool m_containBaseInfo = false;                //含基本信息

    QFrame *m_codecInfoFrame = nullptr;            //编码信息
    QFormLayout *m_codecInfoFrameLayout = nullptr;
    bool m_containCodecInfo = false;               //含编码信息

    QFrame *m_audioInfoFrame = nullptr;            //音频流信息
    QFormLayout *m_audioInfoFrameLayout = nullptr;
    bool m_containAudioInfo = false;               //含音频流信息

    QVBoxLayout *m_mainLayout = nullptr;
    DScrollArea *m_scrollArea = nullptr;

    QString m_displayName; //显示出来的名字
    bool m_isTrash; //是最近删除路径
};

#endif // VIDEOINFODIALOG_H
