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
#include "imgviewwidget.h"
#include "application.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "utils/unionimage.h"

#include "dbmanager/dbmanager.h"
#include "controller/configsetter.h"
#include "widgets/elidedlabel.h"
#include "imageengine/imageengineapi.h"
#include "ac-desktop-define.h"

#include <QTimer>
#include <QScroller>
#include <QScrollBar>
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

#include "imgviewlistview.h"
#include "imagedataservice.h"

DWIDGET_USE_NAMESPACE

MyImageListWidget::MyImageListWidget(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *hb = new QHBoxLayout(this);
    hb->setContentsMargins(0, 0, 0, 0);
    hb->setSpacing(0);
    this->setLayout(hb);
    m_listview = new ImgViewListView(this);
    m_listview->setObjectName("m_imgListWidget");
    hb->addWidget(m_listview);

    connect(m_listview, &ImgViewListView::clicked, this, &MyImageListWidget::onClicked);
    connect(m_listview, &ImgViewListView::openImg, this, &MyImageListWidget::openImg);
    connect(m_listview->horizontalScrollBar(), &QScrollBar::valueChanged, this, &MyImageListWidget::onScrollBarValueChanged);
}

MyImageListWidget::~MyImageListWidget()
{
}

void MyImageListWidget::setAllFile(const SignalManager::ViewInfo &info, QString path)
{
    m_listview->setAllFile(info, path);
    bool visual = info.dBImgInfos.size() > 1;
    if (!visual) {
        //尝试paths的大小
        visual = info.paths.size() > 1;
    }
    this->setVisible(visual);
    setSelectCenter();
    emit openImg(m_listview->getSelectIndexByPath(path), path);
}

DBImgInfo MyImageListWidget::getImgInfo(const QString &path)
{
    DBImgInfo info;
    for (int i = 0; i < m_listview->m_model->rowCount(); i++) {
        QModelIndex indexImg = m_listview->m_model->index(i, 0);
        DBImgInfo infoImg = indexImg.data(Qt::DisplayRole).value<DBImgInfo>();
        if (infoImg.filePath == path) {
            info = infoImg;
            break;
        }
    }
    return info;
}
//将选中项移到最前面，后期可能有修改，此时获取的列表宽度不正确
void MyImageListWidget::setSelectCenter()
{
    m_listview->setSelectCenter();
}

int MyImageListWidget::getImgCount()
{
    return m_listview->m_model->rowCount();
}

void MyImageListWidget::removeCurrent()
{
    m_listview->removeCurrent();
    this->setVisible(getImgCount() > 1);
}

void MyImageListWidget::reloadImage(int value)
{
    //加载上下两百张
    if (m_loadTimer == nullptr) {
        m_loadTimer = new QTimer(this);
        m_loadTimer->setInterval(50);
        m_loadTimer->setSingleShot(true);
        connect(m_loadTimer, &QTimer::timeout, this, [ = ] {
            qDebug() << __FUNCTION__ << "---m_value = " << m_value;
            QModelIndex load = m_listview->indexAt(QPoint(m_value, 30));
            if (!load.isValid())
            {
                load = m_listview->indexAt(QPoint(m_value + 10, 30));
            }
            if (!load.isValid())
            {
                load = m_listview->indexAt(QPoint(m_value + 20, 30));
            }
            if (!load.isValid())
            {
                load = m_listview->m_model->index(0, 0);
            }
            if (!load.isValid())
            {
                return;
            }
            QStringList pathlist;
            int count = 0;
            DBImgInfo dataload = load.data(Qt::DisplayRole).value<DBImgInfo>();
            qDebug() << __FUNCTION__ << "---" << dataload.filePath;
            for (int i = load.row(); i >= 0; i--)
            {
                QModelIndex index = m_listview->m_model->index(i, 0);
                DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
                pathlist << data.filePath;
                count++;
                if (count >= 200) {
                    break;
                }
            }
            count = 0;
            for (int i = load.row(); i < m_listview->m_model->rowCount(); i++)
            {
                QModelIndex index = m_listview->m_model->index(i, 0);
                DBImgInfo data = index.data(Qt::DisplayRole).value<DBImgInfo>();
                pathlist << data.filePath;
                count++;
                if (count >= 200) {
                    break;
                }
            }
            qDebug() << __FUNCTION__ << "---pathlist = " << pathlist.size();
            ImageDataService::instance()->readThumbnailByPaths(pathlist, false);
            m_listview->update();
        });
    }
    if (m_loadTimer->isActive()) {
        m_loadTimer->stop();
    }
    m_loadTimer->start();
}

void MyImageListWidget::onScrollBarValueChanged(int value)
{
    m_value = value;
    reloadImage(value);
    Q_UNUSED(value)
    QModelIndex index = m_listview->indexAt(QPoint((m_listview->width() - 15), 10));
    if (!index.isValid()) {
        index = m_listview->indexAt(QPoint((m_listview->width() - 20), 10));
    }
}

void MyImageListWidget::openNext()
{
    m_listview->openNext();
}

void MyImageListWidget::openPre()
{
    m_listview->openPre();
}

bool MyImageListWidget::isLast()
{
    return m_listview->isLast();
}

bool MyImageListWidget::isFirst()
{
    return m_listview->isFirst();
}

void MyImageListWidget::onClicked(const QModelIndex &index)
{
    m_listview->onClicked(index);
}
