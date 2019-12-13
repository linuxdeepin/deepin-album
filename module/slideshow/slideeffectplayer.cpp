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
#include "slideeffectplayer.h"
#include "application.h"
#include "controller/configsetter.h"
#include <QDebug>
#include <QFileInfo>
#include <QPainter>
#include <QTimerEvent>
#include <DIconButton>

namespace {

const QString DURATION_SETTING_GROUP = "SLIDESHOWDURATION";
const QString DURATION_SETTING_KEY = "Duration";
const int ANIMATION_DURATION  = 1000;
const int SLIDER_DURATION  = 3000;

} // namespace

SlideEffectPlayer::SlideEffectPlayer(QObject *parent)
    : QObject(parent)
{
    connect(dApp->signalM, &SignalManager::updateButton, this, [=]{
            killTimer(m_tid);
            m_tid = 0;
    });
    connect(dApp->signalM, &SignalManager::sigStartTimer, this, [=]{
        m_tid = startTimer(SLIDER_DURATION);
    });

}

SlideEffectPlayer::~SlideEffectPlayer()
{
    if (m_thread.isRunning())
    {
        m_thread.quit();
    }

    if (m_effect)
    {
        delete m_effect;
        m_effect = nullptr;
    }
}

void SlideEffectPlayer::timerEvent(QTimerEvent *e)
{
    if (e->timerId() != m_tid || m_pausing)
        return;
//    if(m_current == m_paths.size()-1){
//        emit dApp->signalM->updateButton();
//        emit dApp->signalM->updatePauseButton();
//    }

    if (! startNext()) {
        stop();
    }
}

int SlideEffectPlayer::duration() const
{
    return dApp->setter->value(DURATION_SETTING_GROUP,
                               DURATION_SETTING_KEY).toInt() * 1000;
}

void SlideEffectPlayer::setFrameSize(int width, int height)
{
    m_w = width;
    m_h = height;
}

void SlideEffectPlayer::setImagePaths(const QStringList& paths)
{
    m_paths = paths;
    m_current = 0;
}

void SlideEffectPlayer::setCurrentImage(const QString &path)
{
    for (int i = 0; i < m_paths.length(); i++)
    {
        if(path == m_paths[i])
        {
            m_current = i;
        }
    }
}

QString SlideEffectPlayer::currentImagePath() const
{
    return m_paths[m_current];
}

bool SlideEffectPlayer::isRunning() const
{
    return m_running;
}

void SlideEffectPlayer::start()
{
    if (m_paths.isEmpty())
        return;

    cacheNext();
    cachePrevious();
    m_running = true;
    m_tid = startTimer(SLIDER_DURATION);
}

void SlideEffectPlayer::pause() {
    if (m_effect)
        {
            m_pausing = !m_pausing;
            m_effect->pause();
        }
}

bool SlideEffectPlayer::startNext()
{
    qDebug()<<"SlideEffectPlayer::startNext()";

    if (m_paths.isEmpty())
        return false;
    QSize fSize(m_w, m_h);
    if (! fSize.isValid()) {
        qWarning() << "Invalid frame size!";
        return false;
    }

    if(1 == m_paths.length())
    {
        return false;
    }

    int current = m_current + 1;
    if (current == m_paths.length()) {
        current = 0;
    }

    if (m_cacheImages.value(m_paths[current]).isNull())
    {
        return false;
    }

    if (m_effect)
        m_effect->deleteLater();

    const QString oldPath = m_paths[m_current];

    if(m_paths.length() > 1)
    {
        m_current++;
        if(m_current == m_paths.length()){
            m_current = 0;
        }
    }

    cacheNext();

    QString newPath = m_paths[m_current];
    m_effect = SlideEffect::create("enter_from_right");
    m_effect->setDuration(ANIMATION_DURATION);
    m_effect->setSize(fSize);

    using namespace utils::image;
    qDebug()<<"m_cacheImages.value";
    QImage oldImg = m_cacheImages.value(oldPath);
    QImage newImg = m_cacheImages.value(newPath);
    // The "newPath" would be the next "oldPath", so there is no need to remove it now
//    m_cacheImages.remove(oldPath);

    qDebug()<<m_cacheImages;
    m_effect->setImages(oldImg, newImg);
    if (!m_thread.isRunning())
        m_thread.start();

    m_effect->moveToThread(&m_thread);
    connect(m_effect, &SlideEffect::frameReady, this, [=] (const QImage &img) {
        if (m_running) {
            Q_EMIT frameReady(img);
        }
    }, Qt::DirectConnection);
    QMetaObject::invokeMethod(m_effect, "start");

    if(m_current == m_paths.length()-1){
        emit dApp->signalM->updateButton();
        emit dApp->signalM->updatePauseButton();
    }

    return true;
}

bool SlideEffectPlayer::startPrevious()
{
    if (m_paths.isEmpty())
    {
        return false;
    }

    QSize fSize(m_w, m_h);
    if (! fSize.isValid())
    {
        return false;
    }

    if(1 == m_paths.length())
    {
        return false;
    }

    int current = m_current - 1;
    if (current == -1)
    {
        current = m_paths.length() - 1;
    }

    if (m_cacheImages.value(m_paths[current]).isNull())
    {
        return false;
    }

    if (m_effect)
        m_effect->deleteLater();

    const QString oldPath = m_paths[m_current];

    if(m_paths.length() > 1){
        m_current --;
        if(m_current == -1){
            m_current = m_paths.length()-1;
        }
    }

    cachePrevious();

    QString newPath = m_paths[m_current];
    m_effect = SlideEffect::create("enter_from_left");
    m_effect->setDuration(ANIMATION_DURATION);
    m_effect->setSize(fSize);

    using namespace utils::image;
    qDebug()<<"m_cacheImages.value";
    QImage oldImg = m_cacheImages.value(oldPath);
    QImage newImg = m_cacheImages.value(newPath);
    // The "newPath" would be the next "oldPath", so there is no need to remove it now

//    m_cacheImages.remove(oldPath);

    m_effect->setImages(oldImg, newImg);
//    if (!m_thread.isRunning())
//        m_thread.start();

//    m_effect->moveToThread(&m_thread);
    connect(m_effect, &SlideEffect::frameReady, this, [=] (const QImage &img) {
        if (m_running) {
            Q_EMIT frameReady(img);
        }
    }, Qt::DirectConnection);
    QMetaObject::invokeMethod(m_effect, "start");
    return true;
}

void SlideEffectPlayer::cacheNext()
{
    qDebug()<<"SlideEffectPlayer::cacheNext()";
    int current = m_current;
    current ++;
    if (current == m_paths.length())
    {
        current = 0;
    }

    QString path = m_paths[current];

    if(m_cacheImages.value(path).isNull())
    {
        CacheThread *t = new CacheThread(path);
        connect(t, &CacheThread::cached,
                this, [=] (const QString path, const QImage img) {
            qDebug()<<"m_cacheImages.insert(path, img)";
            m_cacheImages.insert(path, img);
        });
        connect(t, &CacheThread::finished, t, &CacheThread::deleteLater);
        t->start();
    }
}

void SlideEffectPlayer::cachePrevious()
{
    qDebug()<<"SlideEffectPlayer::cachePrevious()";
    int current = m_current;
    current--;
    if (-1 == current)
    {
        current = m_paths.length() - 1;
    }

    QString path = m_paths[current];

    if(m_cacheImages.value(path).isNull())
    {
        CacheThread *t = new CacheThread(path);
        connect(t, &CacheThread::cached,
                this, [=] (const QString path, const QImage img) {
            qDebug()<<"m_cacheImages.insert(path, img)";
            m_cacheImages.insert(path, img);
        });
        connect(t, &CacheThread::finished, t, &CacheThread::deleteLater);
        t->start();
    }
}

void SlideEffectPlayer::stop()
{
    if (!isRunning())
        return;

    if(m_pausing)
    {
        m_pausing = !m_pausing;
        m_effect->pause();
    }

    killTimer(m_tid);
    m_tid = 0;
    m_running = false;
    m_cacheImages.clear();
    Q_EMIT finished();
}

