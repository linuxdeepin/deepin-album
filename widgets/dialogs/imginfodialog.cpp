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
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "../imagebutton.h"
#include "widgets/formlabel.h"

#include <dtitlebar.h>
#include <QFormLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>

namespace {

const int MAX_WIDTH = 250;
const int THUMBNAIL_WIDTH = 240;
const int THUMBNAIL_HEIGHT = 160;
const int TITLE_MAXWIDTH = 100;

struct MetaData {
    QString key;
    const char *name;
};

static MetaData MetaDataBasics[] = {
    {"FileName",            QT_TRANSLATE_NOOP("MetadataName", "照片名称")},
    {"FileFormat",          QT_TRANSLATE_NOOP("MetadataName", "照片类型")},
    {"FileSize",            QT_TRANSLATE_NOOP("MetadataName", "照片大小")},
    {"Dimension",           QT_TRANSLATE_NOOP("MetadataName", "照片尺寸")},
    {"DateTimeOriginal",    QT_TRANSLATE_NOOP("MetadataName", "拍摄时间")},
    {"DateTimeDigitized",   QT_TRANSLATE_NOOP("MetadataName", "修改时间")},
    {"", ""}
};

static MetaData MetaDataDetails[] = {
    {"ExposureMode",        QT_TRANSLATE_NOOP("MetadataName", "曝光模式")},
    {"ExposureProgram",     QT_TRANSLATE_NOOP("MetadataName", "曝光程序")},
    {"ExposureTime",        QT_TRANSLATE_NOOP("MetadataName", "曝光时间")},
    {"Flash",               QT_TRANSLATE_NOOP("MetadataName", "闪光灯")},
    {"ApertureValue",       QT_TRANSLATE_NOOP("MetadataName", "光圈大小")},
    {"FocalLength",         QT_TRANSLATE_NOOP("MetadataName", "焦距")},
    {"ISOSpeedRatings",     QT_TRANSLATE_NOOP("MetadataName", "IOS光感度")},
    {"MaxApertureValue",    QT_TRANSLATE_NOOP("MetadataName", "最大光圈值")},
    {"MeteringMode",        QT_TRANSLATE_NOOP("MetadataName", "测光模式")},
    {"WhiteBalance",        QT_TRANSLATE_NOOP("MetadataName", "白平衡")},
    {"FlashExposureComp",   QT_TRANSLATE_NOOP("MetadataName", "闪光灯补偿")},
    {"Model",               QT_TRANSLATE_NOOP("MetadataName", "镜头型号")},
    {"", ""}
};

static int maxTitleWidth()
{
    int maxWidth = 0;
    QFont tf;
    tf.setPixelSize(12);
    for (const MetaData* i = MetaDataBasics; ! i->key.isEmpty(); ++i) {
        maxWidth = qMax(maxWidth + 1, utils::base::stringWidth(tf, i->name));
    }
    for (const MetaData* i = MetaDataDetails; ! i->key.isEmpty(); ++i) {
        maxWidth = qMax(maxWidth + 1, utils::base::stringWidth(tf, i->name));
    }

    return maxWidth;
}
}  // namespace

ImgInfoDialog::ImgInfoDialog(const QString &path, QWidget *parent)
    : DDialog(parent),m_maxTitleWidth(maxTitleWidth())
{

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags()
                   & ~ Qt::WindowMaximizeButtonHint
                   & ~ Qt::WindowMinimizeButtonHint
                   & ~ Qt::WindowSystemMenuHint);
    m_path = path;
    QString basicInfo = tr("Basic info");
    QString openWith = tr("Details Info");
    initUI();
    QStringList titleList;
    titleList << basicInfo;
    titleList << openWith;
    m_exifLayout_base = new QFormLayout();
    m_exifLayout_details = new QFormLayout();

//    m_expandGroup = addExpandWidget(titleList);

    QFrame *baseFrame = createBaseInfoFrame();
    QFrame *detailsFrame = createDetailsInfoFrame();
    updateInfo();
    baseFrame->setFixedHeight(baseInfoHeidht);
    detailsFrame->setFixedHeight(detailsInfoHeidht);
    m_mainLayout->addWidget(baseFrame);
    m_mainLayout->addWidget(detailsFrame);

//    m_expandGroup->expand(0)->setContent(baseFrame,Qt::AlignTop);
//    m_expandGroup->expand(1)->setContent(detailsFrame,Qt::AlignTop);
//    m_expandGroup->expand(0)->setExpand(true);
//    m_expandGroup->expand(1)->setExpand(true);
//    m_expandGroup->expands().first()->setExpand(true);
//    m_expandGroup->expands().last()->setExpandedSeparatorVisible(true);
//    m_mainLayout->addStretch(1);

}
void ImgInfoDialog::initUI()
{
    m_mainLayout = new QVBoxLayout;

    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setMargin(0);
    m_mainLayout->setSpacing(0);
    m_mainLayout->setAlignment(Qt::AlignTop);
    QFrame *frame = new QFrame(this);
    frame->setLayout(m_mainLayout);
    addContent(frame);
}
void ImgInfoDialog::updateInfo()
{
    using namespace utils::image;
    using namespace utils::base;
    auto mds = getAllMetaData(m_path);
    // Minus layout margins
    m_maxFieldWidth = width() - m_maxTitleWidth - (10 + 8) * 2;
    updateBaseInfo(mds);
    bool ret = false;
    for (MetaData *i = MetaDataDetails; ! i->key.isEmpty(); i ++) {
        if(mds.value(i->key).length() > 0)
        {
            ret = true;
            break;
        }
    }
    if(ret)
    {
        updateDetailsInfo(mds);
    }
}

QFrame *ImgInfoDialog::createBaseInfoFrame()
{
    QFrame *widget = new QFrame();
    m_exifLayout_base = new QFormLayout;
    m_exifLayout_base->setHorizontalSpacing(12);
    m_exifLayout_base->setVerticalSpacing(16);
    m_exifLayout_base->setLabelAlignment(Qt::AlignRight);

    widget->setLayout(m_exifLayout_base);
    widget->setFixedHeight(200);
    return widget;
}
QFrame *ImgInfoDialog::createDetailsInfoFrame()
{
    QFrame *widget = new QFrame();

    m_exifLayout_details = new QFormLayout;
    m_exifLayout_details->setHorizontalSpacing(12);
    m_exifLayout_details->setVerticalSpacing(16);
    m_exifLayout_details->setLabelAlignment(Qt::AlignRight);

    widget->setLayout(m_exifLayout_details);
    widget->setFixedHeight(500);
    return widget;
}
void ImgInfoDialog::updateBaseInfo(const QMap<QString, QString> &infos)
{
    using namespace utils::image;
    using namespace utils::base;
    clearLayout(m_exifLayout_base);

    SimpleFormLabel *infoTitle = new SimpleFormLabel(tr("基本信息"));
    infoTitle->setAlignment(Qt::AlignLeft);
    m_exifLayout_base->addRow(infoTitle);
    baseInfoHeidht+=50;
    for (MetaData *i = MetaDataBasics; ! i->key.isEmpty(); i ++) {
        QString value = infos.value(i->key);
        if (value.isEmpty()) continue;
        SimpleFormField *field = new SimpleFormField;
        field->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        field->setText(wrapStr(value, field->font(), m_maxFieldWidth));
        SimpleFormLabel *title = new SimpleFormLabel(trLabel(i->name) + ":");
        title->setMinimumHeight(field->minimumHeight());
        title->setFixedWidth(qMin(m_maxTitleWidth, TITLE_MAXWIDTH));
        title->setAlignment(Qt::AlignRight | Qt::AlignTop);
        m_exifLayout_base->addRow(title, field);
        baseInfoHeidht+=40;
    }
    baseInfoHeidht-=15;
}

void ImgInfoDialog::updateDetailsInfo(const QMap<QString, QString> &infos)
{
    using namespace utils::image;
    using namespace utils::base;
    clearLayout(m_exifLayout_details);

    SimpleFormLabel *infoTitle = new SimpleFormLabel(tr("详细信息"));
    infoTitle->setAlignment(Qt::AlignLeft);
    m_exifLayout_details->addRow(infoTitle);
    detailsInfoHeidht+=40;
    for (MetaData *i = MetaDataDetails; ! i->key.isEmpty(); i ++) {
        QString value = infos.value(i->key);
        if (value.isEmpty()) continue;
        SimpleFormField *field = new SimpleFormField;
        field->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        field->setText(wrapStr(value, field->font(), m_maxFieldWidth));
        SimpleFormLabel *title = new SimpleFormLabel(trLabel(i->name) + ":");
        title->setMinimumHeight(field->minimumHeight());
        title->setFixedWidth(qMin(m_maxTitleWidth, TITLE_MAXWIDTH));
        title->setAlignment(Qt::AlignRight | Qt::AlignTop);
        m_exifLayout_details->addRow(title, field);
        detailsInfoHeidht+=35;
    }

//    m_separator->setVisible(m_exifLayout_details->count() > 10);
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
DExpandGroup *ImgInfoDialog::expandGroup() const
{
    return m_expandGroup;
}
void ImgInfoDialog::loadPluginExpandWidgets()
{

}
DExpandGroup *ImgInfoDialog::addExpandWidget(const QStringList &titleList)
{
//    QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(this->layout());
    DExpandGroup *group = new DExpandGroup;
//    QLabel *line = new QLabel(this);
//    line->setObjectName("Line");
//    line->setFixedHeight(1);
//    layout->addWidget(line, 0, Qt::AlignTop);

    for (const QString &title : titleList) {
        DArrowLineExpand *expand = new DArrowLineExpand;

        expand->setTitle(title);
        expand->setFixedHeight(30);

        connect(expand, &DArrowLineExpand::expandChange, this, &ImgInfoDialog::onExpandChanged);

//        layout->addWidget(expand, 0, Qt::AlignTop);
        m_mainLayout->addWidget(expand, 0, Qt::AlignTop);
        group->addExpand(expand);
    }
    return group;
}

void ImgInfoDialog::onExpandChanged(const bool &e)
{
    DArrowLineExpand *expand = qobject_cast<DArrowLineExpand *>(sender());
    if (expand) {
        if (e) {
            expand->setSeparatorVisible(false);
        } else {
            QTimer::singleShot(200, expand, [ = ] {
                expand->setSeparatorVisible(true);
            });
        }
    }
}
void ImgInfoDialog::hideEvent(QHideEvent *e)
{
//    DMainWindow::hideEvent(e);

    emit closed();
}

void ImgInfoDialog::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape) {
        this->close();
    }
}


void ImgInfoDialog::initThumbnail(const QString &path)
{
    using namespace utils::image;
    QLabel *iconLabel = new QLabel;
    iconLabel->setObjectName("IconLabel");
    QPixmap pixmap = cutSquareImage(getThumbnail(path),
                                    QSize(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT) * devicePixelRatioF());
    pixmap.setDevicePixelRatio(devicePixelRatioF());
    // Draw inside-border
    QPainter p(&pixmap);
    QRect br(0, 0, pixmap.width() - 1, pixmap.height() - 1);
    QPainterPath pp;
    pp.addRect(br);
    p.setPen(QPen(QColor(0, 0, 0, 0.2 * 255), 1));
    p.drawPath(pp);

    iconLabel->setPixmap(pixmap);
    m_layout->addWidget(iconLabel, 0, Qt::AlignHCenter);
}

void ImgInfoDialog::initSeparator()
{
    QLabel *sl = new QLabel;
    sl->setObjectName("Separator");
    sl->setFixedSize(MAX_WIDTH - 5 * 2, 1);
    m_layout->addSpacing(9);
    m_layout->addWidget(sl, 0, Qt::AlignHCenter);
}

void ImgInfoDialog::initInfos(const QString &path)
{
//    using namespace utils::image;
//    using namespace utils::base;
//    QWidget *w = new QWidget;
//    QFormLayout *infoLayout = new QFormLayout(w);
//    infoLayout->setSpacing(8);
//    infoLayout->setContentsMargins(10, 15, 10, 26);
//    infoLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);

//    auto mds = getAllMetaData(path);
//    for (const MetaData* i = MetaDatas; ! i->key.isEmpty(); i ++) {
//        QString v = mds.value(i->key);
//        if (v.isEmpty()) continue;

//        SimpleFormField *field = new SimpleFormField;
//        field->setObjectName("Field");
//        field->setAlignment(Qt::AlignLeft | Qt::AlignTop);
//        field->setText(wrapStr(v, field->font(), TITLE_MAXWIDTH + 30));
//        if (QString(i->name) == "Name")
//            field->setMinimumHeight(stringHeight(field->font(),
//                   field->text()) * field->text().split(" ").length());

//        SimpleFormLabel *title = new SimpleFormLabel(dApp->translate("MetadataName", i->name) + ":");
//        title->setObjectName("Title");
//        title->setFixedHeight(field->maximumHeight());
//        title->setFixedWidth(qMin(maxTitleWidth(), TITLE_MAXWIDTH));
//        title->setAlignment(Qt::AlignRight | Qt::AlignTop);

//        infoLayout->addRow(title, field);
//    }

//    m_layout->addWidget(w, 0, Qt::AlignHCenter);
}

void ImgInfoDialog::initCloseButton()
{
    ImageButton* cb = new ImageButton(this);
    cb->setTooltipVisible(true);
    cb->setNormalPic(":/resources/common/images/window_close_normal.svg");
    cb->setHoverPic(":/resources/common/images/window_close_hover.svg");
    cb->setPressPic("::/resources/common/images/window_close_press.svg");
    cb->setFixedSize(27, 23);
    cb->move(this->x() + this->width() - cb->width() + 1, 4);
    connect(cb, &ImageButton::clicked, this, &ImgInfoDialog::close);
}
