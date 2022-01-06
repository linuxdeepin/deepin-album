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
#include "imginfodialog.h"
#include "controller/signalmanager.h"
#include "utils/imageutils.h"
#include "imageengineapi.h"
#include "utils/unionimage.h"
#include "widgets/formlabel.h"

#include "dfmdarrowlineexpand.h"

#include <QFormLayout>
#include <QLocale>
#include <QApplication>
#include <QScrollBar>

#include <DTitlebar>
#include <DApplicationHelper>
#include <DFontSizeManager>

namespace {

struct MetaData {
    QString key;
    const char *name;
};

static MetaData MetaDataBasics[] = {
    {"FileName",            QT_TRANSLATE_NOOP("MetadataName", "Name")},

    {"DateTimeOriginal",    QT_TRANSLATE_NOOP("MetadataName", "Date captured")},
    {"DateTimeDigitized",   QT_TRANSLATE_NOOP("MetadataName", "Date modified")},
    {"FileFormat",          QT_TRANSLATE_NOOP("MetadataName", "Type")},
    {"Dimensions",          QT_TRANSLATE_NOOP("MetadataName", "Dimensions")},
    {"FileSize",            QT_TRANSLATE_NOOP("MetadataName", "File size")},

    // {"MaxApertureValue",    QT_TRANSLATE_NOOP("MetadataName", "Max aperture")},
    // {"FocalLength",         QT_TRANSLATE_NOOP("MetadataName", "Focal length")},
    {"", ""}
};

static MetaData MetaDataDetails[] = {
    {"ColorSpace",          QT_TRANSLATE_NOOP("MetadataName", "Colorspace")},
    {"ExposureMode",        QT_TRANSLATE_NOOP("MetadataName", "Exposure mode")},
    {"ExposureProgram",     QT_TRANSLATE_NOOP("MetadataName", "Exposure program")},
    {"ExposureTime",        QT_TRANSLATE_NOOP("MetadataName", "Exposure time")},
    {"Flash",               QT_TRANSLATE_NOOP("MetadataName", "Flash")},
    {"ApertureValue",       QT_TRANSLATE_NOOP("MetadataName", "Aperture")},
    {"FocalLength",         QT_TRANSLATE_NOOP("MetadataName", "Focal length")},
    {"ISOSpeedRatings",     QT_TRANSLATE_NOOP("MetadataName", "ISO")},
    {"MaxApertureValue",    QT_TRANSLATE_NOOP("MetadataName", "Max aperture")},
    {"MeteringMode",        QT_TRANSLATE_NOOP("MetadataName", "Metering mode")},
    {"WhiteBalance",        QT_TRANSLATE_NOOP("MetadataName", "White balance")},
    {"FlashExposureComp",   QT_TRANSLATE_NOOP("MetadataName", "Flash compensation")},
    {"Model",               QT_TRANSLATE_NOOP("MetadataName", "Camera model")},
    {"LensType",            QT_TRANSLATE_NOOP("MetadataName", "Lens model")},
    {"", ""}
};

}  // namespace



ImgInfoDialog::ImgInfoDialog(const QString &path, QWidget *parent)
    : DDialog(parent), m_title_maxwidth(0), m_maxFieldWidth(0),
      m_isBaseInfo(false), m_isDetailsInfo(false), m_exif_base(nullptr),
      m_exif_details(nullptr), m_exifLayout_base(nullptr), m_exifLayout_details(nullptr),
      m_mainLayout(nullptr), m_scrollArea(nullptr)
{
    //LMH0424默认字体大小
    QFont font;
    m_currentFontSize = DFontSizeManager::instance()->fontPixelSize(font);
    QLocale locale;
    if (locale.language() == QLocale::Chinese)  //语言为中文
        m_title_maxwidth = 60;
    else
        m_title_maxwidth = 108;

    initUI();
    setImagePath(path);
}

int ImgInfoDialog::height()
{
    return contentHeight() + 10;
}

void ImgInfoDialog::initUI()
{
    //wayland背景透明问题
    DPalette imgInfoDlgPl = this->palette();
    QColor imgInfoDlgColor("#F7F7F7");
    imgInfoDlgColor.setAlphaF(0.8);
    imgInfoDlgPl.setColor(DPalette::Window, imgInfoDlgColor);
    this->setBackgroundRole(DPalette::Window);

    setFixedWidth(320);
    setMaximumHeight(540);
    setContentLayoutContentsMargins(QMargins(0, 0, 0, 0));

    //title
    DLabel *title = new DLabel(this);
    title->setText(tr("Image info"));
    title->setGeometry(this->x() + (this->width() - title->width()) / 2, this->y(), 112, 50);
    title->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    DFontSizeManager::instance()->bind(title, DFontSizeManager::T6);
    DPalette pa = DApplicationHelper::instance()->palette(title);
    pa.setBrush(DPalette::Text, pa.color(DPalette::TextTitle));
    title->setPalette(pa);

    setContentsMargins(0, 0, 0, 0);

    DWidget *content = new DWidget();
    addContent(content);

    m_exif_base = new QFrame(this);
    m_exif_base->setFixedWidth(280);
    m_exif_details = new QFrame(this);
    m_exif_details->setFixedWidth(280);
    m_exifLayout_base = new QFormLayout();
    m_exifLayout_base->setVerticalSpacing(7);
    m_exifLayout_base->setHorizontalSpacing(16);
    m_exifLayout_base->setContentsMargins(10, 1, 7, 10);
    m_exifLayout_base->setLabelAlignment(Qt::AlignLeft);

    m_exifLayout_details = new QFormLayout();

    m_exifLayout_details->setVerticalSpacing(7);
    m_exifLayout_details->setHorizontalSpacing(16);
    m_exifLayout_details->setContentsMargins(10, 1, 7, 10);
    m_exifLayout_details->setLabelAlignment(Qt::AlignLeft);

    m_exif_base->setLayout(m_exifLayout_base);
    m_exif_details->setLayout(m_exifLayout_details);

    m_mainLayout = new QVBoxLayout;

    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setMargin(0);
    m_mainLayout->setSpacing(0);

    m_scrollArea = new DScrollArea();
    DPalette palette = m_scrollArea->viewport()->palette();
    palette.setBrush(DPalette::Background, Qt::NoBrush);
    m_scrollArea->viewport()->setPalette(palette);
    m_scrollArea->setFrameShape(QFrame::Shape::NoFrame);
    QFrame *infoframe = new QFrame;
    QVBoxLayout *scrollWidgetLayout = new QVBoxLayout;
    scrollWidgetLayout->setContentsMargins(10, 0, 10, 0);
    scrollWidgetLayout->setSpacing(10);
    infoframe->setLayout(scrollWidgetLayout);
    m_scrollArea->setWidget(infoframe);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    m_scrollArea->viewport()->installEventFilter(this);

    QVBoxLayout *scrolllayout = new QVBoxLayout;
    scrolllayout->addWidget(m_scrollArea);
    m_mainLayout->insertLayout(1, scrolllayout, 1);

    content->setLayout(m_mainLayout);
}

void ImgInfoDialog::setImagePath(const QString &path)
{
    m_path = path;
    m_isBaseInfo = false;
    m_isDetailsInfo = false;
    updateInfo();

    QStringList titleList;
    QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(m_scrollArea->widget()->layout());
    if (nullptr != layout) {
        QLayoutItem *child;
        while ((child = layout->takeAt(0)) != nullptr) {
            layout->removeWidget(child->widget());
            child->widget()->setParent(nullptr);
            delete child;
        }
    }
    m_expandGroup.clear();

    if (m_isBaseInfo == true && m_isDetailsInfo == true) {
        titleList << tr("Basic info");
        titleList << tr("Details");
        m_expandGroup = addExpandWidget(titleList);
        m_expandGroup.at(0)->setContent(m_exif_base);
        m_expandGroup.at(0)->setExpand(true);
        m_expandGroup.at(1)->setContent(m_exif_details);
        m_expandGroup.at(1)->setExpand(true);

    } else if (m_isBaseInfo == false && m_isDetailsInfo == true) {
        titleList << tr("Details");
        m_expandGroup = addExpandWidget(titleList);
        m_expandGroup.at(0)->setContent(m_exif_details);
        m_expandGroup.at(0)->setExpand(true);
    } else if (m_isBaseInfo == true && m_isDetailsInfo == false) {
        titleList << tr("Basic info");
        m_expandGroup = addExpandWidget(titleList);
        m_expandGroup.at(0)->setContent(m_exif_base);
        m_expandGroup.at(0)->setExpand(true);
    }
    //剔除多余不需要的焦点
    QList<QWidget *>allwidgetlist = this->findChildren<QWidget *>();
    for (int i = 0; i < allwidgetlist.size() ; i++) {
        allwidgetlist.at(i)->setFocusPolicy(Qt::NoFocus);
    }
    for (int j = 0; j < m_expandGroup.size(); j++) {
        QList<DIconButton *> expandlist = m_expandGroup.at(j)->findChildren<DIconButton *>();
        for (int k = 0; k < expandlist.size(); k++) {
            expandlist.at(k)->setFocusPolicy(Qt::TabFocus);
        }
    }
    QWidget *closeButton = this->findChild<QWidget *>("DTitlebarDWindowCloseButton");
    if (closeButton) {
        closeButton->setFocusPolicy(Qt::TabFocus);
    }
}

void ImgInfoDialog::clearLayout(QLayout *layout)
{
    QFormLayout *fl = static_cast<QFormLayout *>(layout);
    if (fl) {
        // FIXME fl->rowCount() will always increase
        for (int i = 0; i < fl->rowCount(); i++) {
            QLayoutItem *li = fl->itemAt(i, QFormLayout::LabelRole);
            QLayoutItem *fi = fl->itemAt(i, QFormLayout::FieldRole);
            if (li) {
                if (li->widget()) delete li->widget();
                fl->removeItem(li);
            }
            if (fi) {
                if (fi->widget()) delete fi->widget();
                fl->removeItem(fi);
            }
        }
    }
}

const QString ImgInfoDialog::trLabel(const char *str)
{
    return qApp->translate("MetadataName", str);
}

void ImgInfoDialog::updateInfo()
{
    auto mds = UnionImage_NameSpace::getAllMetaData(m_path);
    m_maxFieldWidth = width() - m_title_maxwidth - 20 * 2 - 10 * 2;
    updateBaseInfo(mds);
    updateDetailsInfo(mds);
}

void ImgInfoDialog::updateBaseInfo(const QMap<QString, QString> &infos)
{
    using namespace utils::image;
    using namespace utils::base;
    clearLayout(m_exifLayout_base);

    for (MetaData *i = MetaDataBasics; ! i->key.isEmpty(); i ++) {
        QString value = infos.value(i->key);
        if (i->key.contains("Dimension")) {
            value = infos.value("Dimension");
            if (value == "0x0") {
                QImage tImg;
                QString errMsg;
                if (!UnionImage_NameSpace::loadStaticImageFromFile(m_path, tImg, errMsg)) {
                    qDebug() << errMsg;
                    continue;
                }
                value = QString::number(tImg.width()) + "x" + QString::number(tImg.height());
            }
        } else if (i->key.contains("FileFormat")) {
            QStringList list = value.split("/");
            if (list.count() > 0) {
                value = list.value(list.count() - 1);
            }
        } else if (i->key == "DateTimeOriginal" || i->key == "DateTimeDigitized") {
            QStringList list = value.split(" ");
            if (2 == list.count()) {
                QStringList listDate = list[0].split("/");
                if (3 == listDate.count()) {
                    value = listDate[0] + "/" + listDate[1] + "/" + listDate[2] + " " + list[1];
                }
            }

        }
        if (value.isEmpty()) continue;

        m_isBaseInfo = true;
        if (!((i->key == "DateTimeOriginal" || i->key == "DateTimeDigitized") && '0' == value.left(1))) {
            SimpleFormField *field = new SimpleFormField;
            field->setAlignment(Qt::AlignLeft | Qt::AlignTop);
            DFontSizeManager::instance()->bind(field, DFontSizeManager::T8);
            DPalette pa1 = DApplicationHelper::instance()->palette(field);
            pa1.setBrush(DPalette::Text, pa1.color(DPalette::TextTitle));
            field->setPalette(pa1);
            field->setText(SpliteText(value, field->font(), m_maxFieldWidth));

            SimpleFormLabel *title = new SimpleFormLabel(trLabel(i->name) + ":");
            title->setMinimumHeight(field->minimumHeight());
            title->setFixedWidth(m_title_maxwidth);
            title->setAlignment(Qt::AlignLeft | Qt::AlignTop);
            //2020/03/13-xiaolong
            DFontSizeManager::instance()->bind(title, DFontSizeManager::T8);
            DPalette pa2 = DApplicationHelper::instance()->palette(title);
            pa2.setBrush(DPalette::Text, pa2.color(DPalette::TextTitle));
            title->setPalette(pa2);
            title->setText(SpliteText(trLabel(i->name) + ":", title->font(), m_title_maxwidth));

            m_exifLayout_base->addRow(title, field);
        }
    }
}

void ImgInfoDialog::updateDetailsInfo(const QMap<QString, QString> &infos)
{
    using namespace utils::image;
    using namespace utils::base;
    clearLayout(m_exifLayout_details);
    for (MetaData *i = MetaDataDetails; ! i->key.isEmpty(); i ++) {
        QString value = infos.value(i->key);
        if (value.isEmpty()) continue;
        m_isDetailsInfo = true;
        SimpleFormField *field = new SimpleFormField;
        field->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        DFontSizeManager::instance()->bind(field, DFontSizeManager::T8);
        DPalette pa1 = DApplicationHelper::instance()->palette(field);
        pa1.setBrush(DPalette::Text, pa1.color(DPalette::TextTitle));
        field->setPalette(pa1);
        field->setText(SpliteText(value, field->font(), m_maxFieldWidth));

        SimpleFormLabel *title = new SimpleFormLabel(trLabel(i->name) + ":");
        title->setMinimumHeight(field->minimumHeight());
        title->setFixedWidth(m_title_maxwidth);
        title->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        DFontSizeManager::instance()->bind(title, DFontSizeManager::T8);
        DPalette pa2 = DApplicationHelper::instance()->palette(title);
        pa2.setBrush(DPalette::Text, pa2.color(DPalette::TextTitle));
        title->setPalette(pa2);
        title->setText(SpliteText(trLabel(i->name) + ":", title->font(), m_title_maxwidth));
        QString text = title->text();
        m_exifLayout_details->addRow(title, field);
    }
}

QList<DDrawer *> ImgInfoDialog::addExpandWidget(const QStringList &titleList)
{
    QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(m_scrollArea->widget()->layout());
    QList<DDrawer *> group;

    for (const QString &title : titleList) {
        DFMDArrowLineExpand *expand = new DFMDArrowLineExpand;//DArrowLineExpand;
        expand->setTitle(title);
        initExpand(layout, expand);
        group.push_back(expand);
    }

    return group;
}
void ImgInfoDialog::initExpand(QVBoxLayout *layout, DDrawer *expand)
{
    expand->setFixedHeight(30);
    QMargins cm = layout->contentsMargins();
    QRect rc1 = contentsRect();
    expand->setFixedWidth(rc1.width() - cm.left() - cm.right());
    // 去掉界面的横线
    expand->setExpandedSeparatorVisible(false);
    expand->setSeparatorVisible(false);
    layout->addWidget(expand, 0, Qt::AlignTop);

    DEnhancedWidget *hanceedWidget = new DEnhancedWidget(expand, this);
    connect(hanceedWidget, &DEnhancedWidget::heightChanged, hanceedWidget, [ = ]() {
        QRect rc = geometry();
        if (m_expandGroup.count() == 2) {
            rc.setHeight(contentHeight() + 40);
            setGeometry(rc);
            this->setFixedHeight(qMin(615, contentHeight() + 15));
        } else {
            rc.setHeight(contentHeight() + 40);
            setGeometry(rc);
            this->setFixedHeight(qMin(615, contentHeight() + 25));
        }
//        if(expand->expand()){
//            emit dApp->signalM->extensionPanelHeight(qMin(615,contentHeight()+5));
//        }
    });
}

int ImgInfoDialog::contentHeight() const
{
    int expandsHeight = 10;
//    bool atleastOneExpand = false;
//    for (const DBaseExpand* expand : m_expandGroup) {
//        expandsHeight += 30 + 15;
//        if (expand->expand()) {
//            expandsHeight += expand->getContent()->height();
//            atleastOneExpand = true;
//        }
//    }
//    for (const DDrawer *expand : m_expandGroup) {
//        expandsHeight += expand->height();
//    }
    QList<DDrawer *>::const_iterator expand = m_expandGroup.cbegin();
    while (expand != m_expandGroup.cend()) {
        expandsHeight += (*expand)->height();
        ++expand;
    }
    return (50 + expandsHeight + contentsMargins().top() +
            contentsMargins().bottom());
}

void ImgInfoDialog::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape) {
        this->close();
    }
    DDialog::keyPressEvent(e);
}
//LMH0424检查字体大小随系统改变，从而刷新字体。
void ImgInfoDialog::paintEvent(QPaintEvent *event)
{
    QFont font;
    int currentSize = DFontSizeManager::instance()->fontPixelSize(font);
    if (currentSize != m_currentFontSize) {
        m_currentFontSize = currentSize;
        updateInfo();
    }
    DDialog::paintEvent(event);
}

bool ImgInfoDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (QApplication::activeWindow() != this) {
            this->close();
        }
    }
    return QWidget::event(event);
}

bool ImgInfoDialog::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == m_scrollArea->viewport()) {
        QMouseEvent *event = dynamic_cast<QMouseEvent *>(e);
        if (e->type() == QEvent::MouseButtonPress && event) {
            m_mousePress = true;
            m_mouseY = event->pos().y();
            return true;
        }
        if (e->type() == QEvent::MouseMove && event) {
            int sliderPosition = m_scrollArea->verticalScrollBar()->sliderPosition();
            if (m_mousePress) {
                if (event->pos().y() < m_mouseY) {
                    m_scrollArea->verticalScrollBar()->setSliderPosition(sliderPosition + 1);
                } else if (event->pos().y() > m_mouseY) {
                    m_scrollArea->verticalScrollBar()->setSliderPosition(sliderPosition - 1);
                }
            }
            m_mouseY = event->pos().y();
            return true;
        }
        if (e->type() == QEvent::MouseButtonRelease && event) {
            m_mousePress = false;
            return true;
        }
    }

//    if (e->type() == QEvent::KeyPress) {
//        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
//        if (keyEvent->key() == Qt::Key_Tab) {
//            if (m_allTabOrder.at(0) == obj) {
//                m_allTabOrder.at(1)->setFocus();
//                return true;
//            } else if (m_allTabOrder.at(1) == obj) {
//                m_allTabOrder.at(0)->setFocus();
//                return true;
//            }
//        }
//    }
    return DDialog::eventFilter(obj, e);
}

