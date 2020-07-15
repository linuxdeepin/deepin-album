#include "printhelper.h"
#include "printoptionspage.h"
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

PrintHelper::PrintHelper(QObject *parent)
    : QObject(parent)
{

}

static QAction *hookToolBarActionIcons(QToolBar *bar, QAction **pageSetupAction = nullptr)
{
    QAction *last_action = nullptr;

    for (QAction *action : bar->actions()) {
        const QString &text = action->text();

        if (text.isEmpty())
            continue;

        // 防止被lupdate扫描出来
        const char *context = "QPrintPreviewDialog";
        const char *print = "Print";

        const QMap<QString, QString> map {
            {QCoreApplication::translate(context, "Next page"), QStringLiteral("go-next")},
            {QCoreApplication::translate(context, "Previous page"), QStringLiteral("go-previous")},
            {QCoreApplication::translate(context, "First page"), QStringLiteral("go-first")},
            {QCoreApplication::translate(context, "Last page"), QStringLiteral("go-last")},
            {QCoreApplication::translate(context, "Fit width"), QStringLiteral("fit-width")},
            {QCoreApplication::translate(context, "Fit page"), QStringLiteral("fit-page")},
            {QCoreApplication::translate(context, "Zoom in"), QStringLiteral("zoom-in")},
            {QCoreApplication::translate(context, "Zoom out"), QStringLiteral("zoom-out")},
            {QCoreApplication::translate(context, "Portrait"), QStringLiteral("layout-portrait")},
            {QCoreApplication::translate(context, "Landscape"), QStringLiteral("layout-landscape")},
            {QCoreApplication::translate(context, "Show single page"), QStringLiteral("view-page-one")},
            {QCoreApplication::translate(context, "Show facing pages"), QStringLiteral("view-page-sided")},
            {QCoreApplication::translate(context, "Show overview of all pages"), QStringLiteral("view-page-multi")},
            {QCoreApplication::translate(context, print), QStringLiteral("print")},
            {QCoreApplication::translate(context, "Page setup"), QStringLiteral("page-setup")}
        };


        const QString &icon_name = map.value(action->text());

        if (icon_name.isEmpty())
            continue;

        if (pageSetupAction && icon_name == "page-setup") {
            *pageSetupAction = action;
        }

        QIcon icon(QStringLiteral(":/qt-project.org/dialogs/resources/images/qprintpreviewdialog/images/%1-24.svg").arg(icon_name));
        action->setIcon(icon);
        last_action = action;
    }

    return last_action;
}

void PrintHelper::showPrintDialog(const QStringList &paths, QWidget *parent)
{
    QPrinter printer;
    QImage img;

    printer.setColorMode(QPrinter::Color);

    QPrintPreviewDialog printDialog(&printer, parent);
    PrintOptionsPage *optionsPage = new PrintOptionsPage(&printDialog);
//    printDialog.resize(800, 800);

    QToolBar *toolBar = printDialog.findChild<QToolBar *>();
    if (toolBar) {
        QAction *page_setup_action = nullptr;
        QAction *last_action = hookToolBarActionIcons(toolBar, &page_setup_action);
        QAction *action = new QAction(QIcon(":/qt-project.org/dialogs/resources/images/qprintpreviewdialog/images/preview-24.svg"),
                                      QCoreApplication::translate("PrintPreviewDialog", "Image Settings"), toolBar);
        connect(action, &QAction::triggered, optionsPage, &PrintOptionsPage::show);
        toolBar->insertAction(last_action, action);

        // 使用QPrintPropertiesDialog代替QPageSetupDialog, 用于解决使用QPageSetupDialog进行打印设置无效的问题
        if (page_setup_action) {
            // 先和原有的槽断开连接
            disconnect(page_setup_action, &QAction::triggered, nullptr, nullptr);
            // 触发创建QPrintDialog对象
            connect(page_setup_action, SIGNAL(triggered(bool)), &printDialog, SLOT(_q_print()), Qt::QueuedConnection);
            // 在QPrintDialog对象被创建后调用，用于触发显示 QPrintPropertiesDialog
            connect(page_setup_action, &QAction::triggered, &printDialog, [&printDialog] {
                auto find_child_by_name = [](const QObject * obj, const QByteArray & class_name)
                {
                    for (QObject *child : obj->children()) {
                        if (child->metaObject()->className() == class_name) {
                            return child;
                        }
                    }

                    return static_cast<QObject *>(nullptr);
                };

                if (QPrintDialog *print_dialog = printDialog.findChild<QPrintDialog *>())
                {
                    print_dialog->reject();

                    // 显示打印设置对话框
                    if (QObject *print_properties_dialog = find_child_by_name(print_dialog, "QUnixPrintWidget"))
                        QMetaObject::invokeMethod(print_properties_dialog, "_q_btnPropertiesClicked");
                }
            }, Qt::QueuedConnection);
        }
    } else {
        optionsPage->hide();
    }

    // HACK: Qt的打印设置有点bug，属性对话框中手动设置了纸张方向为横向（默认纵向）其实并不生效，
    //（猜测是透过cups协商出了问题，跟踪src/printsupport里面的代码没有问题，
    // 应该在src/plugins/printsupport中出的问题），
    // 如果在构造QPainter对象之前给QPrinter设置为横向，则实际可以横向打印，
    // 但是这时候手动选择纵向又不会生效。
    // 所以这里的hack是事先判断图像是“横向”还是“纵向”的，给QPrinter设置默认的纸张方向，
    // 以满足大部分图片打印的需求。
    QList<QImage> imgs;

    for (const QString &path : paths) {
        // There're cases that people somehow changed the image file suffixes, like jpg -> png,
        // we'd better detect that before printing, otherwise we get an empty print.
        const QString format = UnionImage_NameSpace::detectImageFormat(path);
        if (!img.load(path, format.toLatin1())) {
            qDebug() << "img load failed" << path;
            continue;
        }

        imgs << img;
    }

    if (!imgs.isEmpty()) {
        QImage img1 = imgs.first();
        qDebug() << img1.width() << img1.height();
        if (!img1.isNull() && img1.width() > img1.height()) {
            printer.setPageOrientation(QPageLayout::Landscape);
        }
    }
    // HACK - end

    auto repaint = [&imgs, &optionsPage, &printer] {
        QPainter painter(&printer);

        int index = 0;
        QRect rect = painter.viewport();//it belongs previous rect of image
        for (QImage img : imgs)
        {
            QSize size = PrintHelper::adjustSize(optionsPage, img, printer.resolution(), rect.size());
            QPoint pos(0, 0);

            //LMH0515优化打印
            if (optionsPage->scaleMode() == PrintOptionsPage::ScaleToPage) {
                painter.setWindow(rect);
                pos = PrintHelper::adjustPosition(optionsPage, size, rect.size());
            } else if (optionsPage->scaleMode() == PrintOptionsPage::ScaleToExpanding) {
                painter.setWindow(img.rect());
            } else {
                painter.setWindow(img.rect());
            }
            img = img.scaledToWidth(size.width(), Qt::FastTransformation);
            img = img.scaledToHeight(size.height(), Qt::FastTransformation);

            int x2 = painter.window().right();
            int y2 = painter.window().bottom();
            painter.drawImage(pos.x(), pos.y(), img, 0, 0, x2, y2);
//            if (size.width() <= img.width() || size.height() <= img.height()) {
//                img = img.scaledToWidth(size.width(), Qt::FastTransformation);
//                img = img.scaledToHeight(size.height(), Qt::FastTransformation);

//                int x1 = painter.window().left();
//                int y1 = painter.window().top();
//                int x2 = painter.window().right();
//                int y2 = painter.window().bottom();
//                painter.drawImage(0, 0, img, 0, 0, x2, y2);

//            } else {
//                painter.setRenderHint(QPainter::SmoothPixmapTransform);
////                if (optionsPage->scaleMode() != PrintOptionsPage::ScaleToExpanding) {
////                    painter.setViewport(qAbs(pos.x()), qAbs(pos.y()), img.width(), img.height());
////                }
//                int x1 = img.rect().left();
//                int y1 = img.rect().top();
//                int x2 = img.rect().right();
//                int y2 = img.rect().bottom();
//                painter.drawImage(0, 0, img, 0, 0, x2, y2);
//            }
            if (++index != imgs.size()) {
                printer.newPage();
            }
            //painter.restore();
        }

        painter.end();
    };
    QObject::connect(&printDialog, &QPrintPreviewDialog::paintRequested, &printDialog, repaint);
    QObject::connect(optionsPage, &PrintOptionsPage::valueChanged, optionsPage, [&printDialog] {
        if (QPrintPreviewWidget *pw = printDialog.findChild<QPrintPreviewWidget *>())
            pw->updatePreview();
    });

    if (printDialog.exec() == QDialog::Accepted) {

        qDebug() << "print succeed!";

        return;
    }

//    QObject::connect(printDialog, &QPrintPreviewDialog::done, printDialog,
//                     &QPrintPreviewDialog::deleteLater);

    qDebug() << "print failed!";
}

QSize PrintHelper::adjustSize(PrintOptionsPage *optionsPage, QImage img, int resolution, const QSize &viewportSize)
{
    PrintOptionsPage::ScaleMode scaleMode = optionsPage->scaleMode();
    QSize size(img.size());

    if (scaleMode == PrintOptionsPage::ScaleToPage) {
        size.scale(viewportSize, Qt::KeepAspectRatio);

    } else if (scaleMode == PrintOptionsPage::ScaleToExpanding) {
        //  size.scale(viewportSize, Qt::KeepAspectRatioByExpanding);
    } else if (scaleMode == PrintOptionsPage::ScaleToCustomSize) {
        double imageWidth = optionsPage->scaleWidth();
        double imageHeight = optionsPage->scaleHeight();
        size.setWidth(int(imageWidth * resolution));
        size.setHeight(int(imageHeight * resolution));
    } else {
        const double inchesPerMeter = 100.0 / 2.54;
        int dpmX = img.dotsPerMeterX() / size.width();
        int dpmY = img.dotsPerMeterY() / size.height();
        if (dpmX > 0 && dpmY > 0) {
            double wImg = double(size.width()) / double(img.dotsPerMeterX()) * inchesPerMeter;
            double hImg = double(size.height()) / double(img.dotsPerMeterY()) * inchesPerMeter;
            size.setWidth(int(wImg * resolution));
            size.setHeight(int(hImg * resolution));
        } else {
            //some image dotspermaters is less than normal
            size.scale(viewportSize, Qt::KeepAspectRatio);
        }
    }

    return size;
}

QPoint PrintHelper::adjustPosition(PrintOptionsPage *optionsPage, const QSize &imageSize, const QSize &viewportSize)
{
    Qt::Alignment alignment = optionsPage->alignment();
    int posX = 0;
    int posY = 0;

    if (alignment & Qt::AlignLeft) {
        posX = 0;
    } else if (alignment & Qt::AlignHCenter) {
        posX = (viewportSize.width() - imageSize.width() ) / 2 < 0 ? 0 : (viewportSize.width() - imageSize.width() ) / 2;
    } else {
        posX = (viewportSize.width() - imageSize.width()) ;
    }

    if (alignment & Qt::AlignTop) {
        posY = 0;
    } else if (alignment & Qt::AlignVCenter) {
        posY = (viewportSize.height() - imageSize.height() ) / 2 < 0 ? 0 : (viewportSize.height() - imageSize.height()) / 2 ;
    } else {
        posY = (viewportSize.height() - imageSize.height()) ;
    }

    return QPoint(posX, posY);
}
