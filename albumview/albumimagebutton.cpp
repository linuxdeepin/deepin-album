#include "albumimagebutton.h"

#include <QDebug>
#include <QPainter>

#include <DHiDPIHelper>
#include <DPalette>

DGUI_USE_NAMESPACE

AlbumImageButton::AlbumImageButton(QWidget *parent)
    : DPushButton(parent), status(0), autoChecked(false)
    , transparent(true)
{
}

AlbumImageButton::AlbumImageButton(const QString &normalPic, const QString &hoverPic,
                                   const QString &pressPic, const QString &checkedPic, QWidget *parent)
    : DPushButton (parent)
{
    defaultPicPath.normalPicPath = normalPic;
    defaultPicPath.hoverPicPath = hoverPic;
    defaultPicPath.pressPicPath = pressPic;
    defaultPicPath.checkedPicPath = checkedPic;

    auto pl = this->palette();
    QColor sbcolor("#000000");
    sbcolor.setAlphaF(0.05);
    pl.setColor(DPalette::Shadow, sbcolor);
    setPalette(pl);
}

void AlbumImageButton::setPropertyPic(QString propertyName, const QVariant &value,
                                      const QString &normalPic, const QString &hoverPic, const QString &pressPic, const QString &checkedPic)
{
    MusicPicPathInfo curPicPath;
    curPicPath.normalPicPath = normalPic;
    curPicPath.hoverPicPath = hoverPic;
    curPicPath.pressPicPath = pressPic;
    curPicPath.checkedPicPath = checkedPic;

    if (propertyPicPaths.first == propertyName) {
        propertyPicPaths.second[value] = curPicPath;
    } else {
        QMap<QVariant, MusicPicPathInfo> curPicPathInfo;
        curPicPathInfo.insert(value, curPicPath);
        propertyPicPaths.first = propertyName;
        propertyPicPaths.second = curPicPathInfo;
    }
}

void AlbumImageButton::setPropertyPic(const QString &normalPic, const QString &hoverPic, const QString &pressPic, const QString &checkedPic)
{
    defaultPicPath.normalPicPath = normalPic;
    defaultPicPath.hoverPicPath = hoverPic;
    defaultPicPath.pressPicPath = pressPic;
    defaultPicPath.checkedPicPath = checkedPic;
}

void AlbumImageButton::setTransparent(bool flag)
{
    transparent = flag;
}

void AlbumImageButton::setAutoChecked(bool flag)
{
    autoChecked = flag;
}

void AlbumImageButton::paintEvent(QPaintEvent *event)
{
    if (!transparent) {
        DPushButton::paintEvent(event);
    }

    QString curPicPath = defaultPicPath.normalPicPath;
    if (propertyPicPaths.first.isEmpty() || !propertyPicPaths.second.contains(property(propertyPicPaths.first.toStdString().data()))) {
        QString curPropertyPicPathStr;
        if (isChecked() && !defaultPicPath.checkedPicPath.isEmpty()) {
            curPropertyPicPathStr = defaultPicPath.checkedPicPath;
        } else {
            if (status == 1 && !defaultPicPath.hoverPicPath.isEmpty()) {
                curPropertyPicPathStr = defaultPicPath.hoverPicPath;
            } else if (status == 2 && !defaultPicPath.pressPicPath.isEmpty()) {
                curPropertyPicPathStr = defaultPicPath.pressPicPath;
            }
        }

        if (!curPropertyPicPathStr.isEmpty()) {
            curPicPath = curPropertyPicPathStr;
        }
    } else {
        QVariant value = property(propertyPicPaths.first.toStdString().data());
        MusicPicPathInfo curPropertyPicPath = propertyPicPaths.second[value];
        QString curPropertyPicPathStr;
        if (isChecked() && !defaultPicPath.checkedPicPath.isEmpty()) {
            curPropertyPicPathStr = curPropertyPicPath.checkedPicPath;
        } else {
            if (status == 1 && !defaultPicPath.hoverPicPath.isEmpty()) {
                curPropertyPicPathStr = curPropertyPicPath.hoverPicPath;
            } else if (status == 2 && !defaultPicPath.pressPicPath.isEmpty()) {
                curPropertyPicPathStr = curPropertyPicPath.pressPicPath;
            } else {
                curPropertyPicPathStr = curPropertyPicPath.normalPicPath;
            }
        }
        if (!curPropertyPicPathStr.isEmpty()) {
            curPicPath = curPropertyPicPathStr;
        }
    }
    QPixmap pixmap = DHiDPIHelper::loadNxPixmap(curPicPath);
    if (pixmap.isNull()) {
        pixmap = DHiDPIHelper::loadNxPixmap(defaultPicPath.normalPicPath);
    }


    pixmap.setDevicePixelRatio(devicePixelRatioF());
    QPainter painter(this);
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::HighQualityAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    int pixmapWidth = pixmap.rect().width();
    int pixmapHeight = pixmap.rect().height();
    QRect pixmapRect((rect().width() - pixmapWidth) / 2, (rect().height() - pixmapHeight) / 2, pixmapWidth, pixmapHeight);
    pixmapRect = pixmapRect.intersected(rect());
    painter.drawPixmap(pixmapRect, pixmap, QRect(0, 0, pixmapWidth, pixmapHeight));

    painter.restore();
}

void AlbumImageButton::enterEvent(QEvent *event)
{
    status = 1;
    DPushButton::enterEvent(event);
    if (autoChecked) {
        setChecked(true);
    }
}

void AlbumImageButton::leaveEvent(QEvent *event)
{
    status = 0;
    DPushButton::leaveEvent(event);
    if (autoChecked) {
        setChecked(false);
    }
}

void AlbumImageButton::mousePressEvent(QMouseEvent *event)
{
    status = 2;
    DPushButton::mousePressEvent(event);
}

void AlbumImageButton::mouseReleaseEvent(QMouseEvent *event)
{
    status = 0;
    DPushButton::mouseReleaseEvent(event);
}
