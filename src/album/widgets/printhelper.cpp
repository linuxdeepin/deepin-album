/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     ZhangYong <zhangyong@uniontech.com>
 *
 * Maintainer: ZhangYong <ZhangYong@uniontech.com>
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
#include "printhelper.h"
#include "utils/unionimage.h"

#include <DPrintPreviewDialog>
#include <DApplication>

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
    m_re = new RequestedSlot(this);
}

PrintHelper::~PrintHelper()
{
    deconstruction();
}

void PrintHelper::showPrintDialog(const QStringList &paths, QWidget *parent)
{
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
    //增加运行时版本判断,需要指定父指针,wayland下才能正常使用
    DPrintPreviewDialog printDialog2(parent);
    if (DApplication::runtimeDtkVersion() >= DTK_VERSION_CHECK(5, 4, 10, 0)) {
        bool suc = printDialog2.setAsynPreview(m_re->m_imgs.size());//设置总页数，异步方式
        //单张照片设置名称,可能多选照片，但能成功加载的可能只有一张，或从相册中选中的原图片不存在
        if (tempExsitPaths.size() == 1) {
            // 提供包含后缀的文件全名，由打印模块自己处理后缀
            QString docName = QString(QFileInfo(tempExsitPaths.at(0)).completeBaseName());
            docName += ".pdf";
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

void PrintHelper::deconstruction()
{
    delete m_re;
    m_re = nullptr;
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
    //更新逻辑，需要nepage和现实所有的，因为需要多版打印,需要显示多张图片
    QPainter painter(_printer);
    int index = 0;
    for (int page : pageRange) {
        if ((page < m_imgs.count() + 1) && page >= 1) {
            QImage img = m_imgs.at(page - 1);
            if (!img.isNull()) {
                painter.setRenderHint(QPainter::Antialiasing);
                painter.setRenderHint(QPainter::SmoothPixmapTransform);
                QRect wRect  = _printer->pageRect();

                qreal ratio = wRect.width() * 1.0 / img.width();

                painter.drawImage(QRectF(0, qreal(wRect.height() - img.height() * ratio) / 2,
                                         wRect.width(), img.height() * ratio), img);
            }
            if (index < pageRange.size() - 1) {
                _printer->newPage();
                index++;
            }
        }
    }
}

void RequestedSlot::paintRequestSync(DPrinter *_printer)
{
    QPainter painter(_printer);
    for (QImage img : m_imgs) {
        if (!img.isNull()) {
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            QRect wRect  = _printer->pageRect();

            qreal ratio = wRect.width() * 1.0 / img.width();

            painter.drawImage(QRectF(0, qreal(wRect.height() - img.height() * ratio) / 2,
                                     wRect.width(), img.height() * ratio), img);
        }
        if (img != m_imgs.last()) {
            _printer->newPage();
        }
    }
    painter.end();
}

#include "printhelper.moc"
