// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PRINTHELPER_H
#define PRINTHELPER_H

#include <QObject>

class RequestedSlot;
class PrintHelper : public QObject
{
    Q_OBJECT

public:
    static PrintHelper *getIntance();
    explicit PrintHelper(QObject *parent = nullptr);
    ~PrintHelper();

    void showPrintDialog(const QStringList &paths, QWidget *parent = nullptr);
    void deconstruction();

    RequestedSlot *m_re = nullptr;

private:
    static PrintHelper *m_Printer;
};

#endif // PRINTHELPER_H
