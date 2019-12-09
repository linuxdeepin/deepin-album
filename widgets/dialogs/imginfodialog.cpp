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
#include "imginfodialog.h"
#include "application.h"
#include "controller/signalmanager.h"
#include "utils/imageutils.h"
#include "widgets/formlabel.h"

#include <QApplication>
#include <QBoxLayout>
#include <QDateTime>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QString>
#include <QPushButton>
#include <QScrollBar>
#include <QtDebug>
#include <DFontSizeManager>
#include <DApplicationHelper>
#include <DDialogCloseButton>
#include <QGraphicsOpacityEffect>

namespace {

const int TITLE_MAXWIDTH = 72-10;
struct MetaData {
    QString key;
    const char *name;
};

static MetaData MetaDataBasics[] = {
    {"FileName",            QT_TRANSLATE_NOOP("MetadataName", "Photo name")},
    {"FileFormat",          QT_TRANSLATE_NOOP("MetadataName", "Type")},
    {"FileSize",            QT_TRANSLATE_NOOP("MetadataName", "File size")},
    {"Dimension",           QT_TRANSLATE_NOOP("MetadataName", "Dimensions")},
    {"DateTimeOriginal",    QT_TRANSLATE_NOOP("MetadataName", "Date captured")},
    {"DateTimeDigitized",   QT_TRANSLATE_NOOP("MetadataName", "Date modified")},
    {"Tag",                 QT_TRANSLATE_NOOP("MetadataName", "Tag")},
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

class DFMDArrowLineExpand : public DArrowLineExpand{
public:
    DFMDArrowLineExpand(){
        if (headerLine()) {
            DFontSizeManager::instance()->bind(headerLine(), DFontSizeManager::T6);

            DPalette pa = DApplicationHelper::instance()->palette(headerLine());
            pa.setBrush(DPalette::Text, pa.color(DPalette::TextTitle));
            headerLine()->setPalette(pa);
            connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this, [=]{
                DPalette pa = DApplicationHelper::instance()->palette(headerLine());
                pa.setBrush(DPalette::Text, pa.color(DPalette::TextTitle));
                headerLine()->setPalette(pa);
            });

            headerLine()->setLeftMargin(10);
        }
    }
protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        QRectF bgRect;
        bgRect.setSize(size());
        const QPalette pal = QGuiApplication::palette();//this->palette();
        QColor bgColor = pal.color(QPalette::Background);

        QPainterPath path;
        path.addRoundedRect(bgRect, 8, 8);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillPath(path, bgColor);
        painter.setRenderHint(QPainter::Antialiasing, false);
    }
};


ImgInfoDialog::ImgInfoDialog(const QString &path, QWidget *parent)
    : DDialog(parent)
{
    initUI();
    setImagePath(path);
}

int ImgInfoDialog::height()
{
    return contentHeight() + 10;
}

void ImgInfoDialog::initUI()
{
//    setAutoFillBackground(true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFixedWidth(300);

    setContentLayoutContentsMargins(QMargins(0,0,0,0));

    //title
    DLabel *title = new DLabel(this);
    title->setText(tr("Photo info"));
    title->setGeometry(this->x() + (this->width() - title->width())/2,this->y(),112,50);
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

    m_scrollArea = new QScrollArea();
    QPalette palette = m_scrollArea->viewport()->palette();
    palette.setBrush(QPalette::Background, Qt::NoBrush);
    m_scrollArea->viewport()->setPalette(palette);
    m_scrollArea->setFrameShape(QFrame::Shape::NoFrame);
    QFrame *infoframe= new QFrame;
    QVBoxLayout *scrollWidgetLayout = new QVBoxLayout;
    scrollWidgetLayout->setContentsMargins(10, 0, 10, 0);
    scrollWidgetLayout->setSpacing(10);
    infoframe->setLayout(scrollWidgetLayout);
    m_scrollArea->setWidget(infoframe);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);

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
    if(nullptr != layout)
    {
        QLayoutItem *child;
        while ((child = layout->takeAt(0)) != 0)
        {
            layout->removeWidget(child->widget());
            child->widget()->setParent(0);
            delete child;
        }
    }
    m_expandGroup.clear();

    if(m_isBaseInfo == true && m_isDetailsInfo == true ){
        titleList << tr("Basic info");
        titleList << tr("Details");
        m_expandGroup = addExpandWidget(titleList);
        m_expandGroup.at(0)->setContent(m_exif_base);
        m_expandGroup.at(0)->setExpand(true);
        m_expandGroup.at(1)->setContent(m_exif_details);
        m_expandGroup.at(1)->setExpand(true);

    }
    else if(m_isBaseInfo == false && m_isDetailsInfo == true ){
        titleList << tr("Details");
        m_expandGroup = addExpandWidget(titleList);
        m_expandGroup.at(0)->setContent(m_exif_details);
        m_expandGroup.at(0)->setExpand(true);
    }
    else if(m_isBaseInfo == true && m_isDetailsInfo == false ){
        titleList << tr("Basic info");
        m_expandGroup = addExpandWidget(titleList);
        m_expandGroup.at(0)->setContent(m_exif_base);
        m_expandGroup.at(0)->setExpand(true);
    }
}

void ImgInfoDialog::clearLayout(QLayout *layout) {
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
    using namespace utils::image;
    using namespace utils::base;
    auto mds = getAllMetaData(m_path);
    m_maxFieldWidth = width() - TITLE_MAXWIDTH - 20*2 - 10*2;
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
        if (value.isEmpty()) continue;

        m_isBaseInfo = true;
        if(!((i->key == "DateTimeOriginal" || i->key == "DateTimeDigitized") && '0' == value.left(1)))
        {
        SimpleFormField *field = new SimpleFormField;
        field->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        DFontSizeManager::instance()->bind(field, DFontSizeManager::T8);
        DPalette pa1 = DApplicationHelper::instance()->palette(field);
        pa1.setBrush(DPalette::Text, pa1.color(DPalette::TextTitle));
        field->setPalette(pa1);
        field->setText(SpliteText(value, field->font(), m_maxFieldWidth));

        SimpleFormLabel *title = new SimpleFormLabel(trLabel(i->name) + ":");
        title->setMinimumHeight(field->minimumHeight());
        title->setFixedWidth(TITLE_MAXWIDTH);
        title->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        DFontSizeManager::instance()->bind(title, DFontSizeManager::T8);
        DPalette pa2= DApplicationHelper::instance()->palette(title);
        pa2.setBrush(DPalette::Text, pa2.color(DPalette::TextTitle));
        title->setPalette(pa2);
        title->setText(SpliteText(trLabel(i->name) + ":", title->font(), TITLE_MAXWIDTH));

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
        title->setFixedWidth(TITLE_MAXWIDTH);
        title->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        DFontSizeManager::instance()->bind(title, DFontSizeManager::T8);
        DPalette pa2= DApplicationHelper::instance()->palette(title);
        pa2.setBrush(DPalette::Text, pa2.color(DPalette::TextTitle));
        title->setPalette(pa2);
        title->setText(SpliteText(trLabel(i->name) + ":", title->font(), TITLE_MAXWIDTH));

        m_exifLayout_details->addRow(title, field);
    }
}

QList<DBaseExpand *> ImgInfoDialog::addExpandWidget(const QStringList &titleList)
{
    QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(m_scrollArea->widget()->layout());
    QList<DBaseExpand *> group;

    for (const QString &title : titleList) {
        DFMDArrowLineExpand *expand = new DFMDArrowLineExpand;//DArrowLineExpand;
        expand->setTitle(title);
        initExpand(layout, expand);
        group.push_back(expand);
    }

    return group;
}
void ImgInfoDialog::initExpand(QVBoxLayout *layout, DBaseExpand *expand)
{
    expand->setFixedHeight(30);
    QMargins cm = layout->contentsMargins();
    QRect rc = contentsRect();
    expand->setFixedWidth(rc.width()-cm.left()-cm.right());
    layout->addWidget(expand, 0, Qt::AlignTop);

    DEnhancedWidget *hanceedWidget = new DEnhancedWidget(expand, this);
    connect(hanceedWidget, &DEnhancedWidget::heightChanged, hanceedWidget, [=](){
        QRect rc = geometry();
        rc.setHeight(contentHeight()+10);
        setGeometry(rc);

        if(expand->expand()){
            emit dApp->signalM->extensionPanelHeight(contentHeight()+20);
        }
    });
}

int ImgInfoDialog::contentHeight() const
{
    int expandsHeight = 0;
    bool atleastOneExpand = false;
    for (const DBaseExpand* expand : m_expandGroup) {
        expandsHeight += 30 + 15;
        if (expand->expand()) {
            expandsHeight += expand->getContent()->height();
            atleastOneExpand = true;
        }
    }
    return ( qMin(expandsHeight + 45 +4,530));
}

void ImgInfoDialog::keyPressEvent(QKeyEvent *e)
{
    if(e->key() == Qt::Key_Escape){
        emit closed();
    }
    DDialog::keyPressEvent(e);
}
