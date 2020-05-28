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
#ifndef ALBUMCREATEDIALOG_H
#define ALBUMCREATEDIALOG_H

#include "ddialog.h"
#include "dlineedit.h"
#include "controller/signalmanager.h"

class AlbumCreateDialog : public DDialog
{
    Q_OBJECT
public:
    explicit AlbumCreateDialog(DWidget *parent = nullptr);

    const QString getCreateAlbumName() const;

signals:
    void albumAdded();
    void sigClose();

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    void initUI();
    void initConnection();
    void createAlbum(const QString &newName);
    const QString getNewAlbumName(const QString &baseName) const;

private:
    QString m_createAlbumName;
    DLineEdit *edit;
    DPushButton *m_Cancel;
    DPushButton *m_OK;
    bool m_OKClicked;

};

#endif // ALBUMCREATEDIALOG_H
