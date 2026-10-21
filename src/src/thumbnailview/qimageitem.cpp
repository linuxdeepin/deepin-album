/*
    SPDX-FileCopyrightText: 2011 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2015 Luca Beltrame <lbeltrame@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qimageitem.h"
#include "ddlog.h"
#include "../imageengine/imagedataservice.h"
#include <QDebug>

#include <DDciIcon>
#include <DGuiApplicationHelper>

DGUI_USE_NAMESPACE

#include <QPainter>
#include <QTimer>

QImage QImageItem::s_damage = QImage();

QImageItem::QImageItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_smooth(false)
    , m_fillMode(QImageItem::Stretch)
{
    qCDebug(albumLog) << "Initializing QImageItem";
    setFlag(ItemHasContents, true);

    QObject::connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
                     this, [this](DGuiApplicationHelper::ColorType) {
        if (isNull())
            update();
    });
}

QImageItem::~QImageItem()
{
}

void QImageItem::initDamage()
{
    updateDamageImage();

    QObject::connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
                     [](DGuiApplicationHelper::ColorType type) {
        Q_UNUSED(type)
        QImageItem::updateDamageImage();
    });
}

void QImageItem::updateDamageImage()
{
    DDciIcon::Theme theme = DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType
        ? DDciIcon::Dark : DDciIcon::Light;
    s_damage = DDciIcon::fromTheme("photo_breach").pixmap(1, 200, theme).toImage();
}

void QImageItem::setImage(const QImage &image)
{
    qCDebug(albumLog) << "QImageItem::setImage - Function entry";
    bool oldImageNull = m_image.isNull();
    m_image = image;

    QRect oldPaintedRect = m_paintedRect;
    updatePaintedRect();
    // 若图片显示方式从方图变为原始比例，需要延迟刷新图片，以便比例切换动画能正常显示
#if 1
    if (ImageDataService::instance()->getLoadMode() == 1) {
        if (m_paintedRect.width() < oldPaintedRect.width() || m_paintedRect.height() < oldPaintedRect.height()) {
            qCDebug(albumLog) << "Scheduling delayed update for image size change from" << oldPaintedRect.size() << "to" << m_paintedRect.size();
            QTimer::singleShot(100, this, [=] {
                update();
            });
        } else {
            update();
        }
    } else {
        update();
    }
#else
    // 仅第一次启动时状态加载原始比例图片
    if (ImageDataService::instance()->getLoadMode() == 1) {
        if (oldImageNull) {
            update();
        }
    } else {
        update();
    }
#endif
    Q_EMIT nativeWidthChanged();
    Q_EMIT nativeHeightChanged();
    Q_EMIT imageChanged();
    if (oldImageNull != m_image.isNull()) {
        qCDebug(albumLog) << "Image null state changed from" << oldImageNull << "to" << m_image.isNull();
        Q_EMIT nullChanged();
    }
    qCDebug(albumLog) << "QImageItem::setImage - Function exit";
}

QImage QImageItem::image() const
{
    // qCDebug(albumLog) << "QImageItem::image - Function entry, returning image";
    return m_image;
}

void QImageItem::resetImage()
{
    qCDebug(albumLog) << "Resetting image";
    setImage(QImage());
}

void QImageItem::setSmooth(const bool smooth)
{
    qCDebug(albumLog) << "QImageItem::setSmooth - Function entry, smooth:" << smooth;
    if (smooth == m_smooth) {
        return;
    }
    qCDebug(albumLog) << "Setting smooth from" << m_smooth << "to" << smooth;
    m_smooth = smooth;
    update();
    qCDebug(albumLog) << "QImageItem::setSmooth - Function exit";
}

bool QImageItem::smooth() const
{
    // qCDebug(albumLog) << "QImageItem::smooth - Function entry, returning:" << m_smooth;
    return m_smooth;
}

int QImageItem::nativeWidth() const
{
    qCDebug(albumLog) << "QImageItem::nativeWidth - Function entry";
    return m_image.size().width() / m_image.devicePixelRatio();
}

int QImageItem::nativeHeight() const
{
    qCDebug(albumLog) << "QImageItem::nativeHeight - Function entry";
    return m_image.size().height() / m_image.devicePixelRatio();
}

QImageItem::FillMode QImageItem::fillMode() const
{
    // qCDebug(albumLog) << "QImageItem::fillMode - Function entry, returning:" << static_cast<int>(m_fillMode);
    return m_fillMode;
}

void QImageItem::setFillMode(QImageItem::FillMode mode)
{
    qCDebug(albumLog) << "QImageItem::setFillMode - Function entry, mode:" << mode;
    if (mode == m_fillMode) {
        return;
    }

    qCDebug(albumLog) << "Setting fill mode from" << m_fillMode << "to" << mode;
    m_fillMode = mode;
    updatePaintedRect();
    update();
    Q_EMIT fillModeChanged();
}

void QImageItem::paint(QPainter *painter)
{
    QImage *pImage = nullptr;

    if (m_image.isNull()) {
        pImage = &s_damage;
    } else {
        pImage = &m_image;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, m_smooth);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, m_smooth);

    if (m_fillMode == TileVertically) {
        painter->scale(width() / (qreal)pImage->width(), 1);
    }

    if (m_fillMode == TileHorizontally) {
        painter->scale(1, height() / (qreal)pImage->height());
    }

    if (m_fillMode == Pad) {
        QRect centeredRect = m_paintedRect;
        centeredRect.moveCenter(pImage->rect().center());
        painter->drawImage(m_paintedRect, *pImage, centeredRect);
    } else if (m_fillMode >= Tile) {
        painter->drawTiledPixmap(m_paintedRect, QPixmap::fromImage(*pImage));
    } else {
        painter->drawImage(m_paintedRect, *pImage, pImage->rect());
    }

    painter->restore();
}

bool QImageItem::isNull() const
{
    // qCDebug(albumLog) << "QImageItem::isNull - Function entry, returning:" << m_image.isNull();
    return m_image.isNull();
}

int QImageItem::paintedWidth() const
{
    // qCDebug(albumLog) << "QImageItem::paintedWidth - Function entry";
    return m_paintedRect.width();
}

int QImageItem::paintedHeight() const
{
    // qCDebug(albumLog) << "QImageItem::paintedHeight - Function entry";
    return m_paintedRect.height();
}

void QImageItem::updatePaintedRect()
{
    // qCDebug(albumLog) << "QImageItem::updatePaintedRect - Function entry";
    QImage *pImage = nullptr;

    if (m_image.isNull()) {
        pImage = &s_damage;
    } else {
        pImage = &m_image;
    }

    QRectF sourceRect = m_paintedRect;
    QRectF destRect;

    switch (m_fillMode) {
    case PreserveAspectFit: {
        // qCDebug(albumLog) << "PreserveAspectFit - Function entry";
        QSizeF scaled = pImage->size();
        QSizeF size = boundingRect().size();
        scaled.scale(boundingRect().size(), Qt::KeepAspectRatio);
        destRect = QRectF(QPoint(0, 0), scaled);
        destRect.moveCenter(boundingRect().center().toPoint());
        qCDebug(albumLog) << "PreserveAspectFit: scaled size" << scaled << "from" << pImage->size();
        break;
    }
    case PreserveAspectCrop: {
        // qCDebug(albumLog) << "PreserveAspectCrop - Function entry";
        QSizeF scaled = pImage->size();
        scaled.scale(boundingRect().size(), Qt::KeepAspectRatioByExpanding);
        destRect = QRectF(QPoint(0, 0), scaled);
        destRect.moveCenter(boundingRect().center().toPoint());
        qCDebug(albumLog) << "PreserveAspectCrop: scaled size" << scaled << "from" << pImage->size();
        break;
    }
    case TileVertically: {
        // qCDebug(albumLog) << "TileVertically - Function entry";
        destRect = boundingRect().toRect();
        destRect.setWidth(destRect.width() / (width() / (qreal)pImage->width()));
        qCDebug(albumLog) << "TileVertically: dest rect" << destRect;
        break;
    }
    case TileHorizontally: {
        // qCDebug(albumLog) << "TileHorizontally - Function entry";
        destRect = boundingRect().toRect();
        destRect.setHeight(destRect.height() / (height() / (qreal)pImage->height()));
        qCDebug(albumLog) << "TileHorizontally: dest rect" << destRect;
        break;
    }
    case Stretch:
    case Tile:
    case Pad:
    default:
        // qCDebug(albumLog) << "Default fill mode - Function entry";
        destRect = boundingRect().toRect();
        qCDebug(albumLog) << "Default fill mode: dest rect" << destRect;
    }

    if (destRect != sourceRect) {
        m_paintedRect = destRect.toRect();
        Q_EMIT paintedHeightChanged();
        Q_EMIT paintedWidthChanged();
    }
    // qCDebug(albumLog) << "QImageItem::updatePaintedRect - Function exit";
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void QImageItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
#else
void QImageItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
#endif
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QQuickPaintedItem::geometryChanged(newGeometry, oldGeometry);
#else
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
#endif
    qCDebug(albumLog) << "Geometry changed from" << oldGeometry << "to" << newGeometry;
    updatePaintedRect();
}
