/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     LiuMingHang <liuminghang@uniontech.com>
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
