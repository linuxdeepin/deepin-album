#include "qmlWidget.h"
#include "widgets/timelineview/timelineview.h"
//#include "../config.h"
#include <QTimer>
#include <QQuickWindow>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDebug>
QmlCustomInternalWidget::QmlCustomInternalWidget(QQuickPaintedItem *parent) :
    QWidget(nullptr)
{
    m_qquickContainer = parent;
    setFocusPolicy(Qt::WheelFocus);
    setAttribute(Qt::WA_WState_Created, true);// don't create window on setVisible(true)
    setVisible(true);// if not visible some operations will not done.

#ifdef USE_INNER
    QHBoxLayout *horizontalLayout = new QHBoxLayout(this);
    m_timeView = new TimeLineView();
    horizontalLayout->addWidget(m_timeView);
#endif
}

QPaintEngine *QmlCustomInternalWidget::paintEngine() const
{
    return nullptr;
}

void QmlCustomInternalWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    m_qquickContainer->update(event->rect());
}

QmlWidget::QmlWidget(QQuickItem *parent) :
    QQuickPaintedItem(parent)
{
    setOpaquePainting(true);
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);

    setFlag(QQuickItem::ItemAcceptsDrops, true);
    setFlag(QQuickItem::ItemHasContents, true);
#ifdef USE_INNER
    m_widget = new QmlCustomInternalWidget(this);
#else
    m_widget = new TimeLineView(this);
    m_widget->setFocusPolicy(Qt::WheelFocus);
    m_widget->setAttribute(Qt::WA_WState_Created, true);
    m_widget->setAutoFillBackground(true);   // 添加这行
    m_widget->setVisible(true);
#endif
    connect(this, &QQuickItem::widthChanged, this, &QmlWidget::updateGeometry);
    connect(this, &QQuickItem::heightChanged, this, &QmlWidget::updateGeometry);
    connect(this, &QQuickItem::visibleChanged, this, &QmlWidget::updateGeometry);
}

void QmlWidget::paint(QPainter *painter)
{
    m_widget->render(painter, QPoint(), QRegion(),
                     QWidget::DrawWindowBackground | QWidget::DrawChildren);
}

void QmlWidget::refresh()
{
    if (m_widget) {
        m_disableHoverEvent = true;
        m_lastHoveredWidget = nullptr;
        m_widget->clearAndStartLayout();
        m_disableHoverEvent = false;
    }
}

void QmlWidget::navigateToMonth(const QString &month)
{
    if (m_widget) {
        m_widget->getThumbnailListView()->navigateToMonth(month);
    }
}

QVariantList QmlWidget::allUrls()
{
    if (m_widget) {
        return m_widget->getThumbnailListView()->allUrls();
    }

    return QVariantList();
}

void QmlWidget::unSelectAll()
{
    if (m_widget)
        m_widget->clearAllSelection();
}

void QmlWidget::setFocus(bool arg)
{
    QQuickPaintedItem::setFocus(arg);
    if(arg){
        m_widget->setFocus();
        forceActiveFocus();
    }
    else{
        m_widget->clearFocus();
    }
    setActiveFocusOnTab(arg);
}

void QmlWidget::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    qDebug() << "QmlCustomWidget::geometryChanged, " << newGeometry;
    if(newGeometry == oldGeometry)
        return;
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    updateGeometry();
}

#if 0
bool QmlWidget::event(QEvent *e)
{
//    qDebug() << "QmlCustomWidget::event" << e->type();

    if(e->type() == QEvent::HoverEnter ||
        e->type() == QEvent::HoverMove ||
        e->type() == QEvent::HoverLeave ){

        handleHoverMoveEvent((QHoverEvent*)e);
        return true;
    }

    if( e->type() == QEvent::MouseButtonPress ||
        e->type() == QEvent::MouseButtonDblClick )
        setFocus(true);

    if(m_widget->event(e))
        return true;
    return QQuickPaintedItem::event(e);
}
#else
bool QmlWidget::event(QEvent *e)
{
    static QPoint lastPos;
    static bool isPressed = false;
    static QWidget* pressedWidget = nullptr;
    QObject* pScrollArea = m_widget->findChild<QObject*>("qt_scrollarea_viewport");
    // 处理鼠标事件
    if (auto mouseEvent = dynamic_cast<QMouseEvent *>(e)) {
        QPoint pos = mouseEvent->pos();

        switch (mouseEvent->type()) {
            case QEvent::MouseButtonPress:
                isPressed = true;
                lastPos = pos;
                pressedWidget = m_widget->childAt(pos);
                break;
            case QEvent::MouseMove:
                if (!isPressed) {
                    pressedWidget = m_widget->childAt(pos);
                }
                break;
            case QEvent::MouseButtonRelease:
                isPressed = false;
                pressedWidget = nullptr;

                if ((lastPos - pos).manhattanLength() >= 2) {
                    qDebug() << "pos.x" << pos.x() << "width: " << m_widget->width();
                    if (pos.x() < 0)
                        pos.setX(2);
                    if (pos.x() > m_widget->width())
                        pos.setX(m_widget->width() - 10);
                    qDebug() << "pos.y" << pos.y();
                    if (pos.y() < 87)
                        pos.setY(87);
                    if (pos.y() > m_widget->height())
                        pos.setY(m_widget->height() - 10);
                }
                break;
            default:
                break;
        }

        QWidget* targetWidget = isPressed ? pressedWidget : m_widget->childAt(pos);
        if (targetWidget) {
            QPoint localPos = targetWidget->mapFrom(m_widget, pos);

            QMouseEvent mappedEvent(mouseEvent->type(), localPos, mouseEvent->button(), mouseEvent->buttons(), mouseEvent->modifiers());

            bool handled = QCoreApplication::sendEvent(targetWidget, &mappedEvent);

            if (handled) {
                update();
            }

            // 右键点击事件
            if (mouseEvent->button() == Qt::RightButton && mouseEvent->type() == QEvent::MouseButtonPress) {
                QContextMenuEvent contextMenuEvent(QContextMenuEvent::Mouse, localPos, mouseEvent->globalPos());
                QCoreApplication::sendEvent(targetWidget, &contextMenuEvent);
            }

            // 对于鼠标移动事件，即使没有被处理也返回true，以确保持续接收移动事件
            if (mouseEvent->type() == QEvent::MouseMove) {
                return true;
            }

            return handled;
        }
    } else if (auto keyEvent = dynamic_cast<QKeyEvent*>(e)) {
        if(pScrollArea)
            QCoreApplication::sendEvent(pScrollArea, keyEvent);
    } else if (auto wheelEvent = dynamic_cast<QWheelEvent *>(e)) {
        // 处理鼠标滚轮事件
        // Qt6下，滚轮事件只有转发到scrollArea上，缩略图列表才能在正常接收滚轮事件
        // 转发到其子控件，比如Label或TimeLineDateWidget上，事件都不会路由到缩略图列表
        if(pScrollArea)
            QCoreApplication::sendEvent(pScrollArea, wheelEvent);
    } else if(e->type() == QEvent::HoverMove) {
        //qWarning() << "e->type()" << e->type();
        // 处理鼠标悬停事件
        QHoverEvent *hoverEvent = dynamic_cast<QHoverEvent *>(e);
        if (hoverEvent && !m_disableHoverEvent) {
            QWidget* ss = m_widget->childAt(hoverEvent->pos());
            if (ss) {
                if (ss != m_lastHoveredWidget) {
                    if (m_lastHoveredWidget) {
                        QEvent leaveEvent(QEvent::Leave);
                        QCoreApplication::sendEvent(m_lastHoveredWidget, &leaveEvent);
                    }

                    QEvent enterEvent(QEvent::Enter);
                    QCoreApplication::sendEvent(ss, &enterEvent);
                    m_lastHoveredWidget = ss;
                }

                QCoreApplication::sendEvent(ss, e);
            }
        }
    } else if (e->type() == QEvent::HoverEnter) {
        setFocus(true);
    }

    return QQuickPaintedItem::event(e);

    // if (m_widget->event(e))
    //     return true;
}
#endif

void QmlWidget::updateGeometry()
{
    if (!isVisible())
        return;

    QPointF newPos(0, 0);
    newPos = mapToItem(0, newPos);
    qDebug() << "QmlCustomWidget::updateGeometry, top left mapped to window: " << newPos;

    if(this->window()){
        newPos += this->window()->position();
        qDebug() << "QmlCustomWidget::updateGeometry, top left mapped to screen: " << newPos;
    }
    QRectF absRect(newPos, contentsBoundingRect().size());
    m_widget->setGeometry(absRect.toRect());
}
