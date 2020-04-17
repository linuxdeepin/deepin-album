#pragma once

#include <QScopedPointer>
#include <QMap>
#include <QVariant>

#include <DPushButton>

DWIDGET_USE_NAMESPACE

class AlbumImageButton : public DPushButton
{
    struct MusicPicPathInfo {
        QString normalPicPath;
        QString hoverPicPath;
        QString pressPicPath;
        QString checkedPicPath;
    };
    Q_OBJECT
public:
    explicit AlbumImageButton(QWidget *parent = Q_NULLPTR);
    AlbumImageButton(const QString &normalPic, const QString &hoverPic,
                     const QString &pressPic, const QString &checkedPic = QString(), QWidget *parent = nullptr);
    void setPropertyPic(QString propertyName, const QVariant &value, const QString &normalPic, const QString &hoverPic,
                        const QString &pressPic, const QString &checkedPic = QString());
    void setPropertyPic(const QString &normalPic, const QString &hoverPic,
                        const QString &pressPic, const QString &checkedPic = QString());
    void setTransparent(bool flag);
    void setAutoChecked(bool flag);
protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void enterEvent(QEvent *event) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
private:
    char                                               status                  = 0;
    bool                                               autoChecked             = false;
    MusicPicPathInfo                                   defaultPicPath;
    bool                                               transparent             = true;
    QPair<QString, QMap<QVariant, MusicPicPathInfo> >  propertyPicPaths;
};
