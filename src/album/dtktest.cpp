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
#include "dtktest.h"

#include <DGroupBox>
#include <DCheckBox>
#include <DDialog>
#include <DTextEdit>
#include <DArrowRectangle>
#include <DFloatingWidget>
#include <DIconButton>
#include <QVBoxLayout>
#include <QRadioButton>
#include <QLabel>
#include <QMessageBox>
#include <DTitlebar>
#include <QAction>
#include <QDebug>
#include <DSuggestButton>
#include "imageengine/imageengineapi.h"
#include "thumbnail/thumbnaillistview.h"

DtkTest::DtkTest(QWidget *parent)
    : DMainWindow(parent)
{
    ImageEngineApi::instance(this);
    resize(1080, 720);
    initUI();
}

void DtkTest::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);
    auto centralWidget = new QWidget;
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);
    ThumbnailListView *m_pThumbnailListView = new ThumbnailListView(ThumbnailDelegate::AllPicViewType);
    mainLayout->addWidget(m_pThumbnailListView);
//    QStringList list;
//    list << QString("/home/archermind/Desktop/testpic");
//    m_pThumbnailListView->importFilesFromLocal(list);
    m_pThumbnailListView->loadFilesFromDB();
}
