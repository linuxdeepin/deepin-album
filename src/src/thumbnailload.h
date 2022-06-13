#ifndef THUMBNAILLOAD_H
#define THUMBNAILLOAD_H

#include <QQuickImageProvider>
#include <QQuickWindow>
#include <QImage>

//大图预览下的小图
class ThumbnailLoad : public QQuickImageProvider
{
public:
    explicit ThumbnailLoad();
    //获取缩略图
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize);  //预留
    bool imageIsNull(const QString &path);
    //当前图片
    QImage m_Img;
    QMap <QString, QImage> m_imgMap; //缩略图
};

//大图预览的大图
class ViewLoad : public QQuickImageProvider
{
public:
    explicit ViewLoad();
    //获取缩略图
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize);  //预留

    //获得当前图片的宽和高
    int getImageWidth(const QString &path);
    int getImageHeight(const QString &path);
    double getFitWindowScale(const QString &path, double WindowWidth, double WindowHeight);

    QMap <QString, QSize> m_imgSizes; //图片大小
    //当前图片
    QImage m_Img;
    //加载路径
    QString m_currentPath;
};

//缩略图
class ImagePublisher : public QObject, public QQuickImageProvider
{
    Q_OBJECT

public:
    explicit ImagePublisher(QObject *parent = nullptr);

    //切换图片显示状态
    Q_INVOKABLE void switchLoadMode();

    //获得当前图片显示的状态
    Q_INVOKABLE int getLoadMode();

protected:
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    //图片裁剪策略
    QImage clipToRect(const QImage &src);
    QImage addPadAndScaled(const QImage &src);

    //加载模式控制，requestImage是由QML引擎多线程调用，此处需要采用原子锁，防止崩溃
    std::atomic_int m_loadMode;
};

//聚合图
class CollectionPublisher : public QQuickImageProvider
{
public:
    explicit CollectionPublisher();

protected:
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    //图片裁剪策略
    QImage createYearImage(const QString &year); //生成年视图
    QImage createMonthImage(const QString &year, const QString &month);//生成月视图
};

class LoadImage : public QObject
{
    Q_OBJECT
public:
    explicit LoadImage(QObject *parent = nullptr);

    ThumbnailLoad *m_pThumbnail{nullptr};
    ViewLoad *m_viewLoad{nullptr};
    ImagePublisher *m_publisher{nullptr};
    CollectionPublisher *m_collectionPublisher{nullptr};
    Q_INVOKABLE double getFitWindowScale(const QString &path, double WindowWidth, double WindowHeight);
    Q_INVOKABLE bool imageIsNull(const QString &path);
    //获得当前图片的宽和高
    Q_INVOKABLE int getImageWidth(const QString &path);
    Q_INVOKABLE int getImageHeight(const QString &path);
    //获得宽高比例
    Q_INVOKABLE double getrealWidthHeightRatio(const QString &path);
    //加载路径
    QString m_path;

public slots:
    //加载多张
    void loadThumbnails(const QStringList list);
    //加载一张
    void loadThumbnail(const QString path);
    //缩略图裁切接口-预留
    void catThumbnail(const QStringList &list);

signals:
    //通知QML刷新
    void callQmlRefeshImg();
};

#endif // THUMBNAILLOAD_H
