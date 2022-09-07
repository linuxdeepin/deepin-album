// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IMGINFODIALOG_H
#define IMGINFODIALOG_H

#include <DDialog>
#include <DScrollArea>
//#include <DArrowLineExpand>
#include <DDrawer>
#include <denhancedwidget.h>

DWIDGET_USE_NAMESPACE

class QFormLayout;
class QVBoxLayout;
class QWidget;

class ImgInfoDialog : public DDialog
{
    Q_OBJECT
public:
    explicit ImgInfoDialog(const QString &path, const QString &displayName, QWidget *parent = nullptr);
    int height();
private:
    void initUI();
    void setImagePath(const QString &path);
    void updateInfo();
    void clearLayout(QLayout *layout);
    const QString trLabel(const char *str);
    void updateBaseInfo(const QMap<QString, QString> &infos);
    void updateDetailsInfo(const QMap<QString, QString> &infos);
    QList<DDrawer *> addExpandWidget(const QStringList &titleList);
    void initExpand(QVBoxLayout *layout, DDrawer *expand);
    int contentHeight() const;

private:
    int m_title_maxwidth;
    int m_maxFieldWidth;
    int m_currentFontSize;
    bool m_isBaseInfo;
    bool m_isDetailsInfo;
    QString m_path;
    QString m_displayName; //最近删除里面的文件名可能和它自己的文件名不一样
    QFrame *m_exif_base;
    QFrame *m_exif_details;
    QFormLayout *m_exifLayout_base;
    QFormLayout *m_exifLayout_details;
    QList<DDrawer *> m_expandGroup;
    QVBoxLayout *m_mainLayout;
    DScrollArea *m_scrollArea;
    bool m_mousePress = false;
    int m_mouseY = 0;
    void keyPressEvent(QKeyEvent *e) override;
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event)override;
    bool eventFilter(QObject *obj, QEvent *e) Q_DECL_OVERRIDE;
};

#endif // IMGINFODIALOG_H
