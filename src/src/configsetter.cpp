// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
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
    // qDebug() << "LibConfigSetter::LibConfigSetter - Function entry";
}

LibConfigSetter::~LibConfigSetter() {
    // qDebug() << "LibConfigSetter::~LibConfigSetter - Function entry";
}

LibConfigSetter *LibConfigSetter::m_setter = nullptr;
LibConfigSetter *LibConfigSetter::instance()
{
    // qDebug() << "LibConfigSetter::instance - Function entry";
    if (!m_setter) {
        // qDebug() << "LibConfigSetter::instance - Branch: creating new instance";
        m_setter = new LibConfigSetter();
    }

    // qDebug() << "LibConfigSetter::instance - Function exit";
    return m_setter;
}

void LibConfigSetter::loadConfig(imageViewerSpace::ImgViewerType type)
{
    qDebug() << "LibConfigSetter::loadConfig - Function entry, type:" << type;
    if (m_viewType == type) {
        qDebug() << "LibConfigSetter::loadConfig - Branch: same type, returning early";
        return;
    }

    m_viewType = type;
    m_settings = new QSettings(imageViewerSpace::ImgViewerTypeAlbum == type ? CONFIG_PATH_ALBUM : CONFIG_PATH_IMAGE_VIEWER, QSettings::IniFormat, this);

    if (imageViewerSpace::ImgViewerTypeAlbum == type) {
        qDebug() << "LibConfigSetter::loadConfig - Branch: configuring album type settings";
        if (!contains("", "loadDayView"))
            setValue("", "loadDayView", 1);
        if (!contains("", "loadImport"))
            setValue("", "loadImport", 1);
        if (!contains("","animationDuration"))
            setValue("", "animationDuration", 400);
        if (!contains("", "largeImagePreviewAnimationDuration"))
            setValue("", "largeImagePreviewAnimationDuration", 800);
        if (!contains("", "enableSidebarAnimation"))
            setValue("", "enableSidebarAnimation", 0);
    }

//    if (imageViewerSpace::ImgViewerTypeAlbum == m_viewType) {
//        if (!m_settings->contains("album-zoomratio")) {
//            m_settings->setValue("album-zoomratio", 4);
//        }
//    } else if (imageViewerSpace::ImgViewerTypeLocal == m_viewType) {

//    }
    qDebug() << "LibConfigSetter::loadConfig - Function exit";
}

void LibConfigSetter::setValue(const QString &group, const QString &key, const QVariant &value)
{
    qDebug() << "LibConfigSetter::setValue - Function entry, group:" << group << "key:" << key << "value:" << value;
    m_settings->beginGroup(group);
    m_settings->setValue(key, value);
    m_settings->endGroup();

    emit valueChanged(group, key, value);
    qDebug() << "LibConfigSetter::setValue - Function exit";
}

QVariant LibConfigSetter::value(const QString &group, const QString &key, const QVariant &defaultValue)
{
    qDebug() << "LibConfigSetter::value - Function entry, group:" << group << "key:" << key;
    QMutexLocker locker(&m_mutex);

    QVariant value;
    m_settings->beginGroup(group);
    value = m_settings->value(key, defaultValue);
    m_settings->endGroup();

    qDebug() << "LibConfigSetter::value - Function exit, returning value:" << value;
    return value;
}

bool LibConfigSetter::contains(const QString &group, const QString &key)
{
    qDebug() << "LibConfigSetter::contains - Function entry, group:" << group << "key:" << key;
    QMutexLocker locker(&m_mutex);

    bool bContains = false;
    QVariant value;
    m_settings->beginGroup(group);
    bContains = m_settings->contains(key);
    m_settings->endGroup();

    qDebug() << "LibConfigSetter::contains - Function exit, returning:" << bContains;
    return bContains;
}
