#include "imagesvgitem.h"

#if !defined(QT_NO_GRAPHICSVIEW) && !defined(QT_NO_WIDGETS)

#include "qpainter.h"
#include "qstyleoption.h"
#include "dsvgrenderer.h"
#include "qdebug.h"

//#include "private/qobject_p.h"
//#include "private/qgraphicsitem_p.h"

QT_BEGIN_NAMESPACE

//class ImageSvgItemPrivate : public QGraphicsItemPrivate
//{
//public:
//    Q_DECLARE_PUBLIC(ImageSvgItem)
//    ImageSvgItemPrivate()
//        : renderer(nullptr), shared(false)
//    {
//    }

//    void init(QGraphicsItem *parent)
//    {
//        Q_Q(ImageSvgItem);
//        q->setParentItem(parent);
//        renderer = new DSvgRenderer(q);
//        q->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
//        q->setMaximumCacheSize(QSize(1024, 768));
//    }

//    inline void updateDefaultSize()
//    {
//        QRectF bounds;
//        if (elemId.isEmpty()) {
//            bounds = QRectF(QPointF(0, 0), renderer->defaultSize());
//        } else {
//            bounds = renderer->boundsOnElement(elemId);
//        }
//        if (boundingRect.size() != bounds.size()) {
//            q_func()->prepareGeometryChange();
//            boundingRect.setSize(bounds.size());
//        }
//    }

//    DSvgRenderer *renderer;
//    QRectF boundingRect;
//    bool shared;
//    QString elemId;
//};

ImageSvgItem::ImageSvgItem(QGraphicsItem *parent)
    : QGraphicsObject(parent)/*QGraphicsObject(*new ImageSvgItemPrivate(), nullptr)*/
{
    setParentItem(parent);
    m_renderer = new DSvgRenderer(this);
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    setMaximumCacheSize(QSize(1024, 768));
}

ImageSvgItem::ImageSvgItem(const QString &fileName, QGraphicsItem *parent)
    : QGraphicsObject(parent)/*QGraphicsObject(*new ImageSvgItemPrivate(), nullptr)*/
{
    setParentItem(parent);
    m_renderer = new DSvgRenderer(this);
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    setMaximumCacheSize(QSize(1024, 768));
    m_renderer->load(fileName);
    updateDefaultSize();
}

DSvgRenderer *ImageSvgItem::renderer() const
{
    return m_renderer;
}

QRectF ImageSvgItem::boundingRect() const
{
    return m_boundingRect;
}

static void qt_graphicsItem_highlightSelected(
    QGraphicsItem *item, QPainter *painter, const QStyleOptionGraphicsItem *option)
{
    const QRectF murect = painter->transform().mapRect(QRectF(0, 0, 1, 1));
    if (qFuzzyIsNull(qMax(murect.width(), murect.height())))
        return;

    const QRectF mbrect = painter->transform().mapRect(item->boundingRect());
    if (qMin(mbrect.width(), mbrect.height()) < qreal(1.0))
        return;

    qreal itemPenWidth;
    switch (item->type()) {
    case QGraphicsEllipseItem::Type:
        itemPenWidth = static_cast<QGraphicsEllipseItem *>(item)->pen().widthF();
        break;
    case QGraphicsPathItem::Type:
        itemPenWidth = static_cast<QGraphicsPathItem *>(item)->pen().widthF();
        break;
    case QGraphicsPolygonItem::Type:
        itemPenWidth = static_cast<QGraphicsPolygonItem *>(item)->pen().widthF();
        break;
    case QGraphicsRectItem::Type:
        itemPenWidth = static_cast<QGraphicsRectItem *>(item)->pen().widthF();
        break;
    case QGraphicsSimpleTextItem::Type:
        itemPenWidth = static_cast<QGraphicsSimpleTextItem *>(item)->pen().widthF();
        break;
    case QGraphicsLineItem::Type:
        itemPenWidth = static_cast<QGraphicsLineItem *>(item)->pen().widthF();
        break;
    default:
        itemPenWidth = 1.0;
    }
    const qreal pad = itemPenWidth / 2;

    const qreal penWidth = 0; // cosmetic pen

    const QColor fgcolor = option->palette.windowText().color();
    const QColor bgcolor( // ensure good contrast against fgcolor
        fgcolor.red()   > 127 ? 0 : 255,
        fgcolor.green() > 127 ? 0 : 255,
        fgcolor.blue()  > 127 ? 0 : 255);

    painter->setPen(QPen(bgcolor, penWidth, Qt::SolidLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(item->boundingRect().adjusted(pad, pad, -pad, -pad));

    painter->setPen(QPen(option->palette.windowText(), 0, Qt::DashLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(item->boundingRect().adjusted(pad, pad, -pad, -pad));
}

void ImageSvgItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                         QWidget *widget)
{
//    Q_UNUSED(option);
    Q_UNUSED(widget);
    if (!m_renderer->isValid())
        return;

    if (m_elemId.isEmpty())
        m_renderer->render(painter, m_boundingRect);
    else
        m_renderer->render(painter, m_elemId, m_boundingRect);

    if (option->state & QStyle::State_Selected)
        qt_graphicsItem_highlightSelected(this, painter, option);
}

int ImageSvgItem::type() const
{
    return Type;
}

void ImageSvgItem::setMaximumCacheSize(const QSize &size)
{
    Q_UNUSED(size);
    //QGraphicsItem::d_ptr->setExtra(QGraphicsItemPrivate::ExtraMaxDeviceCoordCacheSize, size);
    update();
}

QSize ImageSvgItem::maximumCacheSize() const
{
    return QSize();//QGraphicsItem::d_ptr->extra(QGraphicsItemPrivate::ExtraMaxDeviceCoordCacheSize).toSize();
}

void ImageSvgItem::updateDefaultSize()
{
    QRectF bounds;
    if (m_elemId.isEmpty()) {
        bounds = QRectF(QPointF(0, 0), m_renderer->defaultSize());
    } else {
        bounds = m_renderer->boundsOnElement(m_elemId);
    }
    if (m_boundingRect.size() != bounds.size()) {
        prepareGeometryChange();
        m_boundingRect.setSize(bounds.size());
    }
}

void ImageSvgItem::setElementId(const QString &id)
{
    m_elemId = id;
    updateDefaultSize();
    update();
}

QString ImageSvgItem::elementId() const
{
    return m_elemId;
}

void ImageSvgItem::setSharedRenderer(DSvgRenderer *renderer)
{
    if (!m_shared)
        delete m_renderer;

    m_renderer = renderer;
    m_shared = true;

    updateDefaultSize();

    update();
}

void ImageSvgItem::setCachingEnabled(bool caching)
{
    setCacheMode(caching ? QGraphicsItem::DeviceCoordinateCache : QGraphicsItem::NoCache);
}

bool ImageSvgItem::isCachingEnabled() const
{
    return cacheMode() != QGraphicsItem::NoCache;
}

#endif // QT_NO_WIDGETS
