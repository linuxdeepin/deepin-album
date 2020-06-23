/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
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
#include "imageview.h"

#include <QDebug>
#include <QFile>
#include <QOpenGLWidget>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QMovie>
#include <QGraphicsRectItem>
#include <QGraphicsSvgItem>
#include <QGraphicsPixmapItem>
#include <QPaintEvent>
#include <QtConcurrent>
#include <QHBoxLayout>
#include <qmath.h>
#include <QScrollBar>
#include <QGestureEvent>
#include <QSvgRenderer>

#include "graphicsitem.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "utils/snifferimageformat.h"
#include "utils/unionimage.h"
#include "application.h"
#include <DGuiApplicationHelper>
#include "controller/signalmanager.h"

#include <imageengine/imageengineapi.h>

#ifndef QT_NO_OPENGL
#include <QGLWidget>
#endif

#include <sys/inotify.h>

DWIDGET_USE_NAMESPACE

namespace {

const QColor LIGHT_CHECKER_COLOR = QColor("#FFFFFF");
const QColor DARK_CHECKER_COLOR = QColor("#CCCCCC");

const qreal MAX_SCALE_FACTOR = 20.0;
const qreal MIN_SCALE_FACTOR = 0.02;
const int MAX_WIDTH_HEIGHT = 3500;      //最大尺寸分辨率，超过则加载缩略图替换


QVariantList cachePixmap(const QString &path)
{
    QImage tImg;

    QString format = DetectImageFormat(path);
    if (format.isEmpty()) {
        QImageReader reader(path);
        reader.setAutoTransform(true);
        if (reader.canRead()) {
            tImg = reader.read();
        }
    } else {
        QImageReader readerF(path, format.toLatin1());
        readerF.setAutoTransform(true);
        if (readerF.canRead()) {
            tImg = readerF.read();
        } else {
            qWarning() << "can't read image:" << readerF.errorString()
                       << format;
            tImg = QImage(path);
        }
    }
    QPixmap p = QPixmap::fromImage(tImg);
    if (QFileInfo(path).exists() && p.isNull()) {
        //判定为损坏图片
        p = utils::image::getDamagePixmap (DApplicationHelper::instance ()->themeType () == DApplicationHelper::LightType);
    }
    QVariantList vl;
    vl << QVariant(path) << QVariant(p);
    return vl;
}

}  // namespace

ImageView::ImageView(QWidget *parent)
    : QGraphicsView(parent)
    , m_renderer(Native)
    , m_pool(new QThreadPool(this))
//    , m_svgItem(nullptr)
    , m_movieItem(nullptr)
    , m_pixmapItem(nullptr)
    , m_bLoadmemory(false)
    , m_rotateControler(nullptr)
{
    onThemeChanged(dApp->viewerTheme->getCurrentTheme());
    setScene(new QGraphicsScene(this));
    setMouseTracking(true);
    setTransformationAnchor(AnchorUnderMouse);
    setDragMode(ScrollHandDrag);
    setViewportUpdateMode(FullViewportUpdate);
    setAcceptDrops(false);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShape(QFrame::Shape::NoFrame);

    viewport()->setCursor(Qt::ArrowCursor);

    grabGesture(Qt::PinchGesture);
    grabGesture(Qt::SwipeGesture);

    connect(&m_watcher, SIGNAL(finished()), this, SLOT(onCacheFinish()));
    connect(dApp->viewerTheme, &ViewerThemeManager::viewerThemeChanged, this,
            &ImageView::onThemeChanged);
    m_pool->setMaxThreadCount(1);

//    m_toast = new Toast(this);
//    m_toast->setIcon(":/resources/common/images/dialog_warning.svg");
//    m_toast->setText(tr("This file contains multiple pages, please use Evince to view all pages."));
//    m_toast->hide();
    // TODO
    //    QPixmap pm(12, 12);
    //    QPainter pmp(&pm);
    //    pmp.fillRect(0, 0, 6, 6, LIGHT_CHECKER_COLOR);
    //    pmp.fillRect(6, 6, 6, 6, LIGHT_CHECKER_COLOR);
    //    pmp.fillRect(0, 6, 6, 6, DARK_CHECKER_COLOR);
    //    pmp.fillRect(6, 0, 6, 6, DARK_CHECKER_COLOR);
    //    pmp.end();

    //    QPalette pal = palette();
    //    pal.setBrush(backgroundRole(), QBrush(pm));
    //    setAutoFillBackground(true);
    //    setPalette(pal);

    // Use openGL to render by default
    //    setRenderer(OpenGL);
    QObject::connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [ = ]() {
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::DarkType) {
            m_backgroundColor = utils::common::DARK_BACKGROUND_COLOR;
        } else {
            m_backgroundColor = utils::common::LIGHT_BACKGROUND_COLOR;
        }
        update();
    });

    m_imgFileWatcher = new CFileWatcher(this);
    connect(m_imgFileWatcher, &CFileWatcher::fileChanged, this, &ImageView::onImgFileChanged);
    m_isChangedTimer = new QTimer(this);
    QObject::connect(m_isChangedTimer, &QTimer::timeout, this, [ = ] {
        dApp->m_imageloader->updateImageLoader(QStringList(m_path));
        setImage(m_path);
        m_isChangedTimer->stop();
    });

}

void ImageView::clear()
{
    if (m_pixmapItem != nullptr) {
        delete m_pixmapItem;
        m_pixmapItem = nullptr;
    }
    m_movieItem = nullptr;
    m_imgSvgItem = nullptr;
    scene()->clear();
}

void ImageView::setImage(const QString &path)
{
    // Empty path will cause crash in release-build mode
    if (path.isEmpty()) {
        return;
    }
    m_path = path;
    m_imgFileWatcher->clear();
    m_imgFileWatcher->addWather(m_path);
    QString strfixL = QFileInfo(path).suffix().toLower();
    QGraphicsScene *s = scene();
    QFileInfo fi(path);

    // The suffix of svf file should be svg
    if ( strfixL == "svg" && DSvgRenderer().load(path)) {
        m_movieItem = nullptr;
//        m_pixmapItem = nullptr;
        if (m_pixmapItem != nullptr) {
            delete m_pixmapItem;
            m_pixmapItem = nullptr;
        }
        s->clear();
        resetTransform();

        DSvgRenderer *svgRenderer = new DSvgRenderer;
        svgRenderer->load(path);
        m_imgSvgItem = new ImageSvgItem();
        m_imgSvgItem->setSharedRenderer(svgRenderer);

//        m_svgItem = new QGraphicsSvgItem(path);
//        m_svgItem->setFlags(QGraphicsItem::ItemClipsToShape);
//        m_svgItem->setCacheMode(QGraphicsItem::NoCache);
//        m_svgItem->setZValue(0);
        // Make sure item show in center of view after reload
//        setSceneRect(m_svgItem->boundingRect());
//        s->addItem(m_svgItem);
        setSceneRect(m_imgSvgItem->boundingRect());
        s->addItem(m_imgSvgItem);
        emit imageChanged(path);

    } else {
        m_imgSvgItem = nullptr;
        QList<QByteArray> fList =  QMovie::supportedFormats(); //"gif","mng","webp"
        //QMovie can't read frameCount of "mng" correctly,so change
        //the judge way to solve the problem
        if (fList.contains(strfixL.toUtf8().data())) {
            if (m_pixmapItem != nullptr) {
                delete m_pixmapItem;
                m_pixmapItem = nullptr;
            }

            s->clear();
            resetTransform();
            m_movieItem = new GraphicsMovieItem(path, strfixL);
            m_movieItem->start();
            // Make sure item show in center of view after reload
            setSceneRect(m_movieItem->boundingRect());
            s->addItem(m_movieItem);
            emit imageChanged(path);
        } else {
            m_movieItem = nullptr;
            qDebug() << "Start cache pixmap: " << path;
            QImageReader imagreader(path);      //取原图的分辨率
            int w = imagreader.size().width();
            int h = imagreader.size().height();
            if (w > MAX_WIDTH_HEIGHT || h > MAX_WIDTH_HEIGHT) { //分辨率较大
                m_bLoadmemory = true;
            } else {
                m_bLoadmemory = false;
            }
            if (m_bLoadmemory) {
                scene()->clear();
                resetTransform();
                ImageDataSt data;   //内存中的数据
                ImageEngineApi::instance()->getImageData(path, data);
                QPixmap pix = data.imgpixmap.scaled(w, h, Qt::KeepAspectRatio); //缩放到原图大小
                m_pixmapItem = new GraphicsPixmapItem(pix);
                m_pixmapItem->setTransformationMode(Qt::SmoothTransformation);
                // Make sure item show in center of view after reload
                setSceneRect(m_pixmapItem->boundingRect());
                scene()->addItem(m_pixmapItem);
                autoFit();
                emit imageChanged(path);
            }

            QFuture<QVariantList> f = QtConcurrent::run(m_pool, cachePixmap, path);
            if (m_watcher.isRunning()) {
                m_watcher.cancel();
                m_watcher.waitForFinished();
            }
            m_watcher.setFuture(f);
            emit hideNavigation();
        }
    }
}

void ImageView::setRenderer(RendererType type)
{
    m_renderer = type;

    if (m_renderer == OpenGL) {
#ifndef QT_NO_OPENGL
        setViewport(new QOpenGLWidget());
#endif
    } else {
        setViewport(new QWidget);
    }
}

void ImageView::setScaleValue(qreal v)
{
    scale(v, v);

    const qreal irs = imageRelativeScale();
    // Rollback
    if ((v < 1 && irs <= MIN_SCALE_FACTOR)) {
        const qreal minv = MIN_SCALE_FACTOR / irs;
        scale(minv, minv);
    } else if (v > 1 && irs >= MAX_SCALE_FACTOR) {
        const qreal maxv = MAX_SCALE_FACTOR / irs;
        scale(maxv, maxv);
    } else {
        m_isFitImage = false;
        m_isFitWindow = false;
    }

    qreal rescale = imageRelativeScale();
    if (rescale - 1 > -0.01 &&
            rescale - 1 < 0.01) {
        emit checkAdaptImageBtn();
    } else {
        emit disCheckAdaptImageBtn();
    }

    qreal wrs = windowRelativeScale();
    if (rescale - wrs > -0.01 &&
            rescale - wrs < 0.01) {
        emit checkAdaptScreenBtn();
    } else {
        emit disCheckAdaptScreenBtn();
    }
    emit scaled(imageRelativeScale() * 100);
    emit showScaleLabel();
    emit transformChanged();
}

void ImageView::autoFit()
{
    //确认场景加载出来后，才能调用场景内的item
//    if (!scene()->isActive())
//        return;
    if (image().isNull())
        return;
    QSize image_size = image().size();
    if ((image_size.width() >= width() ||
            image_size.height() >= height()) &&
            width() > 0 && height() > 0) {
        fitWindow();
    } else {
        fitImage();
    }
}

const QImage ImageView::image()
{
    if (m_movieItem) {           // bit-map
        return m_movieItem->pixmap().toImage();
    } else if (m_pixmapItem) {
        //FIXME: access to m_pixmapItem will crash
        if (nullptr == m_pixmapItem) {  //add to slove crash by shui
            return QImage();
        }
        return m_pixmapItem->pixmap().toImage();
//    } else if (m_svgItem) {    // svg
    } else if (m_imgSvgItem) {    // svg
        QImage image(m_imgSvgItem->renderer()->defaultSize(), QImage::Format_ARGB32_Premultiplied);
        image.fill(QColor(0, 0, 0, 0));
        QPainter imagePainter(&image);
        m_imgSvgItem->renderer()->render(&imagePainter);
        imagePainter.end();
        return image;
    } else {
        return QImage();
    }
}

void ImageView::fitWindow()
{
    qreal wrs = windowRelativeScale();
    resetTransform();
    scale(wrs, wrs);
    emit checkAdaptScreenBtn();
    if (wrs - 1 > -0.01 &&
            wrs - 1 < 0.01) {
        emit checkAdaptImageBtn();
    } else {
        emit disCheckAdaptImageBtn();
    }
    m_isFitImage = false;
    m_isFitWindow = true;
    scaled(imageRelativeScale() * 100);
    emit transformChanged();
}

void ImageView::fitImage()
{
    qreal wrs = windowRelativeScale();
    resetTransform();
    scale(1, 1);
    emit checkAdaptImageBtn();
    if (wrs - 1 > -0.01 &&
            wrs - 1 < 0.01) {
        emit checkAdaptScreenBtn();
    } else {
        emit disCheckAdaptScreenBtn();
    }
    m_isFitImage = true;
    m_isFitWindow = false;
    scaled(imageRelativeScale() * 100);
    emit transformChanged();
}

void ImageView::rotateClockWise()
{
//    const QString suffix = QFileInfo(m_path).suffix();
//    if (suffix.toUpper().compare("SVG") == 0) {
//        ImageSVGConvertThread *imgSVGThread = new ImageSVGConvertThread;
//        imgSVGThread->setData(QStringList() << m_path, 90);
//        connect(imgSVGThread, &ImageSVGConvertThread::updateImages, this, &ImageView::updateImages);
//        connect(imgSVGThread, &ImageSVGConvertThread::finished, imgSVGThread, &QObject::deleteLater);
//        imgSVGThread->start();
//    } else {
//        utils::image::rotate(m_path, 90);
//        setImage(m_path);
//        dApp->m_imageloader->updateImageLoader(QStringList(m_path));
//    }

    QString errMsg;
    if (!UnionImage_NameSpace::rotateImageFIle(90, m_path, errMsg)) {
        qDebug() << errMsg;
        return;
    }
    setImage(m_path);

    dApp->m_imageloader->updateImageLoader(QStringList(m_path));
}

void ImageView::rotateCounterclockwise()
{
//    const QString suffix = QFileInfo(m_path).suffix();
//    if (suffix.toUpper().compare("SVG") == 0) {
//        ImageSVGConvertThread *imgSVGThread = new ImageSVGConvertThread;
//        imgSVGThread->setData(QStringList() << m_path, -90);
//        connect(imgSVGThread, &ImageSVGConvertThread::updateImages, this, &ImageView::updateImages);
//        connect(imgSVGThread, &ImageSVGConvertThread::finished, imgSVGThread, &QObject::deleteLater);
//        imgSVGThread->start();
//    } else {
//        utils::image::rotate(m_path, - 90);
//        setImage(m_path);
//        dApp->m_imageloader->updateImageLoader(QStringList(m_path));
//    }
    QString errMsg;
    if (!UnionImage_NameSpace::rotateImageFIle(-90, m_path, errMsg)) {
        qDebug() << errMsg;
        return;
    }
    setImage(m_path);
    dApp->m_imageloader->updateImageLoader(QStringList(m_path));
}

void ImageView::centerOn(qreal x, qreal y)
{
    QGraphicsView::centerOn(x, y);
    emit transformChanged();
}

qreal ImageView::imageRelativeScale() const
{
    // vertical scale factor are equal to the horizontal one
    return transform().m11();
}

qreal ImageView::windowRelativeScale() const
{
    QRectF bf = sceneRect();
    if (1.0 * width() / height() > 1.0 * bf.width() / bf.height()) {
        return 1.0 * height() / bf.height();
    } else {
        return 1.0 * width() / bf.width();
    }
}

const QRectF ImageView::imageRect() const
{
    QRectF br(mapFromScene(0, 0), sceneRect().size());
    QTransform tf = transform();
    br.translate(tf.dx(), tf.dy());
    br.setWidth(br.width() * tf.m11());
    br.setHeight(br.height() * tf.m22());

    return br;
}

const QString ImageView::path() const
{
    return m_path;
}

QPoint ImageView::mapToImage(const QPoint &p) const
{
    return viewportTransform().inverted().map(p);
}

QRect ImageView::mapToImage(const QRect &r) const
{
    return viewportTransform().inverted().mapRect(r);
}

QRect ImageView::visibleImageRect() const
{
    return mapToImage(rect()) & QRect(0, 0, static_cast<int>(sceneRect().width()), static_cast<int>(sceneRect().height()));
}

bool ImageView::isWholeImageVisible() const
{
    const QRect &r = visibleImageRect();
    const QRectF &sr = sceneRect();

    return r.width() >= sr.width() && r.height() >= sr.height();
}

bool ImageView::isFitImage() const
{
    return m_isFitImage;
}

bool ImageView::isFitWindow() const
{
    return m_isFitWindow;
}

void ImageView::setHighQualityAntialiasing(bool highQualityAntialiasing)
{
#ifndef QT_NO_OPENGL
    setRenderHint(QPainter::HighQualityAntialiasing, highQualityAntialiasing);
#else
    Q_UNUSED(highQualityAntialiasing);
#endif
}

void ImageView::onImgFileChanged(const QString &ddfFile, int tp)
{
    Q_UNUSED(ddfFile)
    Q_UNUSED(tp)
    m_isChangedTimer->start(200);
}

void ImageView::mouseDoubleClickEvent(QMouseEvent *e)
{
    emit doubleClicked();
    QGraphicsView::mouseDoubleClickEvent(e);
}

void ImageView::mouseReleaseEvent(QMouseEvent *e)
{
    QGraphicsView::mouseReleaseEvent(e);

    viewport()->setCursor(Qt::ArrowCursor);
}

void ImageView::mousePressEvent(QMouseEvent *e)
{
    QGraphicsView::mousePressEvent(e);

    viewport()->unsetCursor();
    viewport()->setCursor(Qt::ArrowCursor);

    emit clicked();
}

void ImageView::mouseMoveEvent(QMouseEvent *e)
{
    if (!(e->buttons() | Qt::NoButton)) {
        viewport()->setCursor(Qt::ArrowCursor);

        emit mouseHoverMoved();
    } else {
        QGraphicsView::mouseMoveEvent(e);
        viewport()->setCursor(Qt::ClosedHandCursor);

        emit transformChanged();
    }

    emit dApp->signalM->sigMouseMove();
}

void ImageView::leaveEvent(QEvent *e)
{
    dApp->restoreOverrideCursor();

    QGraphicsView::leaveEvent(e);
}

void ImageView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
//    m_toast->move(width() / 2 - m_toast->width() / 2,
//                  height() - 80 - m_toast->height() / 2 - 11);
}

void ImageView::paintEvent(QPaintEvent *event)
{
    QGraphicsView::paintEvent(event);
//    update();
}

void ImageView::dragEnterEvent(QDragEnterEvent *e)
{
    const QMimeData *mimeData = e->mimeData();
    if (!utils::base::checkMimeData(mimeData)) {
        return;
    }
    e->accept();
}

void ImageView::drawBackground(QPainter *painter, const QRectF &rect)
{
//    QPixmap pm(12, 12);
//    QPainter pmp(&pm);
//    //TODO: the transparent box
//    //should not be scaled with the image
//    pmp.fillRect(0, 0, 6, 6, LIGHT_CHECKER_COLOR);
//    pmp.fillRect(6, 6, 6, 6, LIGHT_CHECKER_COLOR);
//    pmp.fillRect(0, 6, 6, 6, DARK_CHECKER_COLOR);
//    pmp.fillRect(6, 0, 6, 6, DARK_CHECKER_COLOR);
//    pmp.end();

    painter->save();
    painter->fillRect(rect, m_backgroundColor);

//    QPixmap currentImage(m_path);
//    if (!currentImage.isNull())
//        painter->fillRect(currentImage.rect(), QBrush(pm));
    painter->restore();
}

bool ImageView::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture)
        handleGestureEvent(static_cast<QGestureEvent *>(event));

    return QGraphicsView::event(event);
}

void ImageView::onCacheFinish()
{
    QVariantList vl = m_watcher.result();
    if (vl.length() == 2) {
        const QString path = vl.first().toString();
        QPixmap pixmap = vl.last().value<QPixmap>();
        pixmap.setDevicePixelRatio(devicePixelRatioF());
        if (path == m_path) {
            if (m_bLoadmemory) {
                if (!m_pixmapItem)
                    return;
                m_pixmapItem->setPixmap(pixmap);
                autoFit();
                this->update();
            } else {
                if (m_pixmapItem != nullptr) {
                    delete m_pixmapItem;
                    m_pixmapItem = nullptr;
                }
                scene()->clear();
                resetTransform();
                m_pixmapItem = new GraphicsPixmapItem(pixmap);
                m_pixmapItem->setTransformationMode(Qt::SmoothTransformation);
                // Make sure item show in center of view after reload
                setSceneRect(m_pixmapItem->boundingRect());
                scene()->addItem(m_pixmapItem);
                emit imageChanged(path);
            }
        }
    }
}

void ImageView::onThemeChanged(ViewerThemeManager::AppTheme theme)
{
    if (theme == ViewerThemeManager::Dark) {
        m_backgroundColor = utils::common::DARK_BACKGROUND_COLOR;
        m_loadingIconPath = utils::view::DARK_LOADINGICON;
    } else {
        m_backgroundColor = utils::common::LIGHT_BACKGROUND_COLOR;
        m_loadingIconPath = utils::view::LIGHT_LOADINGICON;
    }
    update();
}

void ImageView::scaleAtPoint(QPoint pos, qreal factor)
{
    // Remember zoom anchor point.
    const QPointF targetPos = pos;
    const QPointF targetScenePos = mapToScene(targetPos.toPoint());

    // Do the scaling.
    setScaleValue(factor);

    // Restore the zoom anchor point.
    //
    // The Basic idea here is we don't care how the scene is scaled or transformed,
    // we just want to restore the anchor point to the target position we've
    // remembered, in the coordinate of the view/viewport.
    const QPointF curPos = mapFromScene(targetScenePos);
    const QPointF centerPos = QPointF(width() / 2.0, height() / 2.0) + (curPos - targetPos);
    const QPointF centerScenePos = mapToScene(centerPos.toPoint());
    centerOn(centerScenePos.x(), centerScenePos.y());
}

void ImageView::handleGestureEvent(QGestureEvent *gesture)
{
    if (QGesture *swipe = gesture->gesture(Qt::SwipeGesture))
        swipeTriggered(static_cast<QSwipeGesture *>(swipe));
    else if (QGesture *pinch = gesture->gesture(Qt::PinchGesture))
        pinchTriggered(static_cast<QPinchGesture *>(pinch));
}

void ImageView::pinchTriggered(QPinchGesture *gesture)
{
    QPoint pos = mapFromGlobal(gesture->centerPoint().toPoint());
    scaleAtPoint(pos, gesture->scaleFactor());
}

void ImageView::swipeTriggered(QSwipeGesture *gesture)
{
    if (gesture->state() == Qt::GestureFinished) {
        if (gesture->horizontalDirection() == QSwipeGesture::Left
                || gesture->verticalDirection() == QSwipeGesture::Up) {
            emit nextRequested();
        } else {
            emit previousRequested();
        }
    }

}

void ImageView::updateImages(const QStringList &path)
{
    dApp->m_imageloader->updateImageLoader(path);
    //等待svg图片转换完成后在加载
    setImage(m_path);
}

void ImageView::wheelEvent(QWheelEvent *event)
{
    qreal factor = qPow(1.2, event->delta() / 240.0);
    scaleAtPoint(event->pos(), factor);

    event->accept();
}

CFileWatcher::CFileWatcher(QObject *parent): QThread (parent)
{
    _handleId = inotify_init();
}

CFileWatcher::~CFileWatcher()
{
    clear();
}

bool CFileWatcher::isVaild()
{
    return (_handleId != -1);
}

void CFileWatcher::addWather(const QString &path)
{
    QMutexLocker loker(&_mutex);
    if (!isVaild())
        return;

    QFileInfo info(path);
    if (!info.exists() || !info.isFile()) {
        return;
    }

    if (watchedFiles.find(path) != watchedFiles.end()) {
        return;
    }

    std::string sfile = path.toStdString();
    int fileId = inotify_add_watch(_handleId, sfile.c_str(), IN_MODIFY | IN_DELETE_SELF | IN_MOVE_SELF);

    watchedFiles.insert(path, fileId);
    watchedFilesId.insert(fileId, path);

    if (!_running) {
        _running = true;
        start();
    }
}

void CFileWatcher::removePath(const QString &path)
{
    QMutexLocker loker(&_mutex);

    if (!isVaild())
        return;

    auto itf = watchedFiles.find(path);
    if (itf != watchedFiles.end()) {
        inotify_rm_watch(_handleId, itf.value());

        watchedFilesId.remove(itf.value());
        watchedFiles.erase(itf);
    }
}

void CFileWatcher::clear()
{
    QMutexLocker loker(&_mutex);

    for (auto it : watchedFiles) {
        inotify_rm_watch(_handleId, it);
    }
    watchedFilesId.clear();
    watchedFiles.clear();
}

void CFileWatcher::run()
{
    doRun();
}

void CFileWatcher::doRun()
{
    if (!isVaild())
        return;

    char name[1024];
    auto freadsome = [ = ](void *dest, size_t remain, FILE * file) {
        char *offset = reinterpret_cast<char *>(dest);
        while (remain) {
            size_t n = fread(offset, 1, remain, file);
            if (n == 0) {
                return -1;
            }

            remain -= n;
            offset += n;
        }
        return 0;
    };

    FILE *watcher_file = fdopen(_handleId, "r");

    while (true) {
        inotify_event event;
        if ( -1 == freadsome(&event, sizeof(event), watcher_file) ) {
            qWarning() << "------------- freadsome error !!!!!---------- ";
        }
        if (event.len) {
            freadsome(name, event.len, watcher_file);
        } else {
            QMutexLocker loker(&_mutex);
            auto itf = watchedFilesId.find(event.wd);
            if (itf != watchedFilesId.end()) {
                //qDebug() << "file = " << itf.value() << " event.wd = " << event.wd << "event.mask = " << event.mask;

                if (event.mask & IN_MODIFY) {
                    emit fileChanged(itf.value(), EFileModified);
                } else if (event.mask & IN_MOVE_SELF) {
                    emit fileChanged(itf.value(), EFileMoved);
                } else if (event.mask & IN_DELETE_SELF) {
                    emit fileChanged(itf.value(), EFileMoved);
                }
            }
        }
    }
}
