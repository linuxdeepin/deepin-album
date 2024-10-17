#ifndef QMLWEBVIEWWIDGET_H
#define QMLWEBVIEWWIDGET_H

#include <QQuickPaintedItem>
#include <QWidget>

//#define USE_INNER
class TimeLineView;
class QmlCustomInternalWidget : public QWidget
{
    friend class QmlWidget;
    Q_OBJECT
private:
    explicit QmlCustomInternalWidget(QQuickPaintedItem *parent);

public slots:
protected:
    QPaintEngine * paintEngine() const ;
    void paintEvent(QPaintEvent * event) ;

private:
    QQuickPaintedItem *m_qquickContainer;
    TimeLineView *m_timeView { nullptr };
};


class QmlWidget : public QQuickPaintedItem
{
    Q_OBJECT

public:
    explicit QmlWidget(QQuickItem *parent = nullptr);
    void paint(QPainter *painter);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void navigateToMonth(const QString& month);
    Q_INVOKABLE QVariantList allUrls();
    Q_INVOKABLE void unSelectAll();

public slots:
    void setFocus(bool arg);

private:
#ifdef USE_INNER
    QmlCustomInternalWidget *m_widget;
#else
    TimeLineView *m_widget;
#endif

    QWidget *m_lastHoveredWidget = nullptr;
protected:
    virtual void geometryChanged(const QRectF & newGeometry,
                                 const QRectF & oldGeometry);
    bool event(QEvent * e) ;
protected slots:
    void updateGeometry();

private:
    bool m_disableHoverEvent {false}; // 日视图和已导入视图在刷新时，鼠标悬停事件路由不能执行，否则会访问之前已析构的QWidget控件，导致程序崩溃
};

#endif // QMLWEBVIEWWIDGET_H
