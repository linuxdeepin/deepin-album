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
#include <DMainWindow>
#include "controller/viewerthememanager.h"
#include "widgets/themewidget.h"

#include <QWidget>
#include <QLabel>
#include <QScrollArea>
#include <QVector>
#include <DScrollArea>
#include <DArrowLineExpand>
#include <denhancedwidget.h>
DWIDGET_USE_NAMESPACE
class QFormLayout;
class QVBoxLayout;
class ImgInfoDialog : public DDialog
{
    Q_OBJECT
public:
    explicit ImgInfoDialog(const QString &path, QWidget *parent = 0);
    int height();
private:
    void initUI();
    void setImagePath(const QString &path);
    void updateInfo();
    void clearLayout(QLayout* layout);
    const QString trLabel(const char *str);
    void updateBaseInfo(const QMap<QString, QString> &infos);
    void updateDetailsInfo(const QMap<QString, QString> &infos);
    QList<DBaseExpand *> addExpandWidget(const QStringList &titleList);
    void initExpand(QVBoxLayout *layout, DBaseExpand *expand);
    int contentHeight() const;

private:
    int m_maxFieldWidth;
    bool m_isBaseInfo = false;
    bool m_isDetailsInfo = false;
    QString m_path;
    QFrame* m_exif_base = nullptr;
    QFrame* m_exif_details = nullptr;
    QFormLayout* m_exifLayout_base = nullptr;
    QFormLayout* m_exifLayout_details = nullptr;
    QList<DBaseExpand *> m_expandGroup;
    QVBoxLayout *m_mainLayout = nullptr;
    QScrollArea *m_scrollArea = nullptr;
};

#endif // IMGINFODIALOG_H
