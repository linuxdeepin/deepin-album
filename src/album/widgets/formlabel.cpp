// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "formlabel.h"

SimpleFormLabel::SimpleFormLabel(const QString &t, QWidget *parent)
    : QLabel(t, parent)
{
    QFont font;
    font.setPixelSize(12);
    setFont(font);
}

SimpleFormField::SimpleFormField(QWidget *parent)
    : QLabel(parent)
{
    QFont font;
    font.setPixelSize(12);
    setFont(font);
    setWordWrap(true);
}

void SimpleFormField::resizeEvent(QResizeEvent *event)
{
    if (wordWrap() && sizePolicy().verticalPolicy() == QSizePolicy::Minimum) {
        // heightForWidth rely on minimumSize to evaulate, so reset it before
        setMinimumHeight(0);
        // define minimum height
        setMinimumHeight(heightForWidth(width()));
    }
    QLabel::resizeEvent(event);
}
