// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
    int getCreateUID() const;
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
    static const QString getNewAlbumName(const QString &baseName);
    DLineEdit *getEdit();

signals:
    void albumAdded(int UID);
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
    int m_createUID = 0;
    DLineEdit *edit;
    bool m_OKClicked;

    //键盘交互
    QList<QWidget *> m_allTabOrder;

};

#endif // ALBUMCREATEDIALOG_H
