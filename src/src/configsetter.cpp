// Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "configsetter.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QProcess>

const QString CONFIG_PATH_IMAGE_VIEWER =   QDir::homePath() +
                                           "/.config/deepin/deepin-image-viewer/config.conf";
const QString CONFIG_PATH_ALBUM =   QDir::homePath() +
                                    "/.config/deepin/deepin-album/config.conf";

LibConfigSetter::LibConfigSetter(QObject *parent) : QObject(parent)
    , m_viewType(imageViewerSpace::ImgViewerTypeNull)
{

}

LibConfigSetter *LibConfigSetter::m_setter = nullptr;
LibConfigSetter *LibConfigSetter::instance()
{
    if (!m_setter) {
        m_setter = new LibConfigSetter();
    }

    return m_setter;
}

void LibConfigSetter::loadConfig(imageViewerSpace::ImgViewerType type)
{
    if (m_viewType == type)
        return;

    m_viewType = type;
    m_settings = new QSettings(imageViewerSpace::ImgViewerTypeAlbum == type ? CONFIG_PATH_ALBUM : CONFIG_PATH_IMAGE_VIEWER, QSettings::IniFormat, this);

//    if (imageViewerSpace::ImgViewerTypeAlbum == m_viewType) {
//        if (!m_settings->contains("album-zoomratio")) {
//            m_settings->setValue("album-zoomratio", 4);
//        }
//    } else if (imageViewerSpace::ImgViewerTypeLocal == m_viewType) {

//    }
}

void LibConfigSetter::setValue(const QString &group, const QString &key,
                               const QVariant &value)
{
    m_settings->beginGroup(group);
    m_settings->setValue(key, value);
    m_settings->endGroup();

    emit valueChanged(group, key, value);
}

QVariant LibConfigSetter::value(const QString &group, const QString &key,
                                const QVariant &defaultValue)
{
    QMutexLocker locker(&m_mutex);

    QVariant value;
    m_settings->beginGroup(group);
    value = m_settings->value(key, defaultValue);
    m_settings->endGroup();

    return value;
}
