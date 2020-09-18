/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
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
    explicit ImgInfoDialog(const QString &path, QWidget *parent = nullptr);
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
signals:
    void closed();

};

#endif // IMGINFODIALOG_H
