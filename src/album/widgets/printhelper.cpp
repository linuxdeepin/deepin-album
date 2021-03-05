#include "printhelper.h"
#include "utils/unionimage.h"

#include <DPrintPreviewDialog>

#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrintPreviewWidget>
#include <QPrinter>
#include <QPainter>
#include <QToolBar>
#include <QCoreApplication>
#include <QImageReader>
#include <QDebug>
#include <QVector>
#include <QFileInfo>

DWIDGET_USE_NAMESPACE

//绘制图片处理类
class RequestedSlot : public QObject
{
    Q_OBJECT
public:
    explicit RequestedSlot(QObject *parent = nullptr);
    ~RequestedSlot();
public slots:
    void paintRequestedAsyn(DPrinter *_printer, const QVector<int> &pageRange);
    void paintRequestSync(DPrinter *_printer);

public:
    QStringList m_paths;
    QList<QImage> m_imgs;
};

PrintHelper *PrintHelper::m_Printer = nullptr;

PrintHelper *PrintHelper::getIntance()
{
    if (!m_Printer) {
        m_Printer = new PrintHelper();
    }
    return m_Printer;
}

PrintHelper::PrintHelper(QObject *parent)
    : QObject(parent)
{
    m_re = new RequestedSlot;
}

void PrintHelper::showPrintDialog(const QStringList &paths, QWidget *parent)
{
    Q_UNUSED(parent)
    m_re->m_paths.clear();
    m_re->m_imgs.clear();
    QStringList tempExsitPaths;//保存存在的图片路径

    m_re->m_paths = paths;
    QImage imgTemp;
    //判断图片文件是否存在并加载，耗时巨大
    for (const QString &path : m_re->m_paths) {
        QString errMsg;
        UnionImage_NameSpace::loadStaticImageFromFile(path, imgTemp, errMsg);
        if (!imgTemp.isNull()) {
            m_re->m_imgs << imgTemp;
            tempExsitPaths << path;
        }
    }
    //适配打印接口2.0，dtk大于 5.4.10 版才合入最新的2.0打印控件接口
#if (DTK_VERSION_MAJOR > 5 \
    || (DTK_VERSION_MAJOR >=5 && DTK_VERSION_MINOR > 4) \
    || (DTK_VERSION_MAJOR >= 5 && DTK_VERSION_MINOR >= 4 && DTK_VERSION_PATCH >= 10))//5.4.4暂时没有合入
    //增加运行时版本判断
    DPrintPreviewDialog printDialog2(nullptr);
    if (DTK_VERSION_MAJOR > 5 \
            || (DTK_VERSION_MAJOR >= 5 && DTK_VERSION_MINOR > 4) \
            || (DTK_VERSION_MAJOR >= 5 && DTK_VERSION_MINOR >= 4 && DTK_VERSION_PATCH >= 10)) {
        bool suc = printDialog2.setAsynPreview(m_re->m_imgs.size());//设置总页数，异步方式
        //单张照片设置名称,可能多选照片，但能成功加载的可能只有一张，或从相册中选中的原图片不存在
        if (tempExsitPaths.size() == 1) {
            QString docName = QString(QFileInfo(tempExsitPaths.at(0)).baseName());
            printDialog2.setDocName(docName);
        }//else 多张照片不设置名称，默认使用print模块的print.pdf
        if (suc) {//异步
            connect(&printDialog2, SIGNAL(paintRequested(DPrinter *, const QVector<int> &)),
                    m_re, SLOT(paintRequestedAsyn(DPrinter *, const QVector<int> &)));
        } else {//同步
            connect(&printDialog2, SIGNAL(paintRequested(DPrinter *)),
                    m_re, SLOT(paintRequestSync(DPrinter *)));
        }
    } else {
        connect(&printDialog2, SIGNAL(paintRequested(DPrinter *)),
                m_re, SLOT(paintRequestSync(DPrinter *)));
    }
#else
    DPrintPreviewDialog printDialog2(nullptr);
    connect(&printDialog2, SIGNAL(paintRequested(DPrinter *)),
            m_re, SLOT(paintRequestSync(DPrinter *)));
#endif
    printDialog2.exec();
    m_re->m_paths.clear();
    m_re->m_imgs.clear();
}

RequestedSlot::RequestedSlot(QObject *parent)
{
    Q_UNUSED(parent)
}

RequestedSlot::~RequestedSlot()
{

}

void RequestedSlot::paintRequestedAsyn(DPrinter *_printer, const QVector<int> &pageRange)
{
    QPainter painter(_printer);
    if (pageRange.size() > 0) {
        QImage img = m_imgs.at(pageRange.at(0) - 1);
        if (!img.isNull()) {
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            QRect wRect  = _printer->pageRect();
            QImage tmpMap;
            if (img.width() > wRect.width() || img.height() > wRect.height()) {
                tmpMap = img.scaled(wRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            } else {
                tmpMap = img;
            }
            QRectF drawRectF = QRectF(qreal(wRect.width() - tmpMap.width()) / 2,
                                      qreal(wRect.height() - tmpMap.height()) / 2,
                                      tmpMap.width(), tmpMap.height());
            painter.drawImage(QRectF(drawRectF.x(), drawRectF.y(), tmpMap.width(),
                                     tmpMap.height()), tmpMap);
        }
    }
    painter.end();
}

void RequestedSlot::paintRequestSync(DPrinter *_printer)
{
    QPainter painter(_printer);
    for (QImage img : m_imgs) {
        if (!img.isNull()) {
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            QRect wRect  = _printer->pageRect();
            QImage tmpMap;
            if (img.width() > wRect.width() || img.height() > wRect.height()) {
                tmpMap = img.scaled(wRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            } else {
                tmpMap = img;
            }
            QRectF drawRectF = QRectF(qreal(wRect.width() - tmpMap.width()) / 2,
                                      qreal(wRect.height() - tmpMap.height()) / 2,
                                      tmpMap.width(), tmpMap.height());
            painter.drawImage(QRectF(drawRectF.x(), drawRectF.y(), tmpMap.width(),
                                     tmpMap.height()), tmpMap);
        }
        if (img != m_imgs.last()) {
            _printer->newPage();
        }
    }
    painter.end();
}

#include "printhelper.moc"
