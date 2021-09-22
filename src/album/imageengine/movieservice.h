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
#ifndef MOVIESERVICE_H
#define MOVIESERVICE_H

#include <QObject>
#include <QMap>
#include <QUrl>
#include <QFileInfo>
#include <QMutex>

#include "baseutils.h"
#include <libffmpegthumbnailer/videothumbnailerc.h>

typedef video_thumbnailer *(*mvideo_thumbnailer_create)();
typedef void (*mvideo_thumbnailer_destroy)(video_thumbnailer *thumbnailer);
/* create image_data structure */
typedef image_data *(*mvideo_thumbnailer_create_image_data)(void);
/* destroy image_data structure */
typedef void (*mvideo_thumbnailer_destroy_image_data)(image_data *data);
typedef int (*mvideo_thumbnailer_generate_thumbnail_to_buffer)(video_thumbnailer *thumbnailer, const char *movie_filename, image_data *generated_image_data);


struct MovieInfo {
    bool valid;
    QString title;
    QString fileType;
    QString resolution;
    QString filePath;
    QString creation;

    // rotation in metadata, this affects width/height
    int raw_rotate;
    qint64 fileSize;
    qint64 duration;
    int width = -1;
    int height = -1;

    //3.4添加视频信息
    //视频流信息
    int vCodecID;
    qint64 vCodeRate;
    int fps;
    float proportion;
    //音频流信息
    int aCodeID;
    qint64 aCodeRate;
    int aDigit;
    int channels;
    int sampling;

//    static struct MovieInfo parseFromFile(const QFileInfo &fi, bool *ok = nullptr);
    QString durationStr() const
    {
        return utils::base::Time2str(duration);
    }

    QString videoCodec() const
    {
        return utils::base::videoIndex2str(vCodecID);
    }

    QString audioCodec() const
    {
        return utils::base::audioIndex2str(aCodeID);
    }

    QString sizeStr() const
    {
        auto K = 1024;
        auto M = 1024 * K;
        auto G = 1024 * M;
        if (fileSize > G) {
            return QString(QT_TR_NOOP("%1G")).arg((double)fileSize / G, 0, 'f', 1);
        } else if (fileSize > M) {
            return QString(QT_TR_NOOP("%1M")).arg((double)fileSize / M, 0, 'f', 1);
        } else if (fileSize > K) {
            return QString(QT_TR_NOOP("%1K")).arg((double)fileSize / K, 0, 'f', 1);
        }
        return QString(QT_TR_NOOP("%1")).arg(fileSize);
    }
};

class MovieService: public QObject
{
    Q_OBJECT
public:
    static MovieService *instance(QObject *parent = nullptr);
    ~MovieService();

    //获取视频信息
    MovieInfo   getMovieInfo(const QUrl &url);
    //获取视频首帧图片
    QImage      getMovieCover(const QUrl &url);
private:
    struct MovieInfo parseFromFile(const QFileInfo &fi);
    explicit MovieService(QObject *parent = nullptr);
    void initThumb();
    void initFFmpeg();
    QString libPath(const QString &strlib);
private slots:

signals:
public:

private:
    QMutex m_queuqMutex;
    static MovieService *m_movieService;
    bool m_bInitThumb = false;
    bool m_initFFmpeg = false;

    video_thumbnailer *m_video_thumbnailer = nullptr;
    image_data *m_image_data = nullptr;

    mvideo_thumbnailer_create m_creat_video_thumbnailer = nullptr;
    mvideo_thumbnailer_destroy m_mvideo_thumbnailer_destroy = nullptr;
    mvideo_thumbnailer_create_image_data m_mvideo_thumbnailer_create_image_data = nullptr;
    mvideo_thumbnailer_destroy_image_data m_mvideo_thumbnailer_destroy_image_data = nullptr;
    mvideo_thumbnailer_generate_thumbnail_to_buffer m_mvideo_thumbnailer_generate_thumbnail_to_buffer = nullptr;
};

#endif // MOVIESERVICE_H
