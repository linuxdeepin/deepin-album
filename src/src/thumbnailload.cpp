#include "thumbnailload.h"
#include "unionimage/unionimage.h"
#include "configsetter.h"
#include "imageengine/movieservice.h"
#include "dbmanager/dbmanager.h"
#include <QPainter>

const QString SETTINGS_GROUP = "Thumbnail";
const QString SETTINGS_DISPLAY_MODE = "ThumbnailMode";

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
    m_collectionPublisher = new CollectionPublisher();
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
    //初始化的时候读取上次退出时的状态
    m_loadMode = LibConfigSetter::instance()->value(SETTINGS_GROUP, SETTINGS_DISPLAY_MODE, 0).toInt();
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

    //切完以后保存状态
    LibConfigSetter::instance()->setValue(SETTINGS_GROUP, SETTINGS_DISPLAY_MODE, m_loadMode.load());
}

int ImagePublisher::getLoadMode()
{
    return m_loadMode;
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

//将图片按比例缩小
QImage ImagePublisher::addPadAndScaled(const QImage &src)
{
    auto result = src.convertToFormat(QImage::Format_RGBA8888);

    if (result.height() > result.width()) {
        result = result.scaledToHeight(200, Qt::SmoothTransformation);
    } else {
        result = result.scaledToWidth(200, Qt::SmoothTransformation);
    }

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

    //如果是视频，则采用视频加载
    if( LibUnionImage_NameSpace::isVideo(url.toLocalFile()) ){
        image = MovieService::instance()->getMovieCover(url);
    }
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

CollectionPublisher::CollectionPublisher()
    : QQuickImageProvider(Image)
{
}

//id: random_Y_2022_0 random_M_2022_6
QImage CollectionPublisher::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    auto tokens = id.split("_");

    QImage result;

    if(id.size() < 4) {
        return result;
    }

    auto type = tokens[1];
    if(type == "Y") {
        result = createYearImage(tokens[2]);
    } else if(type == "M") {
        result = createMonthImage(tokens[2], tokens[3]);
    }

    if (size != nullptr) {
        *size = result.size();
    }
    if (requestedSize.width() > 0 && requestedSize.height() > 0) {
        return result.scaled(requestedSize, Qt::KeepAspectRatio);
    } else {
        return result;
    }
}

QImage CollectionPublisher::createYearImage(const QString &year)
{
    auto paths = DBManager::instance()->getYearPaths(year, 1);
    if(paths.isEmpty()) {
        return QImage();
    }
    auto picPath = paths.at(0);

    //TODO: 异常处理：裂图问题

    //加载原图
    QImage image;
    QString error;
    LibUnionImage_NameSpace::loadStaticImageFromFile(picPath, image, error);
    image.scaled(outputWidth, outputHeight, Qt::KeepAspectRatioByExpanding);

    return image;
}

QImage CollectionPublisher::createMonthImage(const QString &year, const QString &month)
{
    auto paths = DBManager::instance()->getMonthPaths(year, month, 6);
    if(paths.isEmpty()) {
        return QImage();
    }

    //1.加载原图
    std::vector<QImage> images;
    std::transform(paths.begin(), paths.end(), std::back_inserter(images), [](const QString &path){
        QImage image;
        QString error;
        LibUnionImage_NameSpace::loadStaticImageFromFile(path, image, error);
        image.scaled(outputWidth, outputHeight, Qt::KeepAspectRatioByExpanding);
        return image;
    });

    //2.拼接图片
    QImage result;
    switch (images.size()) {
    case 1:
        result = images[0];
        break;
    case 2:
        result = createMonth_2(images);
        break;
    case 3:
        result = createMonth_3(images);
        break;
    case 4:
        result = createMonth_4(images);
        break;
    case 5:
        result = createMonth_5(images);
        break;
    case 6:
        result = createMonth_6(images);
        break;
    default:
        result = images[0];
        break;
    }

    //3.返回图片
    return result;
}

QImage CollectionPublisher::createMonth_2(const std::vector<QImage> &images)
{
    //左右摆放

    //初始化画布
    QImage result(outputWidth, outputHeight, QImage::Format_RGB888);
    result.fill(Qt::white);

    //初始化原始图片
    QImage images_0 = images[0].scaled(outputWidth / 2, outputHeight, Qt::KeepAspectRatioByExpanding);
    QImage images_1 = images[1].scaled(outputWidth / 2, outputHeight, Qt::KeepAspectRatioByExpanding);

    //绘制
    QPainter painter;
    painter.begin(&result);
    painter.drawImage(0, 0, images_0);
    painter.drawImage(outputWidth / 2, 0, images_1);
    painter.end();

    return result;
}

QImage CollectionPublisher::createMonth_3(const std::vector<QImage> &images)
{
    //左1右2

    //初始化画布
    QImage result(outputWidth, outputHeight, QImage::Format_RGB888);
    result.fill(Qt::white);

    //初始化原始图片
    QImage images_0 = images[0].scaled(outputWidth / 2, outputHeight, Qt::KeepAspectRatioByExpanding);
    QImage images_1 = images[1].scaled(outputWidth / 2, outputHeight / 2, Qt::KeepAspectRatioByExpanding);
    QImage images_2 = images[2].scaled(outputWidth / 2, outputHeight / 2, Qt::KeepAspectRatioByExpanding);

    //绘制
    QPainter painter;
    painter.begin(&result);
    painter.drawImage(0, 0, images_0);
    painter.drawImage(outputWidth / 2, 0, images_1);
    painter.drawImage(outputWidth / 2, outputHeight / 2, images_2);
    painter.end();

    return result;
}

QImage CollectionPublisher::createMonth_4(const std::vector<QImage> &images)
{
    //左2右2

    //初始化画布
    QImage result(outputWidth, outputHeight, QImage::Format_RGB888);
    result.fill(Qt::white);

    //初始化原始图片
    QImage images_0 = images[0].scaled(outputWidth / 2, outputHeight / 2, Qt::KeepAspectRatioByExpanding);
    QImage images_1 = images[1].scaled(outputWidth / 2, outputHeight / 2, Qt::KeepAspectRatioByExpanding);
    QImage images_2 = images[2].scaled(outputWidth / 2, outputHeight / 2, Qt::KeepAspectRatioByExpanding);
    QImage images_3 = images[3].scaled(outputWidth / 2, outputHeight / 2, Qt::KeepAspectRatioByExpanding);

    //绘制
    QPainter painter;
    painter.begin(&result);
    painter.drawImage(0, 0, images_0);
    painter.drawImage(outputWidth / 2, 0, images_1);
    painter.drawImage(0, outputHeight / 2, images_2);
    painter.drawImage(outputWidth / 2, outputHeight / 2, images_3);
    painter.end();

    return result;
}

QImage CollectionPublisher::createMonth_5(const std::vector<QImage> &images)
{
    //上1下4

    //初始化画布
    QImage result(outputWidth, outputHeight, QImage::Format_RGB888);
    result.fill(Qt::white);
    constexpr int splitPos = static_cast<int>(outputHeight * 0.618);
    constexpr int splitPos_2 = static_cast<int>(outputHeight * (1 - 0.618));

    //初始化原始图片
    QImage images_0 = images[0].scaled(outputWidth, splitPos, Qt::KeepAspectRatioByExpanding);
    QImage images_1 = images[1].scaled(outputWidth / 4, splitPos_2, Qt::KeepAspectRatioByExpanding);
    QImage images_2 = images[2].scaled(outputWidth / 4, splitPos_2, Qt::KeepAspectRatioByExpanding);
    QImage images_3 = images[3].scaled(outputWidth / 4, splitPos_2, Qt::KeepAspectRatioByExpanding);
    QImage images_4 = images[4].scaled(outputWidth / 4, splitPos_2, Qt::KeepAspectRatioByExpanding);

    //绘制
    QPainter painter;
    painter.begin(&result);
    painter.drawImage(0, 0, images_0);
    painter.drawImage(0, splitPos, images_1);
    painter.drawImage(outputWidth / 4 * 1, splitPos, images_2);
    painter.drawImage(outputWidth / 4 * 2, splitPos, images_3);
    painter.drawImage(outputWidth / 4 * 3, splitPos, images_4);
    painter.end();

    return result;
}

QImage CollectionPublisher::createMonth_6(const std::vector<QImage> &images)
{
    //上1下5

    //初始化画布
    QImage result(outputWidth, outputHeight, QImage::Format_RGB888);
    result.fill(Qt::white);
    constexpr int splitPos = static_cast<int>(outputHeight * 0.618);
    constexpr int splitPos_2 = static_cast<int>(outputHeight * (1 - 0.618));

    //初始化原始图片
    QImage images_0 = images[0].scaled(outputWidth, splitPos, Qt::KeepAspectRatioByExpanding);
    QImage images_1 = images[1].scaled(outputWidth / 5, splitPos_2, Qt::KeepAspectRatioByExpanding);
    QImage images_2 = images[2].scaled(outputWidth / 5, splitPos_2, Qt::KeepAspectRatioByExpanding);
    QImage images_3 = images[3].scaled(outputWidth / 5, splitPos_2, Qt::KeepAspectRatioByExpanding);
    QImage images_4 = images[4].scaled(outputWidth / 5, splitPos_2, Qt::KeepAspectRatioByExpanding);
    QImage images_5 = images[5].scaled(outputWidth / 5, splitPos_2, Qt::KeepAspectRatioByExpanding);

    //绘制
    QPainter painter;
    painter.begin(&result);
    painter.drawImage(0, 0, images_0);
    painter.drawImage(0, splitPos, images_1);
    painter.drawImage(outputWidth / 5 * 1, splitPos, images_2);
    painter.drawImage(outputWidth / 5 * 2, splitPos, images_3);
    painter.drawImage(outputWidth / 5 * 3, splitPos, images_4);
    painter.drawImage(outputWidth / 5 * 4, splitPos, images_5);
    painter.end();

    return result;
}
