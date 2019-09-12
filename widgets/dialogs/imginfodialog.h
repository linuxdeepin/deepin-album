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
#include <DExpandGroup>
#include <DArrowLineExpand>
#include <QFormLayout>

DWIDGET_USE_NAMESPACE

class QVBoxLayout;
class ImgInfoDialog : public DDialog
{
    Q_OBJECT
public:
    explicit ImgInfoDialog(const QString &path, QWidget *parent = 0);

signals:
    void closed();

protected:
    void hideEvent(QHideEvent *e) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;

private:
    void initThumbnail(const QString &path);
    void initSeparator();
    void initInfos(const QString &path);
    void initCloseButton();
    void initUI();
    DExpandGroup *expandGroup() const;
    DExpandGroup *addExpandWidget(const QStringList &titleList);
    void onExpandChanged(const bool &e);
    void loadPluginExpandWidgets();
    QFrame *createInfoFrame(const QList<QPair<QString, QString> >& properties);
    QFrame *createBaseInfoFrame();
    QFrame *createDetailsInfoFrame();
    void updateBaseInfo(const QMap<QString, QString> &infos);
    void updateDetailsInfo(const QMap<QString, QString> &infos);
    void clearLayout(QLayout *layout);
    void updateInfo();
    const QString trLabel(const char *str);

private:
//    DUrl m_url{};
    QString m_path;
    int m_maxTitleWidth;  //For align colon
    int m_maxFieldWidth;
    int baseInfoHeidht=0;
    int detailsInfoHeidht=0;
    DExpandGroup* m_expandGroup{ nullptr };
    QVBoxLayout *m_layout{ nullptr };
    QVBoxLayout *m_mainLayout{ nullptr };
    QFormLayout *m_exifLayout_base{ nullptr };
    QFormLayout *m_exifLayout_details{ nullptr };

};

#endif // IMGINFODIALOG_H
