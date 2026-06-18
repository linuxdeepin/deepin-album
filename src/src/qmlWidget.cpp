#include "qmlWidget.h"
#include "widgets/timelineview/timelineview.h"
#include "widgets/importtimelineview/importtimelineview.h"
//#include "../config.h"

#include <DApplication>
#include <QTimer>
#include <QQuickWindow>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollBar>
#include <QAbstractScrollArea>
#include <QDebug>
QmlCustomInternalWidget::QmlCustomInternalWidget(QQuickPaintedItem *parent) :
    QWidget(nullptr)
{
    qDebug() << "Initializing QmlCustomInternalWidget";
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
    qDebug() << "Initializing QmlWidget";
    setOpaquePainting(true);
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);

    setFlag(QQuickItem::ItemAcceptsDrops, true);
    setFlag(QQuickItem::ItemHasContents, true);
#ifdef USE_INNER
    m_view = new QmlCustomInternalWidget(this);
#endif
    connect(this, &QQuickItem::widthChanged, this, &QmlWidget::updateGeometry);
    connect(this, &QQuickItem::heightChanged, this, &QmlWidget::updateGeometry);
    connect(this, &QQuickItem::visibleChanged, this, &QmlWidget::updateGeometry);
}

void QmlWidget::paint(QPainter *painter)
{
    if (!m_view) {
        qWarning() << "Cannot paint - view is null";
        return;
    }

    m_view->render(painter, QPoint(), QRegion(),
                     QWidget::DrawWindowBackground | QWidget::DrawChildren);
}

void QmlWidget::setViewType(int widgetType)
{
    if (m_viewType == widgetType)
        return;

    qDebug() << "Setting view type from" << m_viewType << "to" << widgetType;
    m_viewType = widgetType;

    initWidget();
    viewTypeChanged();
}

int QmlWidget::viewType()
{
    return m_viewType;
}

void QmlWidget::setFilterType(int filterType)
{
    if (m_filterType == filterType)
        return;

    qDebug() << "Setting filter type from" << m_filterType << "to" << filterType;
    m_filterType = filterType;

    filterTypeChanged();
}

int QmlWidget::filterType()
{
    return m_filterType;
}

void QmlWidget::setHideBuiltinFilter(bool hide)
{
    if (m_hideBuiltinFilter == hide)
        return;

    m_hideBuiltinFilter = hide;

    // Forward to the concrete view (built lazily by initWidget)
    if (m_view) {
        if (TimeLineView *view = dynamic_cast<TimeLineView *>(m_view))
            view->setHideBuiltinFilter(hide);
        else if (ImportTimeLineView *view = dynamic_cast<ImportTimeLineView *>(m_view))
            view->setHideBuiltinFilter(hide);
    }
}

bool QmlWidget::hideBuiltinFilter() const
{
    return m_hideBuiltinFilter;
}

void QmlWidget::initWidget()
{
    if (nullptr == m_view) {
        qDebug() << "Initializing widget with type:" << m_viewType;
        if (0 == m_viewType)
            m_view = new TimeLineView(this);
        else if (1 == m_viewType)
            m_view = new ImportTimeLineView(this);
        else {
            qWarning() << "Unknown view type:" << m_viewType;
            return;
        }
        m_view->setFocusPolicy(Qt::WheelFocus);
        m_view->setAttribute(Qt::WA_WState_Created, true);
        m_view->setAutoFillBackground(true);
        m_view->setVisible(true);
        connectScrollSignals();

        // Replay any hideBuiltinFilter value set before the view existed
        // (QML may evaluate hideBuiltinFilter before viewType triggers init)
        if (m_hideBuiltinFilter) {
            if (TimeLineView *view = dynamic_cast<TimeLineView *>(m_view))
                view->setHideBuiltinFilter(true);
            else if (ImportTimeLineView *view = dynamic_cast<ImportTimeLineView *>(m_view))
                view->setHideBuiltinFilter(true);
        }
    }
}

void QmlWidget::refresh()
{
    if (m_view) {
        qDebug() << "Refreshing widget with type:" << m_viewType;
        m_disableHoverEvent = true;
        m_lastHoveredWidget = nullptr;
        if (m_viewType == Types::WidgetDayView) {
            if (TimeLineView* timeLine = dynamic_cast<TimeLineView*>(m_view))
                timeLine->clearAndStartLayout();
        } else if (m_viewType == Types::WidgetImportedView) {
            if (ImportTimeLineView* importTimeLine = dynamic_cast<ImportTimeLineView*>(m_view))
                importTimeLine->clearAndStartLayout();
        }
        m_disableHoverEvent = false;
    } else {
        qWarning() << "Cannot refresh - view is null";
    }
}

void QmlWidget::navigateToMonth(const QString &month)
{
    if (m_view) {
        qDebug() << "Navigating to month:" << month;
        if (TimeLineView* timeLine = dynamic_cast<TimeLineView*>(m_view))
            timeLine->getThumbnailListView()->navigateToMonth(month);
    } else {
        qWarning() << "Cannot navigate - view is null";
    }
}

QVariantList QmlWidget::allUrls()
{
    if (m_view) {
        qDebug() << "Getting all URLs for view type:" << m_viewType;
        if (m_viewType == Types::WidgetDayView) {
            if (TimeLineView* timeLine = dynamic_cast<TimeLineView*>(m_view))
                return timeLine->getThumbnailListView()->allUrls();
        } else if (m_viewType == Types::WidgetImportedView) {
            if (ImportTimeLineView* importTimeLine = dynamic_cast<ImportTimeLineView*>(m_view))
                return importTimeLine->getListView()->allUrls();
        }
    } else {
        qWarning() << "Cannot get URLs - view is null";
    }

    return QVariantList();
}

void QmlWidget::unSelectAll()
{
    if (m_view) {
        qDebug() << "Unselecting all items";
        if (m_viewType == Types::WidgetDayView) {
            if (TimeLineView* timeLine = dynamic_cast<TimeLineView*>(m_view))
                timeLine->clearAllSelection();
        } else if (m_viewType == Types::WidgetImportedView) {
            if (ImportTimeLineView* importTimeLine = dynamic_cast<ImportTimeLineView*>(m_view))
                importTimeLine->clearAllSelection();
        }
    } else {
        qWarning() << "Cannot unselect - view is null";
    }
}

void QmlWidget::selectUrls(const QStringList &urls)
{
    if (m_view) {
        qDebug() << "Selecting" << urls.size() << "URLs";
        if (m_viewType == Types::WidgetDayView) {
            if (TimeLineView* timeLine = dynamic_cast<TimeLineView*>(m_view))
                timeLine->getThumbnailListView()->selectUrls(urls);
        } else if (m_viewType == Types::WidgetImportedView) {
            if (ImportTimeLineView* importTimeLine = dynamic_cast<ImportTimeLineView*>(m_view))
                importTimeLine->getListView()->selectUrls(urls);
        }
    } else {
        qWarning() << "Cannot select URLs - view is null";
    }
}

void QmlWidget::setFocus(bool arg)
{
    QQuickPaintedItem::setFocus(arg);

    if (!m_view) {
        qWarning() << "Cannot set focus - view is null";
        return;
    }

    qDebug() << "Setting focus to:" << arg;
    if(arg){
        m_view->setFocus();
        forceActiveFocus();
    }
    else{
        m_view->clearFocus();
    }
    setActiveFocusOnTab(arg);
}

void QmlWidget::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    qDebug() << "Geometry changed from" << oldGeometry << "to" << newGeometry;
    if(newGeometry == oldGeometry)
        return;
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    updateGeometry();
}

bool QmlWidget::event(QEvent *e)
{
    if (!m_view) {
        qWarning() << "Cannot process event - view is null";
        return QQuickPaintedItem::event(e);
    }

    static QPoint lastPos;
    static bool isPressed = false;
    static QWidget* pressedWidget = nullptr;
    QObject* pScrollArea = m_view->findChild<QObject*>("qt_scrollarea_viewport");
    
    // 处理鼠标事件
    if (auto mouseEvent = dynamic_cast<QMouseEvent *>(e)) {
        QPoint pos = mouseEvent->pos();

        switch (mouseEvent->type()) {
            case QEvent::MouseButtonPress:
                isPressed = true;
                lastPos = pos;
                pressedWidget = m_view->childAt(pos);
                qDebug() << "Mouse press at" << pos << "on widget:" << pressedWidget;
                break;
            case QEvent::MouseMove:
                if (!isPressed) {
                    pressedWidget = m_view->childAt(pos);
                }
                break;
            case QEvent::MouseButtonRelease:
                isPressed = false;
                qDebug() << "Mouse release at" << pos;
                // 即使鼠标释放在 QmlWidget 范围外，也要处理框选结束
                if (pressedWidget) {
                    QPoint localPos = pressedWidget->mapFrom(m_view, pos);
                    QMouseEvent mappedEvent(QEvent::MouseButtonRelease, localPos, mouseEvent->button(), mouseEvent->buttons(), mouseEvent->modifiers());
                    QCoreApplication::sendEvent(pressedWidget, &mappedEvent);
                    update();
                }
                pressedWidget = nullptr;
                break;
            default:
                break;
        }

        QWidget* targetWidget = isPressed ? pressedWidget : m_view->childAt(pos);
        if (targetWidget) {
            QPoint localPos = targetWidget->mapFrom(m_view, pos);
            QMouseEvent mappedEvent(mouseEvent->type(), localPos, mouseEvent->button(), mouseEvent->buttons(), mouseEvent->modifiers());

            bool handled = QCoreApplication::sendEvent(targetWidget, &mappedEvent);

            if (handled) {
                update();
            }

            // 右键点击事件
            if (mouseEvent->button() == Qt::RightButton && mouseEvent->type() == QEvent::MouseButtonPress) {
                qDebug() << "Right click at" << localPos;
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
        if(pScrollArea) {
            qDebug() << "Forwarding key event to scroll area";
            QCoreApplication::sendEvent(pScrollArea, keyEvent);
        }
    } else if (auto wheelEvent = dynamic_cast<QWheelEvent *>(e)) {
        // 处理鼠标滚轮事件
        // Qt6下，滚轮事件只有转发到scrollArea上，缩略图列表才能在正常接收滚轮事件
        // 转发到其子控件，比如Label或TimeLineDateWidget上，事件都不会路由到缩略图列表
        if(pScrollArea) {
            qDebug() << "Forwarding wheel event to scroll area";
            QCoreApplication::sendEvent(pScrollArea, wheelEvent);
        }
    } else if(e->type() == QEvent::HoverMove) {
        // 处理鼠标悬停事件
        QHoverEvent *hoverEvent = dynamic_cast<QHoverEvent *>(e);
        if (hoverEvent && !m_disableHoverEvent) {
            QWidget* ss = m_view->childAt(hoverEvent->pos());
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
        qDebug() << "Hover enter event received";
        setFocus(true);
    }

    return QQuickPaintedItem::event(e);
}

qreal QmlWidget::scrollPosition() const
{
    return m_scrollPosition;
}

qreal QmlWidget::contentRatio() const
{
    return m_contentRatio;
}

void QmlWidget::setScrollPosition(qreal pos)
{
    if (!m_scrollArea)
        return;

    QScrollBar *vbar = m_scrollArea->verticalScrollBar();
    qreal max = vbar->maximum();
    vbar->setValue(qBound(0, qRound(qBound(0.0, pos, 1.0) * max), vbar->maximum()));
}

void QmlWidget::connectScrollSignals()
{
    if (!m_view)
        return;

    if (m_viewType == Types::WidgetDayView) {
        if (auto *timeLine = dynamic_cast<TimeLineView*>(m_view))
            m_scrollArea = qobject_cast<QAbstractScrollArea*>(timeLine->getThumbnailListView());
    } else if (m_viewType == Types::WidgetImportedView) {
        if (auto *importTimeLine = dynamic_cast<ImportTimeLineView*>(m_view))
            m_scrollArea = qobject_cast<QAbstractScrollArea*>(importTimeLine->getListView());
    }

    if (!m_scrollArea)
        return;

    QScrollBar *vbar = m_scrollArea->verticalScrollBar();
    if (!vbar)
        return;

    auto updateScrollInfo = [this, vbar]() {
        int max = vbar->maximum();
        int val = vbar->value();
        int pageStep = vbar->pageStep();

        qreal newPos = max > 0 ? qreal(val) / max : 0.0;
        qreal newRatio = (max + pageStep) > 0 ? qreal(pageStep) / (max + pageStep) : 1.0;

        if (!qFuzzyCompare(m_scrollPosition, newPos)) {
            m_scrollPosition = newPos;
            Q_EMIT scrollPositionChanged();
        }
        if (!qFuzzyCompare(m_contentRatio, newRatio)) {
            m_contentRatio = newRatio;
            Q_EMIT contentRatioChanged();
        }
    };

    connect(vbar, &QScrollBar::valueChanged, this, updateScrollInfo);
    connect(vbar, &QScrollBar::rangeChanged, this, updateScrollInfo);
    updateScrollInfo();
}

void QmlWidget::updateGeometry()
{
    if (!isVisible() || !m_view) {
        qDebug() << "Skipping geometry update - widget not visible or view is null";
        return;
    }

    QPointF newPos(0, 0);
    newPos = mapToItem(0, newPos);
    qDebug() << "Updating geometry, top left mapped to window:" << newPos;

    if(this->window()){
        newPos += this->window()->position();
        qDebug() << "Top left mapped to screen:" << newPos;
    }
    QRectF absRect(newPos, contentsBoundingRect().size());
    m_view->setGeometry(absRect.toRect());
}
