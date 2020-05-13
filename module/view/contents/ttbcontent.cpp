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
#include "ttbcontent.h"
#include "application.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"

#include "widgets/pushbutton.h"
#include "widgets/returnbutton.h"
#include "dbmanager/dbmanager.h"
#include "controller/configsetter.h"
#include "widgets/elidedlabel.h"
#include "controller/signalmanager.h"
#include "imageengine/imageengineapi.h"

#include <QTimer>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QDebug>
#include <DLabel>
#include <QAbstractItemModel>
#include <DImageButton>
#include <DThumbnailProvider>
#include <DApplicationHelper>
#include <DSpinner>
#include <QtMath>

DWIDGET_USE_NAMESPACE
namespace {
const int LEFT_MARGIN = 10;
const QSize ICON_SIZE = QSize(50, 50);
const int ICON_SPACING = 10;
const int RETURN_BTN_MAX = 200;
const int FILENAME_MAX_LENGTH = 600;
const int RIGHT_TITLEBAR_WIDTH = 100;
const int LEFT_SPACE = 20;
const QString LOCMAP_SELECTED_DARK = ":/resources/dark/images/58 drak.svg";
const QString LOCMAP_NOT_SELECTED_DARK = ":/resources/dark/images/imagewithbg-dark.svg";
const QString LOCMAP_SELECTED_LIGHT = ":/resources/light/images/58.svg";
const QString LOCMAP_NOT_SELECTED_LIGHT = ":/resources/light/images/imagewithbg.svg";

const int TOOLBAR_MINIMUN_WIDTH = 782;
const int TOOLBAR_JUSTONE_WIDTH = 532;
const int RT_SPACING = 20;
const int TOOLBAR_HEIGHT = 60;

const int TOOLBAR_DVALUE = 114 + 8;

const int THUMBNAIL_WIDTH = 32;
const int THUMBNAIL_ADD_WIDTH = 32;
const int THUMBNAIL_LIST_ADJUST = 9;
const int THUMBNAIL_VIEW_DVALUE = 668;

const unsigned int IMAGE_TYPE_JEPG = 0xFFD8FF;
const unsigned int IMAGE_TYPE_JPG1 = 0xFFD8FFE0;
const unsigned int IMAGE_TYPE_JPG2 = 0xFFD8FFE1;
const unsigned int IMAGE_TYPE_JPG3 = 0xFFD8FFE8;
const unsigned int IMAGE_TYPE_PNG = 0x89504e47;
const unsigned int IMAGE_TYPE_GIF = 0x47494638;
const unsigned int IMAGE_TYPE_TIFF = 0x49492a00;
const unsigned int IMAGE_TYPE_BMP = 0x424d;
}  // namespace

char *getImageType(QString filepath)
{
    char *ret = nullptr;
    QFile file(filepath);
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);

    // Read and check the header
    quint32 magic;
    in >> magic;
    switch (magic) {
    case IMAGE_TYPE_JEPG:
    case IMAGE_TYPE_JPG1:
    case IMAGE_TYPE_JPG2:
    case IMAGE_TYPE_JPG3:
        //文件类型为 JEPG
        ret = "JEPG";
        break;
    case IMAGE_TYPE_PNG:
        //文件类型为 png
        ret = "PNG";
        break;
    case IMAGE_TYPE_GIF:
        //文件类型为 GIF
        ret = "GIF";
        break;
    case IMAGE_TYPE_TIFF:
        //文件类型为 TIFF
        ret = "TIFF";
        break;
    case IMAGE_TYPE_BMP:
        //文件类型为 BMP
        ret = "BMP";
        break;
    default:
        ret = nullptr;
        break;
    }
    return ret;
};

MyImageListWidget::MyImageListWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
}

bool MyImageListWidget::ifMouseLeftPressed()
{
    return bmouseleftpressed;
}

void MyImageListWidget::setObj(QObject *obj)
{
    m_obj = obj;
}

bool MyImageListWidget::eventFilter(QObject *obj, QEvent *e)
{
    Q_UNUSED(obj)

    if (e->type() == QEvent::MouseButtonPress) {
        bmouseleftpressed = true;
        QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent *>(e);
        m_prepoint = mouseEvent->globalPos();
        qDebug() << "m_prepoint:" << m_prepoint;
    }

    if (e->type() == QEvent::MouseButtonRelease) {
        bmouseleftpressed = false;
        emit mouseLeftReleased();
    }
    if (e->type() == QEvent::Leave && obj == m_obj) {
        bmouseleftpressed = false;
        emit mouseLeftReleased();
    }
    if (e->type() == QEvent::MouseMove && bmouseleftpressed) {
        QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent *>(e);
        QPoint p = mouseEvent->globalPos();
        dynamic_cast<DWidget *>(m_obj)->move((dynamic_cast<DWidget *>(m_obj))->x() + p.x() - m_prepoint.x(), ((dynamic_cast<DWidget *>(m_obj))->y()));
        m_prepoint = p;
        if ((dynamic_cast<DWidget *>(m_obj)->width() + dynamic_cast<DWidget *>(m_obj)->x() - this->width() - 30) < 0) {
            emit needContinueRequest();
        } else {
            emit silmoved();
        }
    }
    return false;
}

ImageItem::ImageItem(int index, ImageDataSt data, QWidget *parent):
    QLabel(parent)
{
    _index = index;
    _path = data.dbi.filePath;
//    qDebug() << index << _path;
    m_bPicNotSuppOrDamaged = data.imgpixmap.isNull();
    bool bLight = DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType;
    _pixmap = m_bPicNotSuppOrDamaged ? utils::image::getDamagePixmap(bLight) : data.imgpixmap;
    _image = new DLabel(this);

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &ImageItem::updateDmgIconByTheme);
}

//ImageItem::ImageItem(int index, QString path, QString imageType, QWidget *parent):
//    DLabel(parent)
//{
//    _index = index;
//    _path = path;
////    QImage image(path,imageType);
//    qDebug() << index << path;
//    if (COMMON_STR_TRASH == imageType) {
//        _pixmap = dApp->m_imagetrashmap.value(path);
//    } else {
//        _pixmap = dApp->m_imagemap.value(path);
//    }

//    _image = new DLabel(this);
//}

void ImageItem::setIndexNow(int i)
{
    _indexNow = i;
}

void ImageItem::setPic(QPixmap pixmap)
{
    bool bLight = DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType;
    m_bPicNotSuppOrDamaged = pixmap.isNull();
    _pixmap = m_bPicNotSuppOrDamaged
              ? utils::image::getDamagePixmap(bLight)
              : pixmap;
    update();
}

void ImageItem::mouseReleaseEvent(QMouseEvent *ev)
{
    Q_UNUSED(ev);
    bmouserelease = true;
}

void ImageItem::mousePressEvent(QMouseEvent *ev)
{
    Q_UNUSED(ev);
    bmouserelease = false;
    QEventLoop loop;
    QTimer::singleShot(200, &loop, SLOT(quit()));
    loop.exec();
    if (bmouserelease)
        emit imageItemclicked(_index, _indexNow);
}

void ImageItem::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

//        painter.drawPixmap(rect(),QPixmap(_path).scaled(60,50));

    painter.setRenderHints(QPainter::HighQualityAntialiasing |
                           QPainter::SmoothPixmapTransform |
                           QPainter::Antialiasing);

    QRect backgroundRect = rect();
    QRect pixmapRect;
    QBrush  backbrush;
    if (_index == _indexNow) {
        QPainterPath backgroundBp;
        backgroundBp.addRoundedRect(backgroundRect, 8, 8);
        painter.setClipPath(backgroundBp);

        backgroundRect.setX(backgroundRect.x() + 1);
        backgroundRect.setWidth(backgroundRect.width() - 1);
        painter.fillRect(backgroundRect, QBrush(DGuiApplicationHelper::instance()->applicationPalette().highlight().color()));
//LMH解决选中竖图，大小改变0509,屏蔽掉修改图片形状
//        if (_pixmap.width() > _pixmap.height()) {
//            _pixmap = _pixmap.copy((_pixmap.width() - _pixmap.height()) / 2, 0, _pixmap.height(), _pixmap.height());
//        } else if (_pixmap.width() < _pixmap.height()) {
//            _pixmap = _pixmap.copy(0, (_pixmap.height() - _pixmap.width()) / 2, _pixmap.width(), _pixmap.width());
//        }

        m_pixmapstring = "";
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::DarkType) {
            m_pixmapstring = LOCMAP_SELECTED_DARK;
            backbrush = QBrush(utils::common::DARK_BACKGROUND_COLOR);
        } else {
            m_pixmapstring = LOCMAP_SELECTED_LIGHT;
            backbrush = QBrush(utils::common::LIGHT_BACKGROUND_COLOR);
        }

        //绘制默认选中背景
        QRect backRect(backgroundRect.x() + 4, backgroundRect.y() + 4, backgroundRect.width() - 8, backgroundRect.height() - 8);
        QPainterPath backBp;
        backBp.addRoundedRect(backRect, 4, 4);
        painter.setClipPath(backBp);
        painter.fillRect(backRect, backbrush);

        QPainterPath bg;
        bg.addRoundedRect(pixmapRect, 4, 4);
        if (!_pixmap.isNull()) {
            pixmapRect.setX(backgroundRect.x() + 4);
            pixmapRect.setY(backgroundRect.y() + 4);
            pixmapRect.setWidth(backgroundRect.width() - 8);
            pixmapRect.setHeight(backgroundRect.height() - 8);
            bg.addRoundedRect(pixmapRect, 4, 4);
            painter.setClipPath(bg);
        }
    } else {
        pixmapRect.setX(backgroundRect.x() + 1);
        pixmapRect.setY(backgroundRect.y() + 0);
        pixmapRect.setWidth(backgroundRect.width() - 2);
        pixmapRect.setHeight(backgroundRect.height() - 0);

        m_pixmapstring = "";
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::DarkType) {
            m_pixmapstring = LOCMAP_NOT_SELECTED_DARK;
        } else {
            m_pixmapstring = LOCMAP_NOT_SELECTED_LIGHT;
        }

        QPixmap pixmap = utils::base::renderSVG(m_pixmapstring, QSize(32, 40));
        QPainterPath bg;
        bg.addRoundedRect(pixmapRect, 4, 4);
        painter.setClipPath(bg);
    }



    QPainterPath bp1;
//    bp1.addRoundedRect(pixmapRect, 4, 4);
//    bp1.addRoundedRect(pixmapRect, 4, 4);
    bp1.addRoundedRect(pixmapRect, 4, 4);
//    int width = pixmapRect.width();
//    int height = pixmapRect.height();
//    pixmapRect.setX(pixmapRect.x() - 1);
//    pixmapRect.setY(pixmapRect.y() - 1);
//    pixmapRect.setWidth(pixmapRect.width() - 2);
//    pixmapRect.setHeight(pixmapRect.height() - 2);
    painter.setClipPath(bp1);
    painter.drawPixmap(pixmapRect, _pixmap);

}

int ImageItem::indexNow() const
{
    return _indexNow;
}

void ImageItem::updateDmgIconByTheme()
{
    if (!m_bPicNotSuppOrDamaged)
        return;
    setPic(QPixmap());
}

int ImageItem::index() const
{
    return _index;
}

void ImageItem::setIndex(int index)
{
    _index = index;
}


bool ImageItem::index_1(int index)
{
    if (_index > index) {
        _index--;
        return true;
    }
    return false;
}

TTBContent::TTBContent(bool inDB, QStringList filelist, QWidget *parent) : QLabel(parent)
{
    onThemeChanged(dApp->viewerTheme->getCurrentTheme());
    m_windowWidth = std::max(this->window()->width(),
                             ConfigSetter::instance()->value("MAINWINDOW", "WindowWidth").toInt());
//    m_contentWidth = std::max(m_windowWidth - RIGHT_TITLEBAR_WIDTH, 1);
    m_allfileslist = filelist;
    m_filelist_size = m_allfileslist.size();
    if (m_allfileslist.size() <= 1) {
        m_contentWidth = TOOLBAR_JUSTONE_WIDTH;
    } else if (m_allfileslist.size() <= 3) {
        m_contentWidth = TOOLBAR_MINIMUN_WIDTH;
    } else {
        m_contentWidth = qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_allfileslist.size() - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) + THUMBNAIL_LIST_ADJUST;
    }

//    qDebug()<<"init:m_contentWidth=============="<<m_contentWidth;
    setFixedWidth(m_contentWidth);
    setFixedHeight(72);
    QHBoxLayout *hb = new QHBoxLayout(this);
    hb->setContentsMargins(LEFT_MARGIN, 0, LEFT_MARGIN, 3);
    hb->setSpacing(0);
    m_inDB = inDB;
#ifndef LITE_DIV
    m_returnBtn = new ReturnButton();
    m_returnBtn->setMaxWidth(RETURN_BTN_MAX);
    m_returnBtn->setMaximumWidth(RETURN_BTN_MAX);
    m_returnBtn->setObjectName("ReturnBtn");
    m_returnBtn->setToolTip(tr("Back"));

    m_folderBtn = new PushButton();
    m_folderBtn->setFixedSize(QSize(24, 24));
    m_folderBtn->setObjectName("FolderBtn");
    m_folderBtn->setToolTip("Image management");
    if (m_inDB) {
        hb->addWidget(m_returnBtn);
    } else {
        hb->addWidget(m_folderBtn);
    }
    hb->addSpacing(20);

    connect(m_returnBtn, &ReturnButton::clicked, this, [ = ] {
        emit clicked();
    });
    connect(m_folderBtn, &PushButton::clicked, this, [ = ] {
        emit clicked();
    });
    connect(m_returnBtn, &ReturnButton::returnBtnWidthChanged, this, [ = ] {
        updateFilenameLayout();
    });
#endif
    // Adapt buttons////////////////////////////////////////////////////////////
    m_backButton = new DIconButton(this);
    m_backButton->setFixedSize(ICON_SIZE);
    m_backButton->setObjectName("ReturnBtn");
    m_backButton->setIcon(QIcon::fromTheme("dcc_back"));
    m_backButton->setIconSize(QSize(36, 36));
    m_backButton->setToolTip(tr("Back"));

    hb->addWidget(m_backButton);
    hb->addStretch();
    //hb->addSpacing(ICON_SPACING * 5);
    connect(m_backButton, &DIconButton::clicked, this, [ = ] {
        emit dApp->signalM->hideImageView();
        emit dApp->signalM->sigPauseOrStart(false); //唤醒后台外设线程
        emit ttbcontentClicked();
    });

    // preButton
    m_preButton = new DIconButton(this);
    m_preButton->setObjectName("PreviousButton");
    m_preButton->setFixedSize(ICON_SIZE);
    m_preButton->setIcon(QIcon::fromTheme("dcc_previous"));
    m_preButton->setIconSize(QSize(36, 36));
    m_preButton->setToolTip(tr("Previous"));

    m_preButton->hide();

    connect(m_preButton, &DIconButton::clicked, this, [ = ] {
        emit showPrevious();
        emit ttbcontentClicked();
    });

    m_preButton_spc = new DWidget;
    m_preButton_spc->setFixedSize(QSize(10, 50));
    m_preButton_spc->hide();

    // nextButton
    m_nextButton = new DIconButton(this);
    m_nextButton->setObjectName("NextButton");
    m_nextButton->setFixedSize(ICON_SIZE);
    m_nextButton->setIcon(QIcon::fromTheme("dcc_next"));
    m_nextButton->setIconSize(QSize(36, 36));
    m_nextButton->setToolTip(tr("Next"));


    m_nextButton->hide();

    connect(parent, SIGNAL(sigResize()), this, SLOT(onResize()));


    connect(m_nextButton, &DIconButton::clicked, this, [ = ] {
        emit showNext();
        emit ttbcontentClicked();
    });


    m_nextButton_spc = new DWidget;
    m_nextButton_spc->setFixedSize(QSize(10, 50));
    m_nextButton_spc->hide();

    hb->addWidget(m_preButton);
    hb->addWidget(m_preButton_spc);
    hb->addWidget(m_nextButton);
    hb->addWidget(m_nextButton_spc);
    //hb->addSpacing(ICON_SPACING);

    // adaptImageBtn
    m_adaptImageBtn = new DIconButton(this);
    m_adaptImageBtn->setObjectName("AdaptBtn");
    m_adaptImageBtn->setFixedSize(ICON_SIZE);
    m_adaptImageBtn->setIcon(QIcon::fromTheme("dcc_11"));
    m_adaptImageBtn->setIconSize(QSize(36, 36));
    m_adaptImageBtn->setToolTip(tr("1:1 Size"));
    m_adaptImageBtn->setCheckable(true);


    hb->addWidget(m_adaptImageBtn);
    hb->addSpacing(ICON_SPACING);
    connect(m_adaptImageBtn, &DIconButton::clicked, this, [ = ] {
        m_adaptImageBtn->setChecked(true);
        if (!badaptImageBtnChecked)
        {
            badaptImageBtnChecked = true;
            emit resetTransform(false);
            emit ttbcontentClicked();
        }
    });


    // adaptScreenBtn
    m_adaptScreenBtn = new DIconButton(this);
    m_adaptScreenBtn->setFixedSize(ICON_SIZE);
    m_adaptScreenBtn->setObjectName("AdaptScreenBtn");
    m_adaptScreenBtn->setIcon(QIcon::fromTheme("dcc_fit"));
    m_adaptScreenBtn->setIconSize(QSize(36, 36));
    m_adaptScreenBtn->setToolTip(tr("Fit to window"));
//    m_adaptScreenBtn->setCheckable(true);


    hb->addWidget(m_adaptScreenBtn);
    hb->addSpacing(ICON_SPACING);
    connect(m_adaptScreenBtn, &DIconButton::clicked, this, [ = ] {
        m_adaptScreenBtn->setChecked(true);
        if (!badaptScreenBtnChecked)
        {
            badaptScreenBtnChecked = true;
            emit resetTransform(true);
            emit ttbcontentClicked();
        }
    });

    // Collection button////////////////////////////////////////////////////////
    m_clBT = new DIconButton(this);
    m_clBT->setFixedSize(ICON_SIZE);
    m_clBT->setObjectName("CollectBtn");

    connect(m_clBT, &DIconButton::clicked, this, [ = ] {

        if (true == m_bClBTChecked)
        {
            DBManager::instance()->removeFromAlbum(COMMON_STR_FAVORITES, QStringList(m_currentpath));
        } else
        {
            DBManager::instance()->insertIntoAlbum(COMMON_STR_FAVORITES, QStringList(m_currentpath));
        }

        emit ttbcontentClicked();
    });

    hb->addWidget(m_clBT);
    hb->addSpacing(ICON_SPACING);

    // rotateLBtn
    m_rotateLBtn = new DIconButton(this);
    m_rotateLBtn->setFixedSize(ICON_SIZE);
    m_rotateLBtn->setIcon(QIcon::fromTheme("dcc_left"));
    m_rotateLBtn->setIconSize(QSize(36, 36));
    m_rotateLBtn->setToolTip(tr("Rotate counterclockwise"));
    hb->addWidget(m_rotateLBtn);
    hb->addSpacing(ICON_SPACING);
    connect(m_rotateLBtn, &DIconButton::clicked, this, [ = ] {
        emit rotateCounterClockwise();
        emit ttbcontentClicked();
    });

    // rotateRBtn
    m_rotateRBtn = new DIconButton(this);
    m_rotateRBtn->setFixedSize(ICON_SIZE);
    m_rotateRBtn->setIcon(QIcon::fromTheme("dcc_right"));
    m_rotateRBtn->setIconSize(QSize(36, 36));
    m_rotateRBtn->setToolTip(tr("Rotate clockwise"));

    hb->addWidget(m_rotateRBtn);
    hb->addSpacing(ICON_SPACING + 8);
    connect(m_rotateRBtn, &DIconButton::clicked, this, [ = ] {
        emit rotateClockwise();
        emit ttbcontentClicked();
    });

    // imgListView
//    m_imgListView = new DWidget();
    m_imgListView = new MyImageListWidget(this);
    m_imgList = new DWidget(m_imgListView);

    connect(dApp->signalM, &SignalManager::hideImageView, this, [ = ] {
        m_imgList->hide();
//        QLayoutItem *child;
//        while ((child = m_imglayout->takeAt(0)) != 0)
//        {
//            m_imglayout->removeWidget(child->widget());
//            child->widget()->setParent(0);
//            delete child;
//        }
    });
    connect(m_imgListView, &MyImageListWidget::silmoved, this, [ = ] {
        binsertneedupdate = false;
    });
    connect(m_imgListView, &MyImageListWidget::needContinueRequest, this, [ = ] {
        binsertneedupdate = false;
        if (m_requestCount > 0)
        {
            bneedloadimage = true;
        } else
        {
            requestSomeImages();
        }
    });
    connect(m_imgListView, &MyImageListWidget::mouseLeftReleased, this, [ = ] {
        int movex = m_imgList->x();
        if (movex >= 0)
        {
            if (m_imgList->width() < m_imgListView->width()) {
                if (m_imgList->width() + movex > m_imgListView->width()) {
                    movex = (m_imgListView->width() - m_imgList->width()) / 2;
                } else {
                    return;
                }
            } else {
                movex = 0;
            }
        } else
        {
            if (m_imgList->width() < m_imgListView->width()) {
                movex = (m_imgListView->width() - m_imgList->width()) / 2;
            } else {
                if (movex <  m_imgListView->width() - m_imgList->width()) {
                    movex =  m_imgListView->width() - m_imgList->width();
                }
            }
        }
        m_imgList->move(movex, m_imgList->y());
    });
    m_imgListView->setObj(m_imgList);
    m_imgList->installEventFilter(m_imgListView);
    if (m_allfileslist.size() <= 3) {
        m_imgList->setFixedSize(QSize(TOOLBAR_DVALUE, TOOLBAR_HEIGHT));
    } else {
        m_imgList->setFixedSize(QSize(qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_allfileslist.size() - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) - THUMBNAIL_VIEW_DVALUE + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT));
    }
//    qDebug()<<"init:m_imgList.width=============="<<m_imgList->width();

    m_imgList->setDisabled(false);
//    m_imgList->setHidden(true);
    m_imglayout = new QHBoxLayout();
    m_imglayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_imglayout->setMargin(0);
    m_imglayout->setSpacing(0);
    m_imgList->setLayout(m_imglayout);

    if (m_allfileslist.size() <= 3) {
        m_imgListView->setFixedSize(QSize(TOOLBAR_DVALUE, TOOLBAR_HEIGHT));
    } else {
        m_imgListView->setFixedSize(QSize(qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_allfileslist.size() - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) - THUMBNAIL_VIEW_DVALUE + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT));
    }
//    qDebug()<<"init:m_imgListView.width=============="<<m_imgListView->width();
//    m_imgListView->hide();
    QPalette palette ;
    palette.setColor(QPalette::Background, QColor(0, 0, 0, 0)); // 最后一项为透明度
    m_imgList->setPalette(palette);
    m_imgListView->setPalette(palette);
    hb->addWidget(m_imgListView);
    hb->addSpacing(ICON_SPACING + 14);

    m_trashBtn = new DIconButton(this);
    m_trashBtn->setFixedSize(ICON_SIZE);
    m_trashBtn->setObjectName("TrashBtn");
    m_trashBtn->setIcon(QIcon::fromTheme("dcc_delete"));
    m_trashBtn->setIconSize(QSize(36, 36));
    m_trashBtn->setToolTip(tr("Delete"));

    hb->addWidget(m_trashBtn);
    hb->addSpacing(10);
    m_fileNameLabel = new ElidedLabel();
//    hb->addWidget(m_fileNameLabel);


//    connect(SignalManager::instance(), &SignalManager::deleteByMenu, this, &TTBContent::deleteImage);


//    connect(m_trashBtn, &DIconButton::clicked, this, &TTBContent::deleteImage);
    connect(m_trashBtn, &DIconButton::clicked, SignalManager::instance(), &SignalManager::deleteByMenu);
//    m_allfileslist << filelist;
    m_filesbeleft << filelist;
    m_allNeedRequestFilesCount += filelist.size();

    connect(this, &TTBContent::sigRequestSomeImages, this, [ = ] {
        requestSomeImages();
    });

    //当图片载入完毕，更新底部状态栏  2020/04/08 xiaolong
    connect(dApp->signalM, &SignalManager::sigUpdateTTB, this, &TTBContent::updateScreen);

}

//TTBContent::TTBContent(bool inDB,
//                       DBImgInfoList m_infos,
//                       QWidget *parent) : QLabel(parent)
//{
//    onThemeChanged(dApp->viewerTheme->getCurrentTheme());
//    m_windowWidth = std::max(this->window()->width(),
//                             ConfigSetter::instance()->value("MAINWINDOW", "WindowWidth").toInt());
////    m_contentWidth = std::max(m_windowWidth - RIGHT_TITLEBAR_WIDTH, 1);
//    m_imgInfos = m_infos;
//    m_filelist_size = m_imgInfos.size();
//    if (m_imgInfos.size() <= 1) {
//        m_contentWidth = TOOLBAR_JUSTONE_WIDTH;
//    } else if (m_imgInfos.size() <= 3) {
//        m_contentWidth = TOOLBAR_MINIMUN_WIDTH;
//    } else {
//        m_contentWidth = qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_imgInfos.size() - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) + THUMBNAIL_LIST_ADJUST;
//    }

////    qDebug()<<"init:m_contentWidth=============="<<m_contentWidth;
//    setFixedWidth(m_contentWidth);
//    setFixedHeight(72);
//    QHBoxLayout *hb = new QHBoxLayout(this);
//    hb->setContentsMargins(LEFT_MARGIN, 0, LEFT_MARGIN, 3);
//    hb->setSpacing(0);
//    m_inDB = inDB;
//#ifndef LITE_DIV
//    m_returnBtn = new ReturnButton();
//    m_returnBtn->setMaxWidth(RETURN_BTN_MAX);
//    m_returnBtn->setMaximumWidth(RETURN_BTN_MAX);
//    m_returnBtn->setObjectName("ReturnBtn");
//    m_returnBtn->setToolTip(tr("Back"));

//    m_folderBtn = new PushButton();
//    m_folderBtn->setFixedSize(QSize(24, 24));
//    m_folderBtn->setObjectName("FolderBtn");
//    m_folderBtn->setToolTip("Image management");
//    if (m_inDB) {
//        hb->addWidget(m_returnBtn);
//    } else {
//        hb->addWidget(m_folderBtn);
//    }
//    hb->addSpacing(20);

//    connect(m_returnBtn, &ReturnButton::clicked, this, [ = ] {
//        emit clicked();
//    });
//    connect(m_folderBtn, &PushButton::clicked, this, [ = ] {
//        emit clicked();
//    });
//    connect(m_returnBtn, &ReturnButton::returnBtnWidthChanged, this, [ = ] {
//        updateFilenameLayout();
//    });
//#endif
//    // Adapt buttons////////////////////////////////////////////////////////////
//    m_backButton = new DIconButton(this);
//    m_backButton->setFixedSize(ICON_SIZE);
//    m_backButton->setObjectName("ReturnBtn");
//    m_backButton->setIcon(QIcon::fromTheme("dcc_back"));
//    m_backButton->setIconSize(QSize(36, 36));
//    m_backButton->setToolTip(tr("Back"));

//    hb->addWidget(m_backButton);
//    hb->addSpacing(ICON_SPACING * 5);
//    connect(m_backButton, &DIconButton::clicked, this, [ = ] {
//        emit dApp->signalM->hideImageView();
//        emit ttbcontentClicked();
//    });

//    // preButton
//    m_preButton = new DIconButton(this);
//    m_preButton->setObjectName("PreviousButton");
//    m_preButton->setFixedSize(ICON_SIZE);
//    m_preButton->setIcon(QIcon::fromTheme("dcc_previous"));
//    m_preButton->setIconSize(QSize(36, 36));
//    m_preButton->setToolTip(tr("Previous"));

//    m_preButton->hide();

//    connect(m_preButton, &DIconButton::clicked, this, [ = ] {
//        emit showPrevious();
//        emit ttbcontentClicked();
//    });

//    m_preButton_spc = new DWidget;
//    m_preButton_spc->setFixedSize(QSize(10, 50));
//    m_preButton_spc->hide();

//    // nextButton
//    m_nextButton = new DIconButton(this);
//    m_nextButton->setObjectName("NextButton");
//    m_nextButton->setFixedSize(ICON_SIZE);
//    m_nextButton->setIcon(QIcon::fromTheme("dcc_next"));
//    m_nextButton->setIconSize(QSize(36, 36));
//    m_nextButton->setToolTip(tr("Next"));


//    m_nextButton->hide();

//    connect(parent, SIGNAL(sigResize()), this, SLOT(onResize()));


//    connect(m_nextButton, &DIconButton::clicked, this, [ = ] {
//        emit showNext();
//        emit ttbcontentClicked();
//    });


//    m_nextButton_spc = new DWidget;
//    m_nextButton_spc->setFixedSize(QSize(10, 50));
//    m_nextButton_spc->hide();

//    hb->addWidget(m_preButton);
//    hb->addWidget(m_preButton_spc);
//    hb->addWidget(m_nextButton);
//    hb->addWidget(m_nextButton_spc);
//    hb->addSpacing(ICON_SPACING);

//    // adaptImageBtn
//    m_adaptImageBtn = new DIconButton(this);
//    m_adaptImageBtn->setObjectName("AdaptBtn");
//    m_adaptImageBtn->setFixedSize(ICON_SIZE);
//    m_adaptImageBtn->setIcon(QIcon::fromTheme("dcc_11"));
//    m_adaptImageBtn->setIconSize(QSize(36, 36));
//    m_adaptImageBtn->setToolTip(tr("1:1 Size"));
//    m_adaptImageBtn->setCheckable(true);


//    hb->addWidget(m_adaptImageBtn);
//    hb->addSpacing(ICON_SPACING);
//    connect(m_adaptImageBtn, &DIconButton::clicked, this, [ = ] {
//        m_adaptImageBtn->setChecked(true);
//        if (!badaptImageBtnChecked)
//        {
//            badaptImageBtnChecked = true;
//            emit resetTransform(false);
//            emit ttbcontentClicked();
//        }
//    });


//    // adaptScreenBtn
//    m_adaptScreenBtn = new DIconButton(this);
//    m_adaptScreenBtn->setFixedSize(ICON_SIZE);
//    m_adaptScreenBtn->setObjectName("AdaptScreenBtn");
//    m_adaptScreenBtn->setIcon(QIcon::fromTheme("dcc_fit"));
//    m_adaptScreenBtn->setIconSize(QSize(36, 36));
//    m_adaptScreenBtn->setToolTip(tr("Fit to window"));
////    m_adaptScreenBtn->setCheckable(true);


//    hb->addWidget(m_adaptScreenBtn);
//    hb->addSpacing(ICON_SPACING);
//    connect(m_adaptScreenBtn, &DIconButton::clicked, this, [ = ] {
//        m_adaptScreenBtn->setChecked(true);
//        if (!badaptScreenBtnChecked)
//        {
//            badaptScreenBtnChecked = true;
//            emit resetTransform(true);
//            emit ttbcontentClicked();
//        }
//    });

//    // Collection button////////////////////////////////////////////////////////
//    m_clBT = new DIconButton(this);
//    m_clBT->setFixedSize(ICON_SIZE);
//    m_clBT->setObjectName("CollectBtn");

//    connect(m_clBT, &DIconButton::clicked, this, [ = ] {

//        if (true == m_bClBTChecked)
//        {
//            DBManager::instance()->removeFromAlbum(COMMON_STR_FAVORITES, QStringList(m_imagePath));
//        } else
//        {
//            DBManager::instance()->insertIntoAlbum(COMMON_STR_FAVORITES, QStringList(m_imagePath));
//        }

//        emit ttbcontentClicked();
//    });

//    hb->addWidget(m_clBT);
//    hb->addSpacing(ICON_SPACING);

//    // rotateLBtn
//    m_rotateLBtn = new DIconButton(this);
//    m_rotateLBtn->setFixedSize(ICON_SIZE);
//    m_rotateLBtn->setIcon(QIcon::fromTheme("dcc_left"));
//    m_rotateLBtn->setIconSize(QSize(36, 36));
//    m_rotateLBtn->setToolTip(tr("Rotate counterclockwise"));
//    hb->addWidget(m_rotateLBtn);
//    hb->addSpacing(ICON_SPACING);
//    connect(m_rotateLBtn, &DIconButton::clicked, this, [ = ] {
//        emit rotateCounterClockwise();
//        emit ttbcontentClicked();
//    });

//    // rotateRBtn
//    m_rotateRBtn = new DIconButton(this);
//    m_rotateRBtn->setFixedSize(ICON_SIZE);
//    m_rotateRBtn->setIcon(QIcon::fromTheme("dcc_right"));
//    m_rotateRBtn->setIconSize(QSize(36, 36));
//    m_rotateRBtn->setToolTip(tr("Rotate clockwise"));

//    hb->addWidget(m_rotateRBtn);
//    hb->addSpacing(ICON_SPACING + 8);
//    connect(m_rotateRBtn, &DIconButton::clicked, this, [ = ] {
//        emit rotateClockwise();
//        emit ttbcontentClicked();
//    });

//    // imgListView
////    m_imgListView = new DWidget();
//    m_imgListView = new MyImageListWidget();
//    m_imgList = new DWidget(m_imgListView);

//    connect(m_imgListView, &MyImageListWidget::needContinueRequest, this, [ = ] {
//        if (m_requestCount > 0)
//        {
//            bneedloadimage = true;
//        } else
//        {
//            requestSomeImages();
//        }
//    });
//    connect(m_imgListView, &MyImageListWidget::mouseLeftReleased, this, [ = ] {
//        int movex = m_imgList->x();
//        if (movex >= 0)
//        {
//            if (m_imgList->width() < m_imgListView->width()) {
//                if (m_imgList->width() + movex > m_imgListView->width()) {
//                    movex = (m_imgListView->width() - m_imgList->width()) / 2;
//                } else {
//                    return;
//                }
//            } else {
//                movex = 0;
//            }
//        } else
//        {
//            if (m_imgList->width() < m_imgListView->width()) {
//                movex = (m_imgListView->width() - m_imgList->width()) / 2;
//            } else {
//                if (movex <  m_imgListView->width() - m_imgList->width()) {
//                    movex =  m_imgListView->width() - m_imgList->width();
//                }
//            }
//        }
//        m_imgList->move(movex, m_imgList->y());
//    });
//    m_imgListView->setObj(m_imgList);
//    m_imgList->installEventFilter(m_imgListView);
//    if (m_imgInfos.size() <= 3) {
//        m_imgList->setFixedSize(QSize(TOOLBAR_DVALUE, TOOLBAR_HEIGHT));
//    } else {
//        m_imgList->setFixedSize(QSize(qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_imgInfos.size() - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) - THUMBNAIL_VIEW_DVALUE + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT));
//    }
////    qDebug()<<"init:m_imgList.width=============="<<m_imgList->width();

//    m_imgList->setDisabled(false);
////    m_imgList->setHidden(true);
//    m_imglayout = new QHBoxLayout();
//    m_imglayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
//    m_imglayout->setMargin(0);
//    m_imglayout->setSpacing(0);
//    m_imgList->setLayout(m_imglayout);

//    if (m_imgInfos.size() <= 3) {
//        m_imgListView->setFixedSize(QSize(TOOLBAR_DVALUE, TOOLBAR_HEIGHT));
//    } else {
//        m_imgListView->setFixedSize(QSize(qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_imgInfos.size() - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) - THUMBNAIL_VIEW_DVALUE + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT));
//    }
////    qDebug()<<"init:m_imgListView.width=============="<<m_imgListView->width();
////    m_imgListView->hide();
//    QPalette palette ;
//    palette.setColor(QPalette::Background, QColor(0, 0, 0, 0)); // 最后一项为透明度
//    m_imgList->setPalette(palette);
//    m_imgListView->setPalette(palette);
//    hb->addWidget(m_imgListView);
//    hb->addSpacing(ICON_SPACING + 14);

//    m_trashBtn = new DIconButton(this);
//    m_trashBtn->setFixedSize(ICON_SIZE);
//    m_trashBtn->setObjectName("TrashBtn");
//    m_trashBtn->setIcon(QIcon::fromTheme("dcc_delete"));
//    m_trashBtn->setIconSize(QSize(36, 36));
//    m_trashBtn->setToolTip(tr("Delete"));

//    hb->addWidget(m_trashBtn);

//    m_fileNameLabel = new ElidedLabel();
////    hb->addWidget(m_fileNameLabel);


//    connect(SignalManager::instance(), &SignalManager::deleteByMenu, this, &TTBContent::deleteImage);


//    connect(m_trashBtn, &DIconButton::clicked, this, &TTBContent::deleteImage);


////    connect(dApp->viewerTheme, &ViewerThemeManager::viewerThemeChanged, this,
////            &TTBContent::onThemeChanged);

////    connect(dApp, &Application::sigFinishLoad, this, [ = ] {
////        if (isVisible())
////        {
////            QList<ImageItem *> labelList = m_imgList->findChildren<ImageItem *>();
////            for (int i = 0; i < labelList.length(); i++) {
////                labelList.at(i)->setPic(dApp->m_imagemap.value(labelList.at(i)->_path));
////            }
////        }
////    });
//}

//void TTBContent::importFilesFromDB(QString name)
//{
//    ImageEngineApi::instance()->importImagesFromDB(m_delegatetype, this, name);
//}

//bool TTBContent::imageFromDBImported(QStringList &filelist)
//{
//    m_allfileslist << filelist;
//    m_filesbeleft << filelist;
//    m_allNeedRequestFilesCount += filelist.size();
//    if (bneedloadimage) {
//        requestSomeImages();
//    }
//    return true;
//}

//void TTBContent::importFilesFromLocal(QStringList files)
//{
//    ImageEngineApi::instance()->importImagesFromLocal(files, this);
//}

//void TTBContent::importFilesFromLocal(DBImgInfoList files)
//{
//    ImageEngineApi::instance()->importImagesFromLocal(files, this);
//}

//bool TTBContent::imageLocalImported(QStringList &filelist)
//{
//    m_allfileslist << filelist;
//    m_filesbeleft << filelist;
//    m_allNeedRequestFilesCount += filelist.size();
//    if (bneedloadimage) {
//        requestSomeImages();
//    }

//    return true;
//}

void TTBContent::requestSomeImages()
{
//    QMutexLocker mutex(&m_mutex);
    bneedloadimage = false;

    if (m_filesbeleft.size() < Number_Of_Displays_Per_Time) {
        m_requestCount += m_filesbeleft.size();
    } else {
        m_requestCount += Number_Of_Displays_Per_Time;
    }
    for (int i = 0; i < Number_Of_Displays_Per_Time; i++) {
        if (m_filesbeleft.size() <= 0) {
            brequestallfiles = true;
            emit dApp->signalM->sigUpdateTTB();
            return;
        }
//        m_requestCount++;
        QString firstfilesbeleft = m_filesbeleft.first();
        m_filesbeleft.removeFirst();
        ImageEngineApi::instance()->reQuestImageData(firstfilesbeleft, this);
    }
//    m_requestCount = Number_Of_Displays_Per_Time;
//    QApplication::processEvents();
}

bool TTBContent::imageLoaded(QString filepath)
{
    m_requestCount--;
    m_allNeedRequestFilesCount--;
    bool reb = false;
    ImageDataSt data;
    if (ImageEngineApi::instance()->getImageData(filepath, data)) {
        insertImageItem(data);
        reb = true;
//        QApplication::processEvents();
    }
    if (m_requestCount < 1
//            && (!bfilefind || bneedloadimage || (m_imgList->width() + m_imgList->x() - m_imgListView->width() - 30) < 0) //注释以后，一次性全部加载
       ) {
        requestSomeImages();
    }
//    QApplication::processEvents();
    return reb;
}

int TTBContent::itemLoadedSize()
{
//    return m_ItemLoaded.size();
    return m_allfileslist.size();
}


QString TTBContent::getIndexPath(int index)
{
//    if (index < 0 || index >= m_ItemLoaded.size()) {
//        return m_currentpath;
//    }
//    QMap<int, QString>::iterator it;
//    it = m_indextopath.find(index);
//    if (it == m_indextopath.end()) {
//        return "";
//    }
//    return it.value();
    return m_allfileslist.at(index);
}

//void TTBContent::updateScreenNoAnimation()
//{
//    if (m_ItemLoaded.size() > 3) {
//        m_imgList->setFixedSize((m_ItemLoaded.size() + 1)*THUMBNAIL_WIDTH, TOOLBAR_HEIGHT);
//        m_imgList->resize((m_ItemLoaded.size() + 1)*THUMBNAIL_WIDTH + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT);

////            qDebug()<<"setImage:m_imgList.width=============="<<m_imgList->width();
////            qDebug()<<"setImage:m_imgListView.width=============="<<m_imgListView->width();

//        m_imgList->setContentsMargins(0, 0, 0, 0);

//        m_imgListView->show();
//        auto num = 32;
//        QList<ImageItem *> labelList = m_imgList->findChildren<ImageItem *>();
//        if (m_nowIndex > -1) {
//            int a = (qCeil(m_imgListView->width() - 26) / 32) / 2;
//            int b = m_ItemLoaded.size() - (qFloor(m_imgListView->width() - 26) / 32) / 2;
////            qDebug() << "a=" << a;
////            qDebug() << "b=" << b;
////            qDebug() << "m_nowIndex=" << m_nowIndex;
////            qDebug() << "m_ItemLoaded.size()=" << m_ItemLoaded.size();
//            if (m_nowIndex > a && m_nowIndex < b) {
//                m_startAnimation = 1;
//            } else if (m_nowIndex < m_ItemLoaded.size() - 2 * a && m_nowIndex > -1) {
//                m_startAnimation = 2;
//            } else if (m_nowIndex > 2 * a - 1 && m_nowIndex < m_ItemLoaded.size()) {
//                m_startAnimation = 3;
//            } else {
//                m_startAnimation = 0;
//            }
////            qDebug() << "m_startAnimation=" << m_startAnimation;
////            for (int j = 0; j < labelList.size(); j++) {
////                labelList.at(j)->setFixedSize(QSize(num, 40));
////                labelList.at(j)->resize(QSize(num, 40));
////                labelList.at(j)->setIndexNow(m_nowIndex );
////            }
//            if (m_nowIndex < labelList.size())
//                labelList.at(m_nowIndex)->setIndexNow(m_nowIndex);
//            if (m_lastIndex > -1) {
//                labelList.at(m_lastIndex)->setFixedSize(QSize(num, 40));
//                labelList.at(m_lastIndex)->resize(QSize(num, 40));
//                labelList.at(m_lastIndex)->setIndexNow(m_nowIndex);
//            }
//            if (labelList.size() > 0) {
//                labelList.at(m_nowIndex)->setFixedSize(QSize(58, 58));
//                labelList.at(m_nowIndex)->resize(QSize(58, 58));
//            }


//            if (binsertneedupdate) {
//                if (1 == m_startAnimation) {
//                    m_imgList->move(QPoint((qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_ItemLoaded.size() - 3)), (qMax(width() - RT_SPACING, TOOLBAR_MINIMUN_WIDTH))) - 496 - 52 + 18) / 2 - ((num) *m_nowIndex), 0));
//                } else if (2 == m_startAnimation) {
//                    m_imgList->move(QPoint(0, 0));
//                } else if (3 == m_startAnimation) {
//                    m_imgList->move(QPoint(m_imgListView->width() - m_imgList->width() + 5, 0));
//                } else if (0 == m_startAnimation) {
//                    m_imgList->show();
//                }
//            }
//            m_imgListView->update();
//            m_imgList->update();
//            m_preButton->show();
//            m_preButton_spc->show();
//            m_nextButton->show();
//            m_nextButton_spc->show();


//            if (m_nowIndex == 0) {
//                m_preButton->setDisabled(true);
//            } else {
//                m_preButton->setDisabled(false);
//            }
//            if (m_nowIndex == labelList.size() - 1) {
//                m_nextButton->setDisabled(true);
//            } else {
//                m_nextButton->setDisabled(false);
//            }
//            m_lastIndex = m_nowIndex;
//        }
//    } else if (m_ItemLoaded.size() > 1) {
//        m_imgList->setFixedSize((m_ItemLoaded.size() + 1)*THUMBNAIL_WIDTH, TOOLBAR_HEIGHT);
//        m_imgList->resize((m_ItemLoaded.size() + 1)*THUMBNAIL_WIDTH, TOOLBAR_HEIGHT);

//        m_imgList->setContentsMargins(0, 0, 0, 0);

//        auto num = 32;

////        int i = 0;
//        m_imgListView->show();
//        if (!binsertneedupdate)
//            return;
//        QList<ImageItem *> labelList = m_imgList->findChildren<ImageItem *>();
//        if (m_nowIndex > -1) {

////            if (COMMON_STR_TRASH == m_imageType) {
////                labelList.at(t)->setPic(dApp->m_imagetrashmap.value(path));
////            } else {
////                labelList.at(t)->setPic(dApp->m_imagemap.value(path));
////            }
////            for (int j = 0; j < labelList.size(); j++) {
////                labelList.at(j)->setFixedSize(QSize(num, 40));
////                labelList.at(j)->resize(QSize(num, 40));
////                labelList.at(j)->setIndexNow(t);
////            }
//            if (m_nowIndex < labelList.size())
//                labelList.at(m_nowIndex)->setIndexNow(m_nowIndex);
//            if (m_lastIndex > -1) {
//                labelList.at(m_lastIndex)->setFixedSize(QSize(num, 40));
//                labelList.at(m_lastIndex)->resize(QSize(num, 40));
//                labelList.at(m_lastIndex)->setIndexNow(m_nowIndex);
//            }
//            if (labelList.size() > 0) {
//                labelList.at(m_nowIndex)->setFixedSize(QSize(58, 58));
//                labelList.at(m_nowIndex)->resize(QSize(58, 58));
//            }

////            m_imgListView->show();
//            m_imgList->show();
//            m_imgListView->update();
//            m_imgList->update();
//            m_preButton->show();
//            m_preButton_spc->show();
//            m_nextButton->show();
//            m_nextButton_spc->show();


//            if (m_nowIndex == 0) {
//                m_preButton->setDisabled(true);
//            } else {
//                m_preButton->setDisabled(false);
//            }
//            if (m_nowIndex == labelList.size() - 1) {
//                m_nextButton->setDisabled(true);
//            } else {
//                m_nextButton->setDisabled(false);
//            }
//            m_lastIndex = m_nowIndex;
//        }
//    } else {
//        m_imgList->hide();
//        m_imgListView->hide();
//        m_preButton->hide();
//        m_preButton_spc->hide();
//        m_nextButton->hide();
//        m_nextButton_spc->hide();
////        m_contentWidth = TOOLBAR_JUSTONE_WIDTH;
////        setFixedWidth(m_contentWidth);
//    }

////    m_windowWidth =  this->window()->geometry().width();
////    if (m_ItemLoaded.size() <= 1) {
////        m_contentWidth = TOOLBAR_JUSTONE_WIDTH;
////    } else if (m_ItemLoaded.size() <= 3) {
////        m_contentWidth = TOOLBAR_MINIMUN_WIDTH;
////        m_imgListView->setFixedSize(QSize(TOOLBAR_DVALUE, TOOLBAR_HEIGHT));
////    } else {
////        m_contentWidth = qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_filelist_size - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) + THUMBNAIL_LIST_ADJUST;
////        m_imgListView->setFixedSize(QSize(qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_filelist_size - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) - THUMBNAIL_VIEW_DVALUE + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT));
////    }
////    setFixedWidth(m_contentWidth);
//}

void TTBContent::updateScreen()
{
    if (m_ItemLoaded.size() > 3) {
        m_imgList->setFixedSize((m_ItemLoaded.size() + 1)*THUMBNAIL_WIDTH, TOOLBAR_HEIGHT);
        m_imgList->resize((m_ItemLoaded.size() + 1)*THUMBNAIL_WIDTH + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT);

//            qDebug()<<"setImage:m_imgList.width=============="<<m_imgList->width();
//            qDebug()<<"setImage:m_imgListView.width=============="<<m_imgListView->width();

        m_imgList->setContentsMargins(0, 0, 0, 0);

        m_imgListView->show();
        auto num = 32;
        QList<ImageItem *> labelList = m_imgList->findChildren<ImageItem *>();
        if (m_nowIndex > -1) {
            int a = (qCeil(m_imgListView->width() - 26) / 32) / 2;
            int b = m_ItemLoaded.size() - (qFloor(m_imgListView->width() - 26) / 32) / 2;
//            qDebug() << "a=" << a;
//            qDebug() << "b=" << b;
//            qDebug() << "m_nowIndex=" << m_nowIndex;
//            qDebug() << "m_ItemLoaded.size()=" << m_ItemLoaded.size();

            if (m_nowIndex > a && m_nowIndex < b) {
                m_startAnimation = 1;
            } else if (m_nowIndex < m_ItemLoaded.size() - 2 * a && m_nowIndex > -1) {
                m_startAnimation = 2;
            } else if (m_nowIndex > 2 * a - 1 && m_nowIndex < m_ItemLoaded.size()) {
                m_startAnimation = 3;
            } else {
                m_startAnimation = 0;
            }
//            qDebug() << "m_startAnimation=" << m_startAnimation;
//            for (int j = 0; j < labelList.size(); j++) {
//                labelList.at(j)->setFixedSize(QSize(num, 40));
//                labelList.at(j)->resize(QSize(num, 40));
//                labelList.at(j)->setIndexNow(m_nowIndex );
//            }

            if (m_nowIndex < labelList.size())
                labelList.at(m_nowIndex)->setIndexNow(m_nowIndex);

            if (m_lastIndex > -1) {
                labelList.at(m_lastIndex)->setFixedSize(QSize(num, 40));
                labelList.at(m_lastIndex)->resize(QSize(num, 40));
                labelList.at(m_lastIndex)->setIndexNow(m_nowIndex);
            }
            if (labelList.size() > 0) {
                labelList.at(m_nowIndex)->setFixedSize(QSize(58, 58));
                labelList.at(m_nowIndex)->resize(QSize(58, 58));
            }


            if (binsertneedupdate) {
                if (1 == m_startAnimation) {
                    if (bresized) {
                        bresized = false;
                        m_imgList->move(QPoint((qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_ItemLoaded.size() - 3)), (qMax(width() - RT_SPACING, TOOLBAR_MINIMUN_WIDTH))) - 496 - 52 + 18) / 2 - ((num) *m_nowIndex), 0));
                    } else {
                        QPropertyAnimation *animation = new QPropertyAnimation(m_imgList, "pos");
                        animation->setDuration(500);
                        animation->setEasingCurve(QEasingCurve::NCurveTypes);
                        animation->setStartValue(m_imgList->pos());
                        animation->setKeyValueAt(1,  QPoint((qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_ItemLoaded.size() - 3)), (qMax(width() - RT_SPACING, TOOLBAR_MINIMUN_WIDTH))) - 496 - 52 + 18) / 2 - ((num)*m_nowIndex), 0));
                        animation->setEndValue(QPoint((qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_ItemLoaded.size() - 3)), (qMax(width() - RT_SPACING, TOOLBAR_MINIMUN_WIDTH))) - 496 - 228/*52*/ + 18) / 2 - ((num)*m_nowIndex), 0));
                        animation->start(QAbstractAnimation::DeleteWhenStopped);
                        connect(animation, &QPropertyAnimation::finished,
                                animation, &QPropertyAnimation::deleteLater);

                        connect(animation, &QPropertyAnimation::finished,
                        this, [ = ] {
                            m_imgList->show();
                        });
                    }
                } else if (2 == m_startAnimation) {
                    if (bresized) {
                        bresized = false;
                        m_imgList->move(QPoint(0, 0));
                    } else {
                        QPropertyAnimation *animation = new QPropertyAnimation(m_imgList, "pos");
                        animation->setDuration(500);
                        animation->setEasingCurve(QEasingCurve::NCurveTypes);
                        animation->setStartValue(m_imgList->pos());
                        animation->setKeyValueAt(1,  QPoint(0, 0));
                        animation->setEndValue(QPoint(0, 0));
                        animation->start(QAbstractAnimation::DeleteWhenStopped);
                        connect(animation, &QPropertyAnimation::finished,
                                animation, &QPropertyAnimation::deleteLater);

                        connect(animation, &QPropertyAnimation::finished,
                        this, [ = ] {
                            m_imgList->show();
                        });
                    }
                } else if (3 == m_startAnimation) {
                    if (bresized) {
                        bresized = false;
                        m_imgList->move(QPoint(m_imgListView->width() - m_imgList->width() + 5, 0));
                    } else {
                        QPropertyAnimation *animation = new QPropertyAnimation(m_imgList, "pos");
                        animation->setDuration(500);
                        animation->setEasingCurve(QEasingCurve::NCurveTypes);
                        animation->setStartValue(m_imgList->pos());
                        animation->setKeyValueAt(1,  QPoint(0, 0));
                        animation->setEndValue(QPoint(m_imgListView->width() - m_imgList->width() + 5, 0));
                        animation->start(QAbstractAnimation::DeleteWhenStopped);
                        connect(animation, &QPropertyAnimation::finished,
                                animation, &QPropertyAnimation::deleteLater);

                        connect(animation, &QPropertyAnimation::finished,
                        this, [ = ] {
                            m_imgList->show();
                        });
                    }
                } else if (0 == m_startAnimation) {
                    m_imgList->show();
                }
            }

            if (m_nowIndex == 0) {
                m_imgList->move(QPoint(0, 0));
            }

            m_imgListView->update();
            m_imgList->update();
            m_preButton->show();
            m_preButton_spc->show();
            m_nextButton->show();
            m_nextButton_spc->show();


            if (m_nowIndex == 0) {
                m_preButton->setDisabled(true);
            } else {
                m_preButton->setDisabled(false);
            }
            if (m_nowIndex == m_allfileslist.size() - 1) {
                m_nextButton->setDisabled(true);
            } else {
                m_nextButton->setDisabled(false);
            }
            m_lastIndex = m_nowIndex;
        }
    } else if (m_ItemLoaded.size() > 1) {
        m_imgList->setFixedSize((m_ItemLoaded.size() + 1)*THUMBNAIL_WIDTH, TOOLBAR_HEIGHT);
        m_imgList->resize((m_ItemLoaded.size() + 1)*THUMBNAIL_WIDTH, TOOLBAR_HEIGHT);

        m_imgList->setContentsMargins(0, 0, 0, 0);

        auto num = 32;

//        int i = 0;
        m_imgListView->show();
        if (!binsertneedupdate)
            return;
        QList<ImageItem *> labelList = m_imgList->findChildren<ImageItem *>();
        if (m_nowIndex > -1) {

//            if (COMMON_STR_TRASH == m_imageType) {
//                labelList.at(t)->setPic(dApp->m_imagetrashmap.value(path));
//            } else {
//                labelList.at(t)->setPic(dApp->m_imagemap.value(path));
//            }
//            for (int j = 0; j < labelList.size(); j++) {
//                labelList.at(j)->setFixedSize(QSize(num, 40));
//                labelList.at(j)->resize(QSize(num, 40));
//                labelList.at(j)->setIndexNow(m_nowIndex);
//            }
            if (m_nowIndex < labelList.size())
                labelList.at(m_nowIndex)->setIndexNow(m_nowIndex);
            if (m_lastIndex > -1) {
                labelList.at(m_lastIndex)->setFixedSize(QSize(num, 40));
                labelList.at(m_lastIndex)->resize(QSize(num, 40));
                labelList.at(m_lastIndex)->setIndexNow(m_nowIndex);
            }
            if (labelList.size() > 0) {
                labelList.at(m_nowIndex)->setFixedSize(QSize(58, 58));
                labelList.at(m_nowIndex)->resize(QSize(58, 58));
            }

//            m_imgListView->show();
            m_imgList->show();
            m_imgListView->update();
            m_imgList->update();
            m_preButton->show();
            m_preButton_spc->show();
            m_nextButton->show();
            m_nextButton_spc->show();


            if (m_nowIndex == 0) {
                m_preButton->setDisabled(true);
            } else {
                m_preButton->setDisabled(false);
            }
            if (m_nowIndex == labelList.size() - 1) {
                m_nextButton->setDisabled(true);
            } else {
                m_nextButton->setDisabled(false);
            }
            m_lastIndex = m_nowIndex;
        }
    } else if (m_ItemLoaded.size() == 1 && m_allfileslist.size() > 1) {
        m_imgList->setFixedSize((m_ItemLoaded.size() + 1)*THUMBNAIL_WIDTH, TOOLBAR_HEIGHT);
        m_imgList->resize((m_ItemLoaded.size() + 1)*THUMBNAIL_WIDTH, TOOLBAR_HEIGHT);

        m_imgList->setContentsMargins(0, 0, 0, 0);

        auto num = 32;

        m_imgListView->show();
        if (!binsertneedupdate)
            return;
        QList<ImageItem *> labelList = m_imgList->findChildren<ImageItem *>();
        if (m_nowIndex > -1) {
            if (m_nowIndex < labelList.size())
                labelList.at(m_nowIndex)->setIndexNow(m_nowIndex);
            if (m_lastIndex > -1) {
                labelList.at(m_lastIndex)->setFixedSize(QSize(num, 40));
                labelList.at(m_lastIndex)->resize(QSize(num, 40));
                labelList.at(m_lastIndex)->setIndexNow(m_nowIndex);
            }
            if (labelList.size() > 0) {
                labelList.at(m_nowIndex)->setFixedSize(QSize(58, 58));
                labelList.at(m_nowIndex)->resize(QSize(58, 58));
            }

//            m_imgListView->show();
            m_imgList->show();
            m_imgListView->update();
            m_imgList->update();
            m_preButton->show();
            m_preButton_spc->show();
            m_nextButton->show();
            m_nextButton_spc->show();


            if (m_nowIndex == 0) {
                m_preButton->setDisabled(true);
            } else {
                m_preButton->setDisabled(false);
            }
            if (m_nowIndex == m_allfileslist.size() - 1) {
                m_nextButton->setDisabled(true);
            } else {
                m_nextButton->setDisabled(false);
            }
            m_lastIndex = m_nowIndex;
        }
    } else {
        m_imgList->hide();
        m_imgListView->hide();
        m_preButton->hide();
        m_preButton_spc->hide();
        m_nextButton->hide();
        m_nextButton_spc->hide();
//        m_contentWidth = TOOLBAR_JUSTONE_WIDTH;
//        setFixedWidth(m_contentWidth);
    }
    m_windowWidth =  this->window()->geometry().width();
    if (m_ItemLoaded.size() <= 1) {
        m_contentWidth = TOOLBAR_JUSTONE_WIDTH;
    } else if (m_ItemLoaded.size() <= 3) {
        m_contentWidth = TOOLBAR_MINIMUN_WIDTH;
        m_imgListView->setFixedSize(QSize(TOOLBAR_DVALUE, TOOLBAR_HEIGHT));
    } else {
        m_contentWidth = qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_filelist_size - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) + THUMBNAIL_LIST_ADJUST;
        m_imgListView->setFixedSize(QSize(qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_filelist_size - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) - THUMBNAIL_VIEW_DVALUE + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT));
    }
    setFixedWidth(m_contentWidth);

}

void TTBContent::insertImageItem(const ImageDataSt file)
{
//    ImageDataSt info = file;
    int index = m_ItemLoaded.size();
    TTBContentData data;
    data.index = index;
    data.data = file;
    m_ItemLoaded.insert(file.dbi.filePath, data);
//    m_indextopath.insert(index, file.dbi.filePath);
    ImageItem *imageItem = new ImageItem(index, file);
    imageItem->setFixedSize(QSize(32, 40));
    imageItem->resize(QSize(32, 40));
    imageItem->installEventFilter(m_imgListView);

    m_imglayout->addWidget(imageItem);
    connect(imageItem, &ImageItem::imageItemclicked, this, [ = ](int index, int indexNow) {
        binsertneedupdate = true;
        m_nowIndex = index;
        bfilefind = true;
        m_currentpath = imageItem->_path;
//        setCurrentItem();
        emit imageClicked(index, (index - indexNow));
        emit ttbcontentClicked();
    });
    if (-1 == m_nowIndex && file.dbi.filePath == m_currentpath) {
        m_nowIndex = index;
        bfilefind = true;
        emit feedBackCurrentIndex(m_nowIndex, m_currentpath);
        setCurrentItem();
    } else {
//        if (binsertneedupdate)
//        updateScreen();
        onResize();
//        updateScreenNoAnimation();
    }
//    onResize();
}

void TTBContent::reLoad()
{
    clearAndStopThread();

    m_ItemLoaded.clear();
//    m_indextopath.clear();
    m_filesbeleft.clear();
    m_filesbeleft << m_allfileslist;
    m_requestCount = 0;

    if (m_allfileslist.size() <= 3) {
        m_imgList->setFixedSize(QSize(TOOLBAR_DVALUE, TOOLBAR_HEIGHT));
        m_imgListView->setFixedSize(QSize(TOOLBAR_DVALUE, TOOLBAR_HEIGHT));
    } else {
        m_imgList->setFixedSize(QSize(qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_allfileslist.size() - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) - THUMBNAIL_VIEW_DVALUE + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT));
        m_imgListView->setFixedSize(QSize(qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_allfileslist.size() - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) - THUMBNAIL_VIEW_DVALUE + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT));
    }
    m_allNeedRequestFilesCount = m_allfileslist.size();
    QLayoutItem *child;
    while ((child = m_imglayout->takeAt(0)) != nullptr) {
        m_imglayout->removeWidget(child->widget());
        child->widget()->setParent(nullptr);
        delete child;

    }
    if (m_filesbeleft.size() > 0) {
        requestSomeImages();
    }
}

void TTBContent::stopLoadAndClear()
{
    clearAndStopThread();
    m_allfileslist.clear();

    QLayoutItem *child;
    while ((child = m_imglayout->takeAt(0)) != nullptr) {
        m_imglayout->removeWidget(child->widget());
        child->widget()->setParent(nullptr);
        delete child;

    }
    m_filesbeleft.clear();
    m_allNeedRequestFilesCount = 0;
}

QStringList TTBContent::getAllFileList()
{
    return m_allfileslist;
}

void TTBContent::disCheckAdaptImageBtn()
{
    m_adaptImageBtn->setChecked(false);
    badaptImageBtnChecked = false;
}
void TTBContent::disCheckAdaptScreenBtn()
{
    m_adaptScreenBtn->setChecked(false);
    badaptScreenBtnChecked = false;
}

void TTBContent::checkAdaptImageBtn()
{
    m_adaptImageBtn->setChecked(true);
    badaptImageBtnChecked = true;
}
void TTBContent::checkAdaptScreenBtn()
{
    m_adaptScreenBtn->setChecked(true);
    badaptScreenBtnChecked = true;
}

void TTBContent::deleteImage()
{
    m_ItemLoaded.remove(m_currentpath);
    m_allfileslist.removeAt(m_nowIndex);
    m_allNeedRequestFilesCount = m_allfileslist.size();



    QList<ImageItem *> labelList = m_imgList->findChildren<ImageItem *>();

    ImageItem *getim = nullptr;
    for (ImageItem *im : labelList) {
        if (im->index() == m_nowIndex) {
            getim = im;
        } else {
            im->setIndexNow(m_nowIndex);
            if (im->index_1(m_nowIndex)) {
                QMap<QString, TTBContentData>::iterator it;
                it = m_ItemLoaded.find(im->_path);
                if (it != m_ItemLoaded.end()) {
                    TTBContentData data = it.value();
                    data.index--;
                    m_ItemLoaded[im->_path] = data;
                }
            }
        }
    }

    QList<ImageItem *> labelList1 = m_imgList->findChildren<ImageItem *>();
    if (m_allfileslist.size() > 0) {
        if (m_allfileslist.size() > m_nowIndex) {
            m_lastIndex = -1;
            m_currentpath = m_allfileslist[m_nowIndex];
        } else {
            m_nowIndex = 0;
            m_lastIndex = -1;
            m_currentpath = m_allfileslist[0];
        }
//        m_nowIndex = - 1;
//        m_lastIndex = -1;
//        m_currentpath = ""/*m_allfileslist[m_nowIndex]*/;
//        reLoad();
//        updateScreen();
    }
//    if (m_filelist_size < 2) {
//        return;
//    }

    m_filelist_size = m_filelist_size - 1;
    if (nullptr != getim) {
        //updateScreen();
        m_imglayout->removeWidget(getim);
//        getim->hide();
        delete getim;
        getim = nullptr;
        updateScreen();
    }
//    int windowWidth =  this->window()->geometry().width();
////    int windowWidth = 1000;
//    if (m_filelist_size <= 1) {
//        m_contentWidth = TOOLBAR_JUSTONE_WIDTH;
//    } else if (m_filelist_size <= 3) {
//        m_contentWidth = TOOLBAR_MINIMUN_WIDTH;
//        m_imgListView->setFixedSize(QSize(TOOLBAR_DVALUE, TOOLBAR_HEIGHT));
//        m_imgList->setFixedSize(QSize(TOOLBAR_DVALUE, TOOLBAR_HEIGHT));
//    } else {
//        m_contentWidth = qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_filelist_size - 3)), qMax(windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) + THUMBNAIL_LIST_ADJUST;
//        m_imgListView->setFixedSize(QSize(qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_filelist_size - 3)), qMax(windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) - THUMBNAIL_VIEW_DVALUE + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT));
//        m_imgList->setFixedSize(QSize(qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_filelist_size - 3)), qMax(windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) - THUMBNAIL_VIEW_DVALUE + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT));
//    }
//    setFixedWidth(m_contentWidth);
    //2020/3/30 20:34   尚未解决 难复现
    emit ttbcontentClicked();
    emit removed();

    //        qDebug() << "m_trashBtn:m_contentWidth==============" << m_contentWidth;
    //        qDebug() << "m_trashBtn:m_imgListView.width==============" << m_imgListView->width();
}

void TTBContent::updateFilenameLayout()
{
    using namespace utils::base;
    DFontSizeManager::instance()->bind(m_fileNameLabel, DFontSizeManager::T8);
    QFontMetrics fm(DFontSizeManager::instance()->get(DFontSizeManager::T8));
    QString filename = QFileInfo(m_currentpath).fileName();
    QString name;

    int strWidth = fm.boundingRect(filename).width();
    int leftMargin = 0;
    int m_leftContentWidth = 0;
#ifndef LITE_DIV
    if (m_inDB)
        m_leftContentWidth = m_returnBtn->buttonWidth() + 6
                             + (ICON_SIZE.width() + 2) * 6 + LEFT_SPACE;
    else {
        m_leftContentWidth = m_folderBtn->width()  + 8
                             + (ICON_SIZE.width() + 2) * 5 + LEFT_SPACE;
    }
#else
    // 39 为logo以及它的左右margin
    m_leftContentWidth = 5 + (ICON_SIZE.width() + 2) * 5 + 39;
#endif

    int ww = dApp->setter->value("MAINWINDOW",  "WindowWidth").toInt();
    m_windowWidth =  std::max(std::max(this->window()->geometry().width(), this->width()), ww);
    m_contentWidth = std::max(m_windowWidth - RIGHT_TITLEBAR_WIDTH + 2, 1);
    setFixedWidth(m_contentWidth);
    m_contentWidth = this->width() - m_leftContentWidth;

    if (strWidth > m_contentWidth || strWidth > FILENAME_MAX_LENGTH) {
        name = fm.elidedText(filename, Qt::ElideMiddle, std::min(m_contentWidth - 32,
                                                                 FILENAME_MAX_LENGTH));
        strWidth = fm.boundingRect(name).width();
        leftMargin = std::max(0, (m_windowWidth - strWidth) / 2
                              - m_leftContentWidth - LEFT_MARGIN - 2);
    } else {
        leftMargin = std::max(0, (m_windowWidth - strWidth) / 2
                              - m_leftContentWidth - 6);
        name = filename;
    }

    m_fileNameLabel->setText(name, leftMargin);
}

void TTBContent::onThemeChanged(ViewerThemeManager::AppTheme theme)
{
    Q_UNUSED(theme);
}

void TTBContent::setCurrentDir(QString text)
{
    if (text == COMMON_STR_FAVORITES) {
        text = tr(COMMON_STR_FAVORITES);
    }

#ifndef LITE_DIV
    m_returnBtn->setText(text);
#endif
}

void TTBContent::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    m_windowWidth =  this->window()->geometry().width();
    if (m_filelist_size <= 1) {
        m_contentWidth = TOOLBAR_JUSTONE_WIDTH;
    } else if (m_filelist_size <= 3) {
        m_contentWidth = TOOLBAR_MINIMUN_WIDTH;
        m_imgListView->setFixedSize(QSize(TOOLBAR_DVALUE, TOOLBAR_HEIGHT));
    } else {
        m_contentWidth = qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_filelist_size - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) + THUMBNAIL_LIST_ADJUST;
        m_imgListView->setFixedSize(QSize(qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_filelist_size - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) - THUMBNAIL_VIEW_DVALUE + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT));
    }
    setFixedWidth(m_contentWidth);
}

void TTBContent::setImage(const QString &path)
{
    binsertneedupdate = true;
    if (!m_allfileslist.isEmpty() && !QFileInfo(path).exists()) {
        emit dApp->signalM->picNotExists(true);
        if (m_allfileslist.size() == 1)
            return;
    } else {
        emit dApp->signalM->picNotExists(false);
    }
    m_currentpath = path;
    QMap<QString, TTBContentData>::iterator it;
    it = m_ItemLoaded.find(path);
    if (it == m_ItemLoaded.end()) {
        bfilefind = false;
        m_nowIndex = -1;
        m_adaptImageBtn->setDisabled(true);
        m_adaptScreenBtn->setDisabled(true);
        m_rotateLBtn->setDisabled(true);
        m_rotateRBtn->setDisabled(true);
//        m_trashBtn->setDisabled(true);
//        m_imgList->setDisabled(false);
    } else {
        TTBContentData data = it.value();
        m_nowIndex = data.index;
        bfilefind = true;
        QList<ImageItem *> labelList = m_imgList->findChildren<ImageItem *>();
        ImageDataSt gdata;
        if (ImageEngineApi::instance()->getImageData(m_currentpath, gdata))
            labelList.at(m_nowIndex)->setPic(gdata.imgpixmap);
        setCurrentItem();
    }
//    emit dApp->signalM->hideBottomToolbar();
}

bool TTBContent::setCurrentItem()
{
    if ((QFileInfo(m_currentpath).exists()) &&
            (m_currentpath.isEmpty() || !QFileInfo(m_currentpath).exists() || !QFileInfo(m_currentpath).isReadable())) {
        m_adaptImageBtn->setDisabled(true);
        m_adaptScreenBtn->setDisabled(true);
        m_rotateLBtn->setDisabled(true);
        m_rotateRBtn->setDisabled(true);
//        m_trashBtn->setDisabled(true);
        m_imgList->setDisabled(false);
    } else {
        m_adaptImageBtn->setDisabled(false);
        m_adaptScreenBtn->setDisabled(false);

        updateScreen();
        if (QFileInfo(m_currentpath).isReadable() &&
                !QFileInfo(m_currentpath).isWritable()) {
//            m_trashBtn->setDisabled(true);
            m_rotateLBtn->setDisabled(true);
            m_rotateRBtn->setDisabled(true);
        } else {
//            m_trashBtn->setDisabled(false);
            if (utils::image::imageSupportSave(m_currentpath)) {
                m_rotateLBtn->setDisabled(false);
                m_rotateRBtn->setDisabled(false);
            } else {
                m_rotateLBtn->setDisabled(true);
                m_rotateRBtn->setDisabled(true);
            }
        }
    }

    QString fileName = "";
    if (m_currentpath != "") {
        fileName = QFileInfo(m_currentpath).fileName();
    }
    emit dApp->signalM->updateFileName(fileName);
//    updateFilenameLayout();
    updateCollectButton();
    return true;
}

void TTBContent::updateCollectButton()
{
    if (m_currentpath.isEmpty()) {
        return;
    }
    if (DBManager::instance()->isImgExistInAlbum(COMMON_STR_FAVORITES, m_currentpath)) {
        m_clBT->setToolTip(tr("Unfavorite"));
        m_clBT->setIcon(QIcon::fromTheme("dcc_ccollection"));
        m_clBT->setIconSize(QSize(36, 36));
        m_bClBTChecked = true;
    } else {
        m_clBT->setToolTip(tr("Favorite"));
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        Q_UNUSED(themeType);
        m_clBT->setIcon(QIcon::fromTheme("dcc_collection_normal"));
        m_clBT->setIconSize(QSize(36, 36));
        m_bClBTChecked = false;
    }
}

void TTBContent::onResize()
{
    if (m_ItemLoaded.size() > 3) {
        m_imgList->setFixedSize((m_ItemLoaded.size() + 1)*THUMBNAIL_WIDTH + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT);
        m_imgList->resize((m_ItemLoaded.size() + 1)*THUMBNAIL_WIDTH + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT);
    } else if (m_ItemLoaded.size() > 1) {
        m_imgList->setFixedSize((m_ItemLoaded.size() + 1)*THUMBNAIL_WIDTH, TOOLBAR_HEIGHT);
        m_imgList->resize((m_ItemLoaded.size() + 1)*THUMBNAIL_WIDTH, TOOLBAR_HEIGHT);
        m_imgListView->show();
    }
    m_imgList->show();

//    if(m_nowIndex==0)
//        m_preButton->setDisabled(true);
//    if(m_nowIndex==m_allfileslist.size())
//        m_nextButton->setDisabled(true);


    m_windowWidth =  this->window()->geometry().width();
    if (m_ItemLoaded.size() <= 1) {
        m_contentWidth = TOOLBAR_JUSTONE_WIDTH;
    } else if (m_ItemLoaded.size() <= 3) {
        m_contentWidth = TOOLBAR_MINIMUN_WIDTH;
        m_imgListView->setFixedSize(QSize(TOOLBAR_DVALUE, TOOLBAR_HEIGHT));
    } else {
        m_contentWidth = qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_filelist_size - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) + THUMBNAIL_LIST_ADJUST;
        m_imgListView->setFixedSize(QSize(qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_filelist_size - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) - THUMBNAIL_VIEW_DVALUE + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT));
    }
    setFixedWidth(m_contentWidth);
//    m_contentWidth = window()->width() - 20;
//    setFixedWidth(m_contentWidth);
//    emit dApp->signalM->sigViewPanelSizeChanged();
    int a = (qCeil(m_imgListView->width() - 26) / 32) / 2;
    int b = m_ItemLoaded.size() - (qFloor(m_imgListView->width() - 26) / 32) / 2;
    if (m_nowIndex > a && m_nowIndex < b) {
        m_startAnimation = 1;
    } else if (m_nowIndex < m_ItemLoaded.size() - 2 * a && m_nowIndex > -1) {
        m_startAnimation = 2;
    } else if (m_nowIndex > 2 * a - 1 && m_nowIndex < m_ItemLoaded.size()) {
        m_startAnimation = 3;
    } else {
        m_startAnimation = 0;
    }
//    m_imgListView->show();

    if (!binsertneedupdate)
        return;
    if (1 == m_startAnimation) {
        m_imgList->move(QPoint((qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_ItemLoaded.size() - 3)),
                                     (qMax(width() - RT_SPACING, TOOLBAR_MINIMUN_WIDTH))) - 496 - 228 + 18) / 2 - ((32)*m_nowIndex), 0));
    } else if (2 == m_startAnimation) {
        m_imgList->move(QPoint(0, 0));
    } else if (3 == m_startAnimation) {
        m_imgList->move(QPoint(m_imgListView->width() - m_imgList->width() + 5, 0));
    } else if (0 == m_startAnimation) {
        m_imgList->move(QPoint(0, 0));
    }

//    m_windowWidth =  this->window()->geometry().width();
//    if (m_ItemLoaded.size() <= 1) {
//        m_contentWidth = TOOLBAR_JUSTONE_WIDTH;
//    } else if (m_ItemLoaded.size() <= 3) {
//        m_contentWidth = TOOLBAR_MINIMUN_WIDTH;
//        m_imgListView->setFixedSize(QSize(TOOLBAR_DVALUE, TOOLBAR_HEIGHT));
//    } else {
//        m_contentWidth = qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_filelist_size - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) + THUMBNAIL_LIST_ADJUST;
//        m_imgListView->setFixedSize(QSize(qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_filelist_size - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) - THUMBNAIL_VIEW_DVALUE + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT));
//    }
//    setFixedWidth(m_contentWidth);
}
