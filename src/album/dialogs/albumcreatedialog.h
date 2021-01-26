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
    /**
     * @brief AlbumCreateDialog::getNewAlbumName
     * @param[in] baseName
     * @param[in] isWithOutSelf
     * @param[in] beforeName
     * @author DJH
     * @date 2020/6/1
     * @return const QString
     * 根据已有相册名，获取对于数据库中不重复的新相册名，当isWithOutSefl为true的时候，查询不会包含自己，用于替换型查询
     */
    static const QString getNewAlbumName(const QString &baseName, bool isWithOutSelf = false, const QString &beforeName = "");
    DLineEdit *getEdit();

signals:
    void albumAdded();
    void sigClose();

protected:
    void keyPressEvent(QKeyEvent *e) override;
    bool eventFilter(QObject *obj, QEvent *event) override;


private:
    void initUI();
    void initConnection();
    void createAlbum(const QString &newName);

private slots:
    void onTextEdited(const QString &);
    void onVisibleChanged(bool v);
    void onReturnPressed();
    void onButtonClicked(int index);
    void onClosed();

private:
    QString m_createAlbumName;
    DLineEdit *edit;
    bool m_OKClicked;

};

#endif // ALBUMCREATEDIALOG_H
