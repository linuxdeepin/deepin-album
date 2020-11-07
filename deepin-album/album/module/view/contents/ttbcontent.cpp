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
#include "utils/unionimage.h"

#include "dbmanager/dbmanager.h"
#include "controller/configsetter.h"
#include "widgets/elidedlabel.h"
#include "controller/signalmanager.h"
#include "imageengine/imageengineapi.h"

#include <QTimer>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QDebug>
#include <QPainterPath>
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
const int FILENAME_MAX_LENGTH = 600;
const int RIGHT_TITLEBAR_WIDTH = 100;
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

const int LOAD_LEFT_RIGHT = 25;     //前后加载图片数（动态）

}  // namespace

MyImageListWidget::MyImageListWidget(QWidget *parent)
    : QWidget(parent), m_timer(new QTimer(this))
{
    setMouseTracking(true);
    m_timer->setSingleShot(200);
    m_animationTimer = new QTimer(this);
    m_animationTimer->setInterval(100);
}

//bool MyImageListWidget::ifMouseLeftPressed()
//{
//    return bmouseleftpressed;
//}

QObject *MyImageListWidget::getObj()
{
    if (nullptr != m_obj) {
        return m_obj;
    }
    return nullptr;
}

void MyImageListWidget::setObj(QObject *obj)
{
    m_obj = obj;
    m_preListGeometryLeft = dynamic_cast<DWidget *>(m_obj)->geometry().left();
    m_resetAnimation = new QPropertyAnimation(m_obj, "pos");
    connect(m_resetAnimation, SIGNAL(finished()), this, SLOT(animationFinished()));
    connect(m_resetAnimation, SIGNAL(valueChanged(const QVariant)), this, SLOT(animationValueChanged(const QVariant)));
}

void MyImageListWidget::setSelectItem(ImageItem *selectItem)
{
    qDebug() << "zy------MyImageListWidget::setSelectItem";
    m_selectItem = selectItem;
    if (!m_isMoving) {
        if (m_resetAnimation->property("type") != "800") {
            animationStart(true, 0, 300);
        }
    }
}

void MyImageListWidget::animationStart(bool isReset, int endPos, int duration)
{
    if (dynamic_cast<DWidget *>(m_obj) == nullptr) {
        return;
    }
    if (m_resetAnimation->state() == QPropertyAnimation::State::Running) {
        m_resetAnimation->stop();
    }

    int moveX = 0;
//    qDebug() << "zy------this->left() = " << dynamic_cast<DWidget *>(m_obj)->geometry().left();
//    qDebug() << "zy------m_obj)->geometry().width() = " << dynamic_cast<DWidget *>(m_obj)->geometry().width();
//    qDebug() << "zy------this->width() = " << this->width();
//    qDebug() << "zy------this->geometry().width() = " << this->geometry().width();
//    qDebug() << "zy------m_selectItem->geometry().left() = " << m_selectItem->geometry().left();
    if (dynamic_cast<DWidget *>(m_obj)->geometry().width() <= this->width()) {
        moveX = 0;
    } else if (m_selectItem->geometry().left() < (this->geometry().width() / 2)) {
        //点击左侧
        moveX = 0;
    } else if ((dynamic_cast<DWidget *>(m_obj)->geometry().width() - m_selectItem->geometry().left()) < (this->geometry().width() / 2)) {
        //点击右侧
        moveX = this->geometry().width() - dynamic_cast<DWidget *>(m_obj)->geometry().width();
    } else {
        //点击中间
        moveX = this->geometry().width() / 2 - m_selectItem->index() * 32 - 26;
    }
    if (!isReset) {
        moveX = endPos;
    }
    m_resetAnimation->setDuration(duration);
    if (duration == 800) {
        m_resetAnimation->setProperty("type", "800");
    } else {
        m_resetAnimation->setProperty("type", "300");
    }
    m_resetAnimation->setEasingCurve(QEasingCurve::OutQuad);
    m_resetAnimation->setStartValue(dynamic_cast<DWidget *>(m_obj)->pos());
    m_resetAnimation->setEndValue(QPoint(moveX, dynamic_cast<DWidget *>(m_obj)->pos().y()));
    m_resetAnimation->start();
}

void MyImageListWidget::stopAnimation()
{
    m_resetAnimation->stop();
}

bool MyImageListWidget::isAnimationStart()
{
    if (m_resetAnimation->state() == QPropertyAnimation::State::Running) {
        return true;
    } else {
        return false;
    }
}

void MyImageListWidget::findSelectItem()
{
    QObjectList list = dynamic_cast<DWidget *>(m_obj)->children();
    for (int i = 0; i < list.size(); i++) {
        QList<ImageItem *> labelList = dynamic_cast<DWidget *>(m_obj)->findChildren<ImageItem *>(QString("%1").arg(i));
        if (labelList.size() <= 0) {
            continue;
        }
        ImageItem *img = labelList.at(0);
        if (nullptr == img) {
            continue;
        }
        if (img->index() == img->indexNow()) {
            m_selectItem = img;
            break;
        }
    }
}

void MyImageListWidget::thumbnailIsMoving()
{
    if (m_obj == nullptr || m_selectItem == nullptr || m_resetFinish) {
        return;
    }
    if (m_resetAnimation->state() == QPropertyAnimation::State::Running && m_resetAnimation->duration() == 300) {
        return;
    }
    int offsetLimit = dynamic_cast<DWidget *>(m_obj)->geometry().left() - m_preListGeometryLeft;
    if (abs(offsetLimit) <= 32) {
        return;
    }
    int index = 0;
    QObjectList list = dynamic_cast<DWidget *>(m_obj)->children();
    if (dynamic_cast<DWidget *>(m_obj)->geometry().width() <= this->width()) {
        if (offsetLimit < 0) {
            index = m_selectItem->index() + 1;
            if (index >= list.size()) {
                index = list.size() - 1;
            }
        } else {
            index = m_selectItem->index() - 1;
            if (index < 0) {
                index = 0;
            }
        }
        QList<ImageItem *> labelList2 = dynamic_cast<DWidget *>(m_obj)->findChildren<ImageItem *>(QString("%1").arg(index));
        if (labelList2.size() > 0) {
            ImageItem *img2 = labelList2.at(0);
            if (nullptr != img2) {
                img2->emitClickSig();
            }
        }
    } else {
        int offset = dynamic_cast<DWidget *>(m_obj)->geometry().left() - m_preListGeometryLeft;
        if (offset > 0) {
            index = m_preSelectItemIndex - abs(offset) / 32;
            if (index < 0) {
                index = 0;
            }
        } else if (offset < 0) {
            index = m_preSelectItemIndex + abs(offset) / 32;
            if (index >= list.size()) {
                index = list.size() - 1;
            }
        }
        QList<ImageItem *> labelList2 = dynamic_cast<DWidget *>(m_obj)->findChildren<ImageItem *>(QString("%1").arg(index));
        if (labelList2.size() > 0) {
            ImageItem *img2 = labelList2.at(0);
            if (nullptr != img2) {
                img2->emitClickSig();
            }
        }
    }
}

void MyImageListWidget::animationValueChanged(const QVariant value)
{
    Q_UNUSED(value)
    if (m_resetAnimation->property("type") != "800") {
        return;
    }
    //惯性滑动过程中显示选中图元
    thumbnailIsMoving();
}

bool MyImageListWidget::eventFilter(QObject *obj, QEvent *e)
{
    Q_UNUSED(obj)
    if (e->type() == QEvent::Leave) {
    }
    if (e->type() == QEvent::MouseButtonPress) {
        m_resetAnimation->stop();
        m_animationTimer->start();
        findSelectItem();
        m_isMoving = false;
        m_resetFinish = false;
        bmouseleftpressed = true;
        //记录点下时当前选中的缩略图图元index，便于移动和惯性时比较
        if (m_selectItem) {
            m_preSelectItemIndex = m_selectItem->indexNow();
        }
        m_preListGeometryLeft = dynamic_cast<DWidget *>(m_obj)->geometry().left();
        QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent *>(e);
        m_presspoint = mouseEvent->globalPos();
        m_prepoint = mouseEvent->globalPos();
        m_movePoints.clear();
    }

    if (e->type() == QEvent::MouseButtonRelease) {
        bmouseleftpressed = false;
        int posX = dynamic_cast<DWidget *>(m_obj)->x();
        int viewwidth = dynamic_cast<DWidget *>(m_obj)->width() + dynamic_cast<DWidget *>(m_obj)->x();
        //向右加载数据
        if ((viewwidth - this->width()) < 32 && viewwidth - this->width() > -32) {
            if (m_timer->isActive()) {
                return false;
            } else {
                m_timer->start();
                emit testloadRight();
            }
        }
        //向左加载数据
        if (posX > -32 && posX < 32) {
            if (m_timer->isActive()) {
                return false;
            } else {
                m_timer->start();
                emit testloadLeft();
            }
        }
        emit mouseLeftReleased();
        if (m_isMoving) {
            if (m_movePoints.size() > 0) {
                int endPos = dynamic_cast<DWidget *>(m_obj)->pos().x() + m_movePoints.last().x() - m_movePoints.first().x();
                //过滤掉触屏点击时的move误操作
                if (abs(m_movePoints.last().x() - m_movePoints.first().x()) > 15) {
                    animationStart(false, endPos, 800);
                } else {
                    animationStart(true, 0, 300);
                }
            }
        }
        if (!m_isMoving) {
            m_resetFinish = false;
            animationStart(true, 0, 300);
        }
        m_isMoving = false;
    }
    if (e->type() == QEvent::Leave && obj == m_obj) {
        bmouseleftpressed = false;
//        emit mouseLeftReleased();
    }
    if (e->type() == QEvent::MouseMove && bmouseleftpressed) {
        QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent *>(e);
        m_isMoving = true;
        QPoint p = mouseEvent->globalPos();
        if (m_movePoints.size() < 20) {
            m_movePoints.push_back(p);
        } else {
            m_movePoints.pop_front();
            m_movePoints.push_back(p);
        }

        if (p.x() <= m_prepoint.x()) {
            m_moveToRight = true;
        } else {
            m_moveToRight = false;
        }
        m_prepoint = mouseEvent->globalPos();
        dynamic_cast<DWidget *>(m_obj)->move((dynamic_cast<DWidget *>(m_obj))->x() + p.x() - m_presspoint.x(), ((dynamic_cast<DWidget *>(m_obj))->y()));
        m_presspoint = p;
        int posX = dynamic_cast<DWidget *>(m_obj)->x();
        int viewwidth = dynamic_cast<DWidget *>(m_obj)->width() + dynamic_cast<DWidget *>(m_obj)->x();
        //多动过程中显示选中图元
        thumbnailIsMoving();

        QObjectList list = dynamic_cast<DWidget *>(m_obj)->children();
        //向右加载数据
        if ((viewwidth - this->width()) < 32 && viewwidth - this->width() > -32) {
//            emit needContinueRequest();
            if (m_timer->isActive()) {
                return false;
            } else {
                m_timer->start();
                emit testloadRight();
            }
        } else {
            emit silmoved();
        }

        //向左加载数据
        if (posX > -32 && posX < 32) {
            if (m_timer->isActive()) {
                return false;
            } else {
                m_timer->start();
                emit testloadLeft();
            }
        }
    }
    return false;
}

void MyImageListWidget::animationTimerTimeOut()
{
    m_animationTimerTOCount++;

    if (m_obj == nullptr || m_selectItem == nullptr || m_resetFinish) {
        return;
    }
    if (m_resetAnimation->state() == QPropertyAnimation::State::Running && m_resetAnimation->duration() == 300) {
        return;
    }
    int offset1 = dynamic_cast<DWidget *>(m_obj)->geometry().left() - m_preListGeometryLeft;
    if (abs(offset1) <= 4) {
        return;
    }
    if (m_animationTimerTOCount == 20) {
        m_animationTimerTOCount = 0;
    }
    int index = 0;
    QObjectList list = dynamic_cast<DWidget *>(m_obj)->children();

    int middle = this->geometry().width() / 2;
    if (dynamic_cast<DWidget *>(m_obj)->geometry().width() <= this->width()) {
        qDebug() << "11111111";
        if (offset1 < 0) {
            index = m_selectItem->index() + 1;
            if (index >= list.size()) {
                index = list.size() - 1;
            }
        } else {
            index = m_selectItem->index() - 1;
            if (index < 0) {
                index = 0;
            }
        }
        QList<ImageItem *> labelList2 = dynamic_cast<DWidget *>(m_obj)->findChildren<ImageItem *>(QString("%1").arg(index));
        if (labelList2.size() > 0) {
            ImageItem *img2 = labelList2.at(0);
            if (nullptr != img2) {
                qDebug() << "2222222222222";
                img2->emitClickSig();
            }
        }
    } else {
        int selitemLeft = m_selectItem->geometry().left() - abs(dynamic_cast<DWidget *>(m_obj)->geometry().left());
        int selitemRight = selitemLeft + 32;
        if (selitemLeft < middle && middle < selitemRight) {
            for (int i = 0; i < list.size(); i++) {
                QList<ImageItem *> labelList = dynamic_cast<DWidget *>(m_obj)->findChildren<ImageItem *>(QString("%1").arg(i));
                if (labelList.size() > 0) {
                    ImageItem *img = labelList.at(0);
                    if (nullptr == img) {
                        continue;
                    }
                    int itemLeft = img->geometry().left() - abs(dynamic_cast<DWidget *>(m_obj)->geometry().left());
                    int itemRight = itemLeft + 32;
                    if (itemLeft < middle && middle < itemRight) {
                        qDebug() << "1111111111111";
                        img->emitClickSig();
                        break;
                    }
                }
            }
        } else {
            int offset = dynamic_cast<DWidget *>(m_obj)->geometry().left() - m_preListGeometryLeft;
            qDebug() << "-------begin-------";
            qDebug() << "zy------(m_obj)->geometry().left() = " << dynamic_cast<DWidget *>(m_obj)->geometry().left();
            qDebug() << "zy------m_preListGeometryLeft = " << m_preListGeometryLeft;
            qDebug() << "-------end------- ";
            if (offset > 0) {
                qDebug() << "offset > 0 ";
                index = m_selectItem->index() - 1;
                if (index < 0) {
                    index = 0;
                }
            } else if (offset < 0) {
                qDebug() << "offset < 0 ";
                index = m_selectItem->index() + 1;
                if (index >= list.size()) {
                    index = list.size() - 1;
                }
            }
            QList<ImageItem *> labelList2 = dynamic_cast<DWidget *>(m_obj)->findChildren<ImageItem *>(QString("%1").arg(index));
            if (labelList2.size() > 0) {
                ImageItem *img2 = labelList2.at(0);
                if (nullptr != img2) {
                    qDebug() << "333333333";
                    img2->emitClickSig();
                }
            }
        }
    }
    m_preListGeometryLeft = dynamic_cast<DWidget *>(m_obj)->geometry().left();
}

void MyImageListWidget::animationFinished()
{
    //设置type标志用来判断是惯性动画还是复位动画
    if (m_resetAnimation->property("type") == "800") {
        m_resetFinish = false;
        animationStart(true, 0, 300);
    }
    if (m_resetAnimation->property("type") == "300") {
        m_resetFinish = true;
    }
}

ImageItem::ImageItem(int index, ImageDataSt data, QWidget *parent):
    QLabel(parent)
{
    m_timer = new QTimer();
    m_timer->setInterval(200);
    m_timer->setSingleShot(true);
    _index = index;
    _path = data.dbi.filePath;
    m_bPicNotSuppOrDamaged = data.imgpixmap.isNull();
    bool bLight = DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType;
    _pixmap = m_bPicNotSuppOrDamaged ? utils::image::getDamagePixmap(bLight) : data.imgpixmap;
    _image = new DLabel(this);

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &ImageItem::updateDmgIconByTheme);
}


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
    if (m_timer->isActive()) {
        emit imageItemclicked(_index, _indexNow);
    }
    QLabel::mouseReleaseEvent(ev);
}

void ImageItem::mousePressEvent(QMouseEvent *ev)
{
    Q_UNUSED(ev);
    bmouserelease = false;
    m_timer->start();
//    QEventLoop loop;
//    QTimer::singleShot(200, &loop, SLOT(quit()));
//    loop.exec();
//    if (bmouserelease) {
//        emit imageItemclicked(_index, _indexNow);
//    }
    QLabel::mousePressEvent(ev);
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
    bp1.addRoundedRect(pixmapRect, 4, 4);
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
        this->setIndexNow(index);
        _index--;
        return true;
    }
    return false;
}

void ImageItem::emitClickSig()
{
    emit imageItemclicked(_index, _indexNow);
}


TTBContent::TTBContent(bool inDB, QStringList filelist, QWidget *parent) : QLabel(parent)
{
    onThemeChanged(dApp->viewerTheme->getCurrentTheme());
    m_windowWidth = std::max(this->window()->width(),
                             ConfigSetter::instance()->value("MAINWINDOW", "WindowWidth").toInt());
    m_allfileslist = filelist;
    m_filelist_size = m_allfileslist.size();
    if (m_allfileslist.size() <= 1) {
        m_contentWidth = TOOLBAR_JUSTONE_WIDTH;
    } else if (m_allfileslist.size() <= 3) {
        m_contentWidth = TOOLBAR_MINIMUN_WIDTH;
    } else {
        m_contentWidth = qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_allfileslist.size() - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) + THUMBNAIL_LIST_ADJUST;
    }

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
        //2020/6/9 DJH 优化退出全屏，不再闪出退出全屏的间隙 31331
        this->setVisible(false);
        emit dApp->signalM->hideImageView();
        this->setVisible(true);
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

    connect(m_preButton, &DIconButton::clicked, this, &TTBContent::onPreButton);

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


    connect(m_nextButton, &DIconButton::clicked, this, &TTBContent::onNextButton);


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
            DBManager::instance()->removeFromAlbum(COMMON_STR_FAVORITES, QStringList(m_currentpath), AlbumDBType::Favourite);
        } else
        {
            DBManager::instance()->insertIntoAlbum(COMMON_STR_FAVORITES, QStringList(m_currentpath), AlbumDBType::Favourite);
            emit dApp->signalM->insertedIntoAlbum(COMMON_STR_FAVORITES, QStringList(m_currentpath));
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
    m_imgListView = new MyImageListWidget(this);
    m_imgList = new DWidget(m_imgListView);
    m_imgList->setObjectName("imageListObj");
    m_imgListView->setObj(m_imgList);
    m_imgListView->setObjectName("MyImageListWidget");
    m_imgList->installEventFilter(m_imgListView);
    connect(m_imgListView, &MyImageListWidget::testloadRight, this, [ = ] {
        if (m_rightlist.isEmpty())
            return ;
        qDebug() << "请求加载右边图片";
        QStringList loadRight;
        for (int i = 0; i < LOAD_LEFT_RIGHT; ++i)
        {
            if (m_rightlist.isEmpty())
                break;
            ImageDataSt data;
            if (ImageEngineApi::instance()->getImageData(m_rightlist.first(), data)) {
                insertImageItem(data);
                loadRight << m_rightlist.first();
                m_rightlist.removeFirst();
            }
        }
        m_allfileslist << loadRight;
        //m_filelist_size = m_allfileslist.size();
//        m_imgList->setFixedWidth(m_imgList->width() + 32 * loadRight.size());
        emit sigloadRight(loadRight);

    });
    connect(m_imgListView, &MyImageListWidget::testloadLeft, this, [ = ] {
        if (m_leftlist.isEmpty())
            return;
        qDebug() << "请求加载左边图片";
        QStringList loadLeft;
        for (int i = 0; i < LOAD_LEFT_RIGHT; ++i)
        {
            if (m_leftlist.isEmpty())
                break;
            ImageDataSt data;
            if (ImageEngineApi::instance()->getImageData(m_leftlist.last(), data)) {
                insertImageItem(data, false);
                loadLeft << m_leftlist.last();
                m_leftlist.removeLast();
            }
        }
        for (const auto &path : loadLeft)
            m_allfileslist.push_front(path);    //倒序放置
        m_filelist_size = m_allfileslist.size();
        emit sigloadLeft(loadLeft);
    });

    connect(dApp->signalM, &SignalManager::hideImageView, this, [ = ] {
        m_imgList->hide();
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
        this->updateScreen();
    });

    if (m_allfileslist.size() <= 3) {
        m_imgList->setFixedSize(QSize(TOOLBAR_DVALUE, TOOLBAR_HEIGHT));
    } else {
        m_imgList->setFixedSize(QSize(qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_allfileslist.size() - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) - THUMBNAIL_VIEW_DVALUE + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT));
    }

    m_imgList->setDisabled(false);
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

//    QPalette palette(m_imgList->palette()) ;
//    m_imgList->setAutoFillBackground(true);
//    palette.setColor(QPalette::Background, Qt::red); // 最后一项为透明度
//    m_imgList->setPalette(palette);
//    m_imgListView->setPalette(palette);


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

    connect(m_trashBtn, &DIconButton::clicked, this, [ = ] {
        emit dApp->signalM->deleteByMenu();
    });
//    connect(m_trashBtn, &DIconButton::clicked, SignalManager::instance(), &SignalManager::deleteByMenu);
    m_filesbeleft << filelist;
    m_allNeedRequestFilesCount += filelist.size();

    connect(this, &TTBContent::sigRequestSomeImages, this, [ = ] {
        requestSomeImages();
    });
}


void TTBContent::requestSomeImages()
{
    bneedloadimage = false;

    if (m_filesbeleft.size() < Number_Of_Displays_Per_Time) {
        m_requestCount += m_filesbeleft.size();
    } else {
        m_requestCount += Number_Of_Displays_Per_Time;
    }
    for (int i = 0; i < Number_Of_Displays_Per_Time; i++) {
        if (m_filesbeleft.size() <= 0) {
            brequestallfiles = true;
            updateScreen();
            return;
        }
        QString firstfilesbeleft = m_filesbeleft.first();
        m_filesbeleft.removeFirst();
        ImageEngineApi::instance()->reQuestImageData(firstfilesbeleft, this);
    }
}

void TTBContent::setRightlist(const QStringList &rightlist)
{
    m_rightlist = rightlist;
}

void TTBContent::setLeftlist(const QStringList &leftlist)
{
    m_leftlist = leftlist;
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
    }
    if (m_requestCount < 1
       ) {
        requestSomeImages();
    }
    return reb;
}

int TTBContent::itemLoadedSize()
{
    return m_allfileslist.size();
}

QString TTBContent::getIndexPath(int index)
{
    return m_allfileslist.at(index);
}

void TTBContent::updateScreen()
{
    qDebug() << "zy------TTBContent::updateScreen";
    if (m_ItemLoaded.size() > 3) {
        m_imgList->setFixedSize((m_ItemLoaded.size() + 1)*THUMBNAIL_WIDTH, TOOLBAR_HEIGHT);
        m_imgList->resize((m_ItemLoaded.size() + 1)*THUMBNAIL_WIDTH + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT);

        m_imgList->setContentsMargins(0, 0, 0, 0);
        m_imgListView->show();

        QList<ImageItem *> labelList = m_imgList->findChildren<ImageItem *>(QString("%1").arg(m_nowIndex));

        if (m_nowIndex > -1) {

            if (labelList.isEmpty())
                return;

            labelList.at(0)->setIndexNow(m_nowIndex);
            m_imgListView->setSelectItem(labelList.at(0));

            if (m_lastIndex > -1 && m_lastIndex != m_nowIndex) {
                QList<ImageItem *> lastlabelList = m_imgList->findChildren<ImageItem *>(QString("%1").arg(m_lastIndex));
                if (lastlabelList.isEmpty()) {
                    return;
                }
                auto num = 32;
                lastlabelList.at(0)->setFixedSize(QSize(num, 40));
                lastlabelList.at(0)->resize(QSize(num, 40));
                lastlabelList.at(0)->setIndexNow(m_nowIndex);
            }
            if (labelList.size() > 0) {
                labelList.at(0)->setFixedSize(QSize(58, 58));
                labelList.at(0)->resize(QSize(58, 58));
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

        m_imgListView->show();
        if (!binsertneedupdate)
            return;
        QList<ImageItem *> labelList = m_imgList->findChildren<ImageItem *>(QString("%1").arg(m_nowIndex));
        if (m_nowIndex > -1) {

            if (labelList.isEmpty())
                return;
            if (m_nowIndex < m_ItemLoaded.size()) {
                labelList.at(0)->setIndexNow(m_nowIndex);
            }
            if (m_lastIndex > -1 && m_lastIndex != m_nowIndex) {
                QList<ImageItem *> lastlabelList = m_imgList->findChildren<ImageItem *>(QString("%1").arg(m_lastIndex));
                if (lastlabelList.isEmpty())
                    return;
                auto num = 32;
                lastlabelList.at(0)->setFixedSize(QSize(num, 40));
                lastlabelList.at(0)->resize(QSize(num, 40));
                lastlabelList.at(0)->setIndexNow(m_nowIndex);
            }
            if (labelList.size() > 0) {
                labelList.at(0)->setFixedSize(QSize(58, 58));
                labelList.at(0)->resize(QSize(58, 58));
                m_imgListView->setSelectItem(labelList.at(0));
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
            if (m_nowIndex == m_ItemLoaded.size() - 1) {
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

        m_imgListView->show();
        if (!binsertneedupdate)
            return;
        QList<ImageItem *> labelList = m_imgList->findChildren<ImageItem *>(QString("%1").arg(m_nowIndex));
        if (m_nowIndex > -1) {
            if (labelList.isEmpty())
                return;
            if (m_nowIndex < labelList.size()) {
                labelList.at(0)->setIndexNow(m_nowIndex);
            }
            if (m_lastIndex > -1) {
                QList<ImageItem *> lastlabelList = m_imgList->findChildren<ImageItem *>(QString("%1").arg(m_lastIndex));
                if (lastlabelList.isEmpty()) {
                    return;
                }
                auto num = 32;
                lastlabelList.at(0)->setFixedSize(QSize(num, 40));
                lastlabelList.at(0)->resize(QSize(num, 40));
                lastlabelList.at(0)->setIndexNow(m_nowIndex);
            }
            if (labelList.size() > 0) {
                labelList.at(0)->setFixedSize(QSize(58, 58));
                labelList.at(0)->resize(QSize(58, 58));
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
    m_windowWidth = this->window()->geometry().width();
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

void TTBContent::insertImageItem(const ImageDataSt &file, bool bloadRight)
{
    int index = m_ItemLoaded.size();
    TTBContentData data;
    data.index = index;
    data.data = file;
    m_ItemLoaded.insert(file.dbi.filePath, data);
    ImageItem *imageItem = new ImageItem(index, file);
    imageItem->setFixedSize(QSize(32, 40));
    imageItem->resize(QSize(32, 40));
    //imageItem->installEventFilter(m_imgListView);

    imageItem->setObjectName(QString("%1").arg(index));

    if (bloadRight) //右侧加载
        m_imglayout->addWidget(imageItem);
    else {
        m_imglayout->insertWidget(0, imageItem);
        QList<ImageItem *> labelList1 = m_imgList->findChildren<ImageItem *>();
        for (auto &lable : labelList1) {
            if (lable->index() == index) { //刚添加进入的
                lable->setIndex(0);     //设置索引为0
                lable->setObjectName("0");
            } else {
                lable->setObjectName(QString("%1").arg(lable->index() + 1));
                if (lable->indexNow() == lable->index()) { //当前已选中
                    lable->setIndex(lable->index() + 1);
                    lable->setIndexNow(lable->indexNow() + 1);
                    m_imgListView->setSelectItem(lable);
                    m_lastIndex = lable->indexNow();
                    m_nowIndex = lable->indexNow();
//                    qDebug() << "name: " << lable->objectName() << "index: " << lable->index() << "indexnow: " << lable->indexNow();
                } else {
                    lable->setIndex(lable->index() + 1);    //索引加1
                    lable->setIndexNow(-1);
                }
            }
        }
        //已加载数据index全部加1
        for (auto &contenData : m_ItemLoaded) {
            if (contenData.index == index)
                contenData.index = 0;
            else
                contenData.index++;
        }
    }

    connect(imageItem, &ImageItem::imageItemclicked, this, [ = ](int index, int indexNow) {
        qDebug() << "zy------ImageItem::imageItemclicked";
        binsertneedupdate = true;
        m_nowIndex = index;

        if (m_lastIndex > -1) {
            QList<ImageItem *> lastlabelList = m_imgList->findChildren<ImageItem *>(QString("%1").arg(m_lastIndex));
            if (lastlabelList.isEmpty()) {
                return;
            }
            lastlabelList.at(0)->setFixedSize(QSize(32, 40));
            lastlabelList.at(0)->resize(QSize(32, 40));
            lastlabelList.at(0)->setIndexNow(m_nowIndex);
        }

        QList<ImageItem *> labelList = m_imgList->findChildren<ImageItem *>(QString("%1").arg(index));
        if (labelList.size() > 0) {
            ImageItem *img = labelList.at(0);
            if (nullptr != img) {
                labelList.at(0)->setFixedSize(QSize(58, 58));
                labelList.at(0)->resize(QSize(58, 58));
                img->setIndexNow(m_nowIndex);
                m_imgListView->setSelectItem(img);
            }
        }
        m_lastIndex = m_nowIndex;

        bfilefind = true;
        m_currentpath = imageItem->_path;
        qDebug() << "单击：" << "index: " << index << "indexnow: " << indexNow << "path: " << m_currentpath;
        emit imageClicked(index, (index - indexNow));
        emit ttbcontentClicked();
    });
    if (-1 == m_nowIndex && file.dbi.filePath == m_currentpath) {
        m_nowIndex = index;
        bfilefind = true;
        emit feedBackCurrentIndex(m_nowIndex, m_currentpath);
//        setCurrentItem();
    } else {
//        if (binsertneedupdate)
//        updateScreen();
        onResize();
//        updateScreenNoAnimation();
    }
//    onResize();
}

//void TTBContent::reLoad()
//{
//    clearAndStopThread();

//    m_ItemLoaded.clear();
//    m_filesbeleft.clear();
//    m_filesbeleft << m_allfileslist;
//    m_requestCount = 0;

//    if (m_allfileslist.size() <= 3) {
//        m_imgList->setFixedSize(QSize(TOOLBAR_DVALUE, TOOLBAR_HEIGHT));
//        m_imgListView->setFixedSize(QSize(TOOLBAR_DVALUE, TOOLBAR_HEIGHT));
//    } else {
//        m_imgList->setFixedSize(QSize(qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_allfileslist.size() - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) - THUMBNAIL_VIEW_DVALUE + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT));
//        m_imgListView->setFixedSize(QSize(qMin((TOOLBAR_MINIMUN_WIDTH + THUMBNAIL_ADD_WIDTH * (m_allfileslist.size() - 3)), qMax(m_windowWidth - RT_SPACING, TOOLBAR_MINIMUN_WIDTH)) - THUMBNAIL_VIEW_DVALUE + THUMBNAIL_LIST_ADJUST, TOOLBAR_HEIGHT));
//    }
//    m_allNeedRequestFilesCount = m_allfileslist.size();
//    QLayoutItem *child;
//    while ((child = m_imglayout->takeAt(0)) != nullptr) {
//        m_imglayout->removeWidget(child->widget());
//        child->widget()->setParent(nullptr);
//        delete child;

//    }
//    if (m_filesbeleft.size() > 0) {
//        requestSomeImages();
//    }
//}

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
    m_imgListView->stopAnimation();
    m_ItemLoaded.remove(m_currentpath);
    if (m_allfileslist.size() == 0)
        return;
    m_allfileslist.removeAt(m_nowIndex == -1 ? 0 : m_nowIndex);
    m_allNeedRequestFilesCount = m_allfileslist.size();

    QList<ImageItem *> labelList = m_imgList->findChildren<ImageItem *>();

    ImageItem *getim = nullptr;

    for (auto &lable : labelList) {
        if (lable->index() == m_nowIndex) {
            getim = lable;
        } else {
            if (lable->index_1(m_nowIndex)) { //图片索引大于删除图片索引
                m_ItemLoaded[lable->_path].index--;     //修改已加载图片索引
                lable->setObjectName(QString("%1").arg(m_ItemLoaded[lable->_path].index));
            }
        }
    }

    QList<ImageItem *> labelList1 = m_imgList->findChildren<ImageItem *>();
    if (m_allfileslist.size() > 0) {
        if (m_allfileslist.size() > m_nowIndex) {
            m_lastIndex = -1;
            if (m_nowIndex > -1)
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

    emit ttbcontentClicked();
    emit removed();     //删除数据库图片
    m_imgListView->animationStart(true, 0, 300);
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

void TTBContent::onNextButton()
{
    int viewwidth = dynamic_cast<DWidget *>(m_imgListView->getObj())->width() + dynamic_cast<DWidget *>(m_imgListView->getObj())->x();

    //向右加载数据
    if ((viewwidth - m_imgListView->width()) < 32 && viewwidth - m_imgListView->width() > -32) {
        if (m_imgListView->m_timer->isActive()) {
            return;
        } else {
            m_imgListView->m_timer->start();
            emit m_imgListView->testloadRight();
        }
    }
    emit showNext();
    emit ttbcontentClicked();
}

void TTBContent::onPreButton()
{
    //向左加载数据
    int posX = dynamic_cast<DWidget *>(m_imgListView->getObj())->x();
    if (posX > -32 && posX < 32) {
        if (m_imgListView->m_timer->isActive()) {
            return;
        } else {
            m_imgListView->m_timer->start();
            emit m_imgListView->testloadLeft();
        }
    }
    emit showPrevious();
    emit ttbcontentClicked();
}

void TTBContent::onThemeChanged(ViewerThemeManager::AppTheme theme)
{
    Q_UNUSED(theme);
}

void TTBContent::setCurrentDir(const QString &text)
{
#ifndef LITE_DIV
    if (text == COMMON_STR_FAVORITES) {
        text = tr(COMMON_STR_FAVORITES);
    }
    m_returnBtn->setText(text);
#else
    Q_UNUSED(text)
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
    qDebug() << "zy------TTBContent::setImage";
    binsertneedupdate = true;
    if (!m_allfileslist.isEmpty() && !QFileInfo(path).exists()) {
        emit dApp->signalM->picNotExists(true);
        m_currentpath = path;
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
//        QList<ImageItem *> labelList = m_imgList->findChildren<ImageItem *>();
        QList<ImageItem *> labelList = m_imgList->findChildren<ImageItem *>(QString("%1").arg(m_nowIndex));
        ImageDataSt gdata;
        if (ImageEngineApi::instance()->getImageData(m_currentpath, gdata)) {
            if (labelList.isEmpty())
                return;
            labelList.at(0)->setPic(gdata.imgpixmap);
        }
        setCurrentItem();
    }
}

bool TTBContent::setCurrentItem()
{
    if (m_currentpath.isEmpty() || !QFileInfo(m_currentpath).exists()) {
        m_adaptImageBtn->setDisabled(true);
        m_adaptScreenBtn->setDisabled(true);
        m_rotateLBtn->setDisabled(true);
        m_rotateRBtn->setDisabled(true);
        m_trashBtn->setDisabled(true);
        m_imgList->setDisabled(false);
        m_clBT->setDisabled(true);
    } else {
        m_adaptImageBtn->setDisabled(false);
        m_adaptScreenBtn->setDisabled(false);

        updateScreen();
        m_trashBtn->setDisabled(false);
        if (UnionImage_NameSpace::canSave(m_currentpath)) {
            m_rotateLBtn->setDisabled(false);
            m_rotateRBtn->setDisabled(false);
        } else {
            m_rotateLBtn->setEnabled(false);
            m_rotateRBtn->setEnabled(false);
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
    if (DBManager::instance()->isImgExistInAlbum(COMMON_STR_FAVORITES, m_currentpath, AlbumDBType::Favourite)) {
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

    if (!binsertneedupdate)
        return;
}
