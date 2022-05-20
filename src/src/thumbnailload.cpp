#include "thumbnailload.h"
#include "unionimage/unionimage.h"
#include <QPainter>

ThumbnailLoad::ThumbnailLoad()
    : QQuickImageProvider(QQuickImageProvider::Image)
{

}

QImage ThumbnailLoad::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QString tempPath = QUrl(id).toLocalFile();
    QImage Img;
    QString error;

    if (!m_imgMap.keys().contains(tempPath)) {
        LibUnionImage_NameSpace::loadStaticImageFromFile(tempPath, Img, error);
        QImage reImg = Img.scaled(100, 100);
        m_imgMap[tempPath] = reImg;
        return reImg;
    } else {
        return m_imgMap[tempPath];
    }

}

QPixmap ThumbnailLoad::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    QString tempPath = QUrl(id).toLocalFile();
    QImage Img;
    QString error;
    LibUnionImage_NameSpace::loadStaticImageFromFile(tempPath, Img, error);
    return QPixmap::fromImage(Img);
}

bool ThumbnailLoad::imageIsNull(const QString &path)
{
    QString tempPath = QUrl(path).toLocalFile();
    if (m_imgMap.find(tempPath) != m_imgMap.end()){
        return m_imgMap[tempPath].isNull();
    }

    return false;
}

LoadImage::LoadImage(QObject *parent) :
    QObject(parent)
{
    m_pThumbnail = new ThumbnailLoad();
    m_viewLoad = new ViewLoad();
    m_publisher = new ImagePublisher(this);
}

double LoadImage::getFitWindowScale(const QString &path, double WindowWidth, double WindowHeight)
{
    return m_viewLoad->getFitWindowScale(path, WindowWidth, WindowHeight);
}

bool LoadImage::imageIsNull(const QString &path)
{
    return m_pThumbnail->imageIsNull(path);
}

int LoadImage::getImageWidth(const QString &path)
{
    return m_viewLoad->getImageWidth(path);
}

int LoadImage::getImageHeight(const QString &path)
{
    return m_viewLoad->getImageHeight(path);
}

double LoadImage::getrealWidthHeightRatio(const QString &path)
{
    double width = double(m_viewLoad->getImageWidth(path));
    double height = double(m_viewLoad->getImageHeight(path));
    return  width/height;
}

void LoadImage::loadThumbnail(const QString path)
{
    QString tempPath = QUrl(path).toLocalFile();
    qDebug() << "----path--" << tempPath;
    QImage Img;
    QString error;
    if (LibUnionImage_NameSpace::loadStaticImageFromFile(tempPath, Img, error)) {
        m_pThumbnail->m_Img = Img;
        emit callQmlRefeshImg();
    } else {
        qDebug() << "load failded,the error is:" << error;
    }
}

void LoadImage::catThumbnail(const QStringList &list)
{
    if (list.size() < 1) {
        return;
    }
    for (QString path : list) {
        QString imgPath = path;

        if (imgPath.startsWith("file://"))
            imgPath.remove(0, 7);
        QImage tImg(imgPath);
        //保持横纵比裁切
        if (abs((tImg.width() - tImg.height()) * 10 / tImg.width()) >= 1) {
            QRect rect = tImg.rect();
            int x = rect.x() + tImg.width() / 2;
            int y = rect.y() + tImg.height() / 2;
            if (tImg.width() > tImg.height()) {
                x = x - tImg.height() / 2;
                y = 0;
                tImg = tImg.copy(x, y, tImg.height(), tImg.height());
            } else {
                y = y - tImg.width() / 2;
                x = 0;
                tImg = tImg.copy(x, y, tImg.width(), tImg.width());
            }
        }
        //压缩画质
        if (0 != tImg.height() && 0 != tImg.width() && (tImg.height() / tImg.width()) < 10 && (tImg.width() / tImg.height()) < 10) {
            if (tImg.height() != /*m_height*/100 || tImg.width() != /*m_with*/100) {
                if (tImg.height() >= tImg.width()) {
                    tImg = tImg.scaledToWidth(/*m_with*/100,  Qt::FastTransformation);
                } else if (tImg.height() <= tImg.width()) {
                    tImg = tImg.scaledToHeight(/*m_height*/100,  Qt::FastTransformation);
                }
            }
        }
//        tImg.save(imgPath, "PNG");
    }
}

void  LoadImage::loadThumbnails(const QStringList list)
{
    QImage Img;
    QString error;
    for (QString path : list) {
        loadThumbnail(path);
    }
}


ViewLoad::ViewLoad()
    : QQuickImageProvider(QQuickImageProvider::Image)
{

}

QImage ViewLoad::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QString tempPath = QUrl(id).toLocalFile();
    QImage Img;
    QString error;
    if (tempPath == m_currentPath) {
        if (m_Img.size() != requestedSize && requestedSize.width() > 0 && requestedSize.height() > 0) {
            m_Img = m_Img.scaled(requestedSize);
        }
        return m_Img;
    }
    LibUnionImage_NameSpace::loadStaticImageFromFile(tempPath, Img, error);
    m_imgSizes[tempPath] = Img.size() ;
    m_Img = Img;
    m_currentPath = tempPath;
    if (m_Img.size() != requestedSize && requestedSize.width() > 0 && requestedSize.height() > 0) {
        Img = m_Img.scaled(requestedSize);
    }
    return Img;
}

QPixmap ViewLoad::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    QString tempPath = QUrl(id).toLocalFile();
    QImage Img;
    QString error;
    if (tempPath == m_currentPath) {
        return QPixmap::fromImage(m_Img);
    }
    LibUnionImage_NameSpace::loadStaticImageFromFile(tempPath, Img, error);
    m_imgSizes[tempPath] = Img.size();
    m_Img = Img;
    m_currentPath = tempPath;
    return QPixmap::fromImage(Img);
}

int ViewLoad::getImageWidth(const QString &path)
{
    QString tempPath = QUrl(path).toLocalFile();
    return m_imgSizes[tempPath].width();
}

int ViewLoad::getImageHeight(const QString &path)
{
    QString tempPath = QUrl(path).toLocalFile();
    return m_imgSizes[tempPath].height();
}

double ViewLoad::getFitWindowScale(const QString &path, double WindowWidth, double WindowHeight)
{
    double scale = 0.0;
    double width = getImageHeight(path);
    double height = getImageWidth(path);
    double scaleWidth = width / WindowWidth;
    double scaleHeight = height / WindowHeight;

    if (scaleWidth > scaleHeight) {
        scale = scaleWidth;
    } else {
        scale = scaleHeight;
    }

    return scale;
}

ImagePublisher::ImagePublisher(QObject *parent)
    : QObject(parent)
    , QQuickImageProvider(Image)
{
    m_loadMode = 0;
}

//切换加载策略
void ImagePublisher::switchLoadMode()
{
    switch (m_loadMode) {
    case 0:
        m_loadMode = 1;
        break;
    case 1:
        m_loadMode = 0;
        break;
    default:
        m_loadMode = 0;
        break;
    }
}

//将图片裁剪为方图，逻辑与原来一样
QImage ImagePublisher::clipToRect(const QImage &src)
{
    auto tImg = src;

    if (!tImg.isNull() && 0 != tImg.height() && 0 != tImg.width() && (tImg.height() / tImg.width()) < 10 && (tImg.width() / tImg.height()) < 10) {
        bool cache_exist = false;
        if (tImg.height() != 200 && tImg.width() != 200) {
            if (tImg.height() >= tImg.width()) {
                cache_exist = true;
                tImg = tImg.scaledToWidth(200,  Qt::FastTransformation);
            } else if (tImg.height() <= tImg.width()) {
                cache_exist = true;
                tImg = tImg.scaledToHeight(200,  Qt::FastTransformation);
            }
        }
        if (!cache_exist) {
            if ((static_cast<float>(tImg.height()) / (static_cast<float>(tImg.width()))) > 3) {
                tImg = tImg.scaledToWidth(200,  Qt::FastTransformation);
            } else {
                tImg = tImg.scaledToHeight(200,  Qt::FastTransformation);
            }
        }
    }

    if (!tImg.isNull()) {
        int width = tImg.width();
        int height = tImg.height();
        if (abs((width - height) * 10 / width) >= 1) {
            QRect rect = tImg.rect();
            int x = rect.x() + width / 2;
            int y = rect.y() + height / 2;
            if (width > height) {
                x = x - height / 2;
                y = 0;
                tImg = tImg.copy(x, y, height, height);
            } else {
                y = y - width / 2;
                x = 0;
                tImg = tImg.copy(x, y, width, width);
            }
        }
    }

    return tImg;
}

//将图片缩小并加透明pad，最终将呈现为原始尺寸模式（从图片分类的部署代码里搬运）
QImage ImagePublisher::addPadAndScaled(const QImage &src)
{
    auto result = src.convertToFormat(QImage::Format_RGBA8888);

    QImage temp(200, 200, result.format());
    temp.fill(0);
    int x = 0;
    int y = 0;
    if (result.height() > result.width()) {
        result = result.scaledToHeight(200, Qt::SmoothTransformation);
        x = (200 - result.width()) / 2;
    } else {
        result = result.scaledToWidth(200, Qt::SmoothTransformation);
        y = (200 - result.height()) / 2;
    }
    QPainter painter;
    painter.begin(&temp);
    painter.drawImage(x, y, result);
    painter.end();
    result = temp;

    return result;
}

//图片请求类
//警告：这个函数将会被多线程执行，需要确保它是可重入的
QImage ImagePublisher::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    //id的前几个字符是强制刷新用的，需要排除出去
    auto startIndex = id.indexOf('_') + 1;

    QUrl url(id.mid(startIndex));

    QString error;
    QImage image;
    LibUnionImage_NameSpace::loadStaticImageFromFile(url.toLocalFile(), image, error);

    if (m_loadMode == 0) {
        image = clipToRect(image);
    } else { //m_loadMode == 1
        image = addPadAndScaled(image);
    }

    if (size != nullptr) {
        *size = image.size();
    }
    if (requestedSize.width() > 0 && requestedSize.height() > 0) {
        return image.scaled(requestedSize, Qt::KeepAspectRatio);
    } else {
        return image;
    }
}
