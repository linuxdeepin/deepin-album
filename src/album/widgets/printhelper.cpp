#include "printhelper.h"
//#include "printoptionspage.h"
#include "utils/unionimage.h"

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

#include <dprintpreviewdialog.h>
#include <dprintpreviewwidget.h>
DWIDGET_USE_NAMESPACE

PrintHelper::PrintHelper(QObject *parent)
    : QObject(parent)
{

}

//static QAction *hookToolBarActionIcons(QToolBar *bar, QAction **pageSetupAction = nullptr)
//{
//    QAction *last_action = nullptr;

//    for (QAction *action : bar->actions()) {
//        const QString &text = action->text();

//        if (text.isEmpty())
//            continue;

//        // 防止被lupdate扫描出来
//        const char *context = "QPrintPreviewDialog";
//        const char *print = "Print";

//        const QMap<QString, QString> map {
//            {QCoreApplication::translate(context, "Next page"), QStringLiteral("go-next")},
//            {QCoreApplication::translate(context, "Previous page"), QStringLiteral("go-previous")},
//            {QCoreApplication::translate(context, "First page"), QStringLiteral("go-first")},
//            {QCoreApplication::translate(context, "Last page"), QStringLiteral("go-last")},
//            {QCoreApplication::translate(context, "Fit width"), QStringLiteral("fit-width")},
//            {QCoreApplication::translate(context, "Fit page"), QStringLiteral("fit-page")},
//            {QCoreApplication::translate(context, "Zoom in"), QStringLiteral("zoom-in")},
//            {QCoreApplication::translate(context, "Zoom out"), QStringLiteral("zoom-out")},
//            {QCoreApplication::translate(context, "Portrait"), QStringLiteral("layout-portrait")},
//            {QCoreApplication::translate(context, "Landscape"), QStringLiteral("layout-landscape")},
//            {QCoreApplication::translate(context, "Show single page"), QStringLiteral("view-page-one")},
//            {QCoreApplication::translate(context, "Show facing pages"), QStringLiteral("view-page-sided")},
//            {QCoreApplication::translate(context, "Show overview of all pages"), QStringLiteral("view-page-multi")},
//            {QCoreApplication::translate(context, print), QStringLiteral("print")},
//            {QCoreApplication::translate(context, "Page setup"), QStringLiteral("page-setup")}
//        };


//        const QString &icon_name = map.value(action->text());

//        if (icon_name.isEmpty())
//            continue;

//        if (pageSetupAction && icon_name == "page-setup") {
//            *pageSetupAction = action;
//        }

//        QIcon icon(QStringLiteral(":/qt-project.org/dialogs/resources/images/qprintpreviewdialog/images/%1-24.svg").arg(icon_name));
//        action->setIcon(icon);
//        last_action = action;
//    }

//    return last_action;
//}

void PrintHelper::showPrintDialog(const QStringList &paths, QWidget *parent)
{
    QList<QImage> imgs;
    QImage imgTemp;
    for (const QString &path : paths) {
        QString errMsg;
        UnionImage_NameSpace::loadStaticImageFromFile(path, imgTemp, errMsg);
        if (!imgTemp.isNull()) {
            imgs << imgTemp;
        }
    }
    //适配打印接口2.0，dtk大于 5.4.4 版才合入最新的2.0打印控件接口
#if (DTK_VERSION_MAJOR > 5 \
    || (DTK_VERSION_MAJOR >=5 && DTK_VERSION_MINOR > 4) \
    || (DTK_VERSION_MAJOR >= 5 && DTK_VERSION_MINOR >= 4 && DTK_VERSION_PATCH > 4))//5.4.4暂时没有合入
    DPrintPreviewDialog printDialog2(nullptr);
    bool suc = printDialog2.setAsynPreview(imgs.size());//设置总页数，异步方式
    if (suc) {
        //异步
        QObject::connect(&printDialog2, &DPrintPreviewDialog::paintRequested, parent, [ = ](DPrinter * _printer, const QVector<int> &pageRange) {
            QPainter painter(_printer);
            for (int i = 0; i < pageRange.size(); i++) {
                QImage img = imgs.at(pageRange.at(i) - 1);
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
                if (i < pageRange.size() - 1) {
                    _printer->newPage();
                }
            }
            painter.end();
        });
    } else {
        //同步
        QObject::connect(&printDialog2, &DPrintPreviewDialog::paintRequested, parent, [ = ](DPrinter * _printer) {
            QPainter painter(_printer);
            for (QImage img : imgs) {
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
                if (img != imgs.last()) {
                    _printer->newPage();
                }
            }
            painter.end();
        });
    }
#else
    DPrintPreviewDialog printDialog2(nullptr);
    QObject::connect(&printDialog2, &DPrintPreviewDialog::paintRequested, parent, [ = ](DPrinter * _printer) {
        QPainter painter(_printer);
        for (QImage img : imgs) {
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
            if (img != imgs.last()) {
                _printer->newPage();
                qDebug() << "painter newPage!    File:" << __FILE__ << "    Line:" << __LINE__;
            }
        }
        painter.end();
    });
#endif
    printDialog2.exec();
}

//QSize PrintHelper::adjustSize(PrintOptionsPage *optionsPage, QImage img, int resolution, const QSize &viewportSize)
//{
//    PrintOptionsPage::ScaleMode scaleMode = optionsPage->scaleMode();
//    QSize size(img.size());

//    if (scaleMode == PrintOptionsPage::ScaleToPage) {
//        size.scale(viewportSize, Qt::KeepAspectRatio);

//    } else if (scaleMode == PrintOptionsPage::ScaleToExpanding) {
//        //  size.scale(viewportSize, Qt::KeepAspectRatioByExpanding);
//    } else if (scaleMode == PrintOptionsPage::ScaleToCustomSize) {
//        double imageWidth = optionsPage->scaleWidth();
//        double imageHeight = optionsPage->scaleHeight();
//        size.setWidth(int(imageWidth * resolution));
//        size.setHeight(int(imageHeight * resolution));
//    } else {
//        const double inchesPerMeter = 100.0 / 2.54;
//        int dpmX = img.dotsPerMeterX() / size.width();
//        int dpmY = img.dotsPerMeterY() / size.height();
//        if (dpmX > 0 && dpmY > 0) {
//            double wImg = double(size.width()) / double(img.dotsPerMeterX()) * inchesPerMeter;
//            double hImg = double(size.height()) / double(img.dotsPerMeterY()) * inchesPerMeter;
//            size.setWidth(int(wImg * resolution));
//            size.setHeight(int(hImg * resolution));
//        } else {
//            //some image dotspermaters is less than normal
//            size.scale(viewportSize, Qt::KeepAspectRatio);
//        }
//    }

//    return size;
//}

//QPoint PrintHelper::adjustPosition(PrintOptionsPage *optionsPage, const QSize &imageSize, const QSize &viewportSize)
//{
//    Qt::Alignment alignment = optionsPage->alignment();
//    int posX = 0;
//    int posY = 0;

//    if (alignment & Qt::AlignLeft) {
//        posX = 0;
//    } else if (alignment & Qt::AlignHCenter) {
//        posX = (viewportSize.width() - imageSize.width()) / 2 < 0 ? 0 : (viewportSize.width() - imageSize.width()) / 2;
//    } else {
//        posX = (viewportSize.width() - imageSize.width()) ;
//    }

//    if (alignment & Qt::AlignTop) {
//        posY = 0;
//    } else if (alignment & Qt::AlignVCenter) {
//        posY = (viewportSize.height() - imageSize.height()) / 2 < 0 ? 0 : (viewportSize.height() - imageSize.height()) / 2 ;
//    } else {
//        posY = (viewportSize.height() - imageSize.height()) ;
//    }

//    return QPoint(posX, posY);
//}
