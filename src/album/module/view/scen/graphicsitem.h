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
#ifndef GRAPHICSMOVIEITEM_H
#define GRAPHICSMOVIEITEM_H

#include "utils/unionimage.h"

#include <QGraphicsPixmapItem>
#include <QPointer>

class QMovie;
class GraphicsMovieItem : public QGraphicsPixmapItem, QObject
{
public:
    explicit GraphicsMovieItem(const QString &fileName, const QString &fileSuffix, QGraphicsItem *parent = nullptr);
    ~GraphicsMovieItem();
    bool isValid() const;
    void start();
    void stop();
private:
//    UnionImage_NameSpace::UnionMovieImage m_movie;
    QPointer<QMovie> m_qmovie;
//    QPointer<QTimer> m_pTImer;
//    QString m_suffix;
//    int m_index;

};

class GraphicsPixmapItem : public QGraphicsPixmapItem
{
public:
    explicit GraphicsPixmapItem(const QPixmap &pixmap);
    ~GraphicsPixmapItem() override;

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    QPair<qreal, QPixmap> cachePixmap;
};

#endif // GRAPHICSMOVIEITEM_H
