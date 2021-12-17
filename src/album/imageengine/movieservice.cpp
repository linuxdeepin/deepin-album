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
#include "movieservice.h"
#include <QMetaType>
#include <QDirIterator>
#include <QStandardPaths>
#include <QLibrary>
#include <QLibraryInfo>

#include "controller/signalmanager.h"
#include "application.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/avutil.h>
}

#define THUMBNAIL_SIZE 500
#define SEEK_TIME "00:00:01"

MovieService *MovieService::m_movieService = nullptr;

typedef int (*mvideo_avformat_open_input)(AVFormatContext **ps, const char *url, AVInputFormat *fmt, AVDictionary **options);
typedef int (*mvideo_avformat_find_stream_info)(AVFormatContext *ic, AVDictionary **options);
typedef int (*mvideo_av_find_best_stream)(AVFormatContext *ic, enum AVMediaType type, int wanted_stream_nb, int related_stream, AVCodec **decoder_ret, int flags);
typedef AVDictionaryEntry *(*mvideo_av_dict_get)(const AVDictionary *m, const char *key, const AVDictionaryEntry *prev, int flags);
typedef void (*mvideo_avformat_close_input)(AVFormatContext **s);

mvideo_avformat_open_input g_mvideo_avformat_open_input = nullptr;
mvideo_avformat_find_stream_info g_mvideo_avformat_find_stream_info = nullptr;
mvideo_av_find_best_stream g_mvideo_av_find_best_stream = nullptr;
mvideo_av_dict_get g_mvideo_av_dict_get = nullptr;
mvideo_avformat_close_input g_mvideo_avformat_close_input = nullptr;

MovieService *MovieService::instance(QObject *parent)
{
    Q_UNUSED(parent);
    if (!m_movieService) {
        m_movieService = new MovieService();
    }
    return m_movieService;
}

MovieService::~MovieService()
{
}

MovieInfo MovieService::getMovieInfo(const QUrl &url)
{
    if (!m_initFFmpeg) {
        initFFmpeg();
    }

    if (url.isLocalFile()) {
        QFileInfo fi(url.toLocalFile());
        if (fi.exists()) {
            return parseFromFile(fi);
        } else {
            return MovieInfo();
        }
    } else {
        return MovieInfo();
    }
}

QImage MovieService::getMovieCover(const QUrl &url)
{
    QMutexLocker locker(&m_queuqMutex);
    if (!m_bInitThumb) {
        initThumb();
        m_mvideo_thumbnailer_destroy_image_data(m_image_data);
        m_image_data = nullptr;
    }

    if (m_creat_video_thumbnailer == nullptr
            || m_mvideo_thumbnailer_destroy == nullptr
            || m_mvideo_thumbnailer_create_image_data == nullptr
            || m_mvideo_thumbnailer_destroy_image_data == nullptr
            || m_mvideo_thumbnailer_generate_thumbnail_to_buffer == nullptr
            || m_video_thumbnailer == nullptr) {
        return QImage();
    }

    m_video_thumbnailer->thumbnail_size = static_cast<int>(THUMBNAIL_SIZE);
    //不取第一帧，与文管影院保持一致
//    m_video_thumbnailer->seek_time = const_cast<char *>(SEEK_TIME);
    m_image_data = m_mvideo_thumbnailer_create_image_data();
    QString file = QFileInfo(url.toLocalFile()).absoluteFilePath();
    m_mvideo_thumbnailer_generate_thumbnail_to_buffer(m_video_thumbnailer, file.toUtf8().data(), m_image_data);
    QImage img = QImage::fromData(m_image_data->image_data_ptr, static_cast<int>(m_image_data->image_data_size), "png");
    m_mvideo_thumbnailer_destroy_image_data(m_image_data);
    m_image_data = nullptr;
    return img;
}

MovieInfo MovieService::parseFromFile(const QFileInfo &fi)
{
    struct MovieInfo mi;
    mi.valid = false;
    AVFormatContext *av_ctx = nullptr;
    AVCodecParameters *video_dec_ctx = nullptr;
    AVCodecParameters *audio_dec_ctx = nullptr;

    if (!fi.exists()) {
//        if (ok) *ok = false;
        return mi;
    }

    auto ret = g_mvideo_avformat_open_input(&av_ctx, fi.filePath().toUtf8().constData(), nullptr, nullptr);
    if (ret < 0) {
        qWarning() << "avformat: could not open input";
//        if (ok) *ok = false;
        return mi;
    }

    if (g_mvideo_avformat_find_stream_info(av_ctx, nullptr) < 0) {
        qWarning() << "av_find_stream_info failed";
//        if (ok) *ok = false;
        return mi;
    }

    if (av_ctx->nb_streams == 0) {
//        if (ok) *ok = false;
        return mi;
    }

    int videoRet = -1;
    int audioRet = -1;
    AVStream *videoStream = nullptr;
    AVStream *audioStream = nullptr;
    videoRet = g_mvideo_av_find_best_stream(av_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    audioRet = g_mvideo_av_find_best_stream(av_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (videoRet < 0 && audioRet < 0) {
//        if (ok) *ok = false;
        return mi;
    }

    if (videoRet >= 0) {
        int video_stream_index = -1;
        video_stream_index = videoRet;
        videoStream = av_ctx->streams[video_stream_index];
        video_dec_ctx = videoStream->codecpar;

        mi.width = video_dec_ctx->width;
        mi.height = video_dec_ctx->height;
        mi.vCodecID = video_dec_ctx->codec_id;
        mi.vCodeRate = video_dec_ctx->bit_rate;

        if (videoStream->r_frame_rate.den != 0) {
            mi.fps = videoStream->r_frame_rate.num / videoStream->r_frame_rate.den;
        } else {
            mi.fps = 0;
        }
        if (mi.height != 0) {
            mi.proportion = static_cast<float>(mi.width) / static_cast<float>(mi.height);
        } else {
            mi.proportion = 0;
        }
    }
    if (audioRet >= 0) {
        int audio_stream_index = -1;
        audio_stream_index = audioRet;
        audioStream = av_ctx->streams[audio_stream_index];
        audio_dec_ctx = audioStream->codecpar;

        mi.aCodeID = audio_dec_ctx->codec_id;
        mi.aCodeRate = audio_dec_ctx->bit_rate;
        mi.aDigit = audio_dec_ctx->format;
        mi.channels = audio_dec_ctx->channels;
        mi.sampling = audio_dec_ctx->sample_rate;
    }

    auto duration = av_ctx->duration == AV_NOPTS_VALUE ? 0 : av_ctx->duration;
    duration = duration + (duration <= INT64_MAX - 5000 ? 5000 : 0);
    mi.duration = duration / AV_TIME_BASE;
    mi.resolution = QString("%1x%2").arg(mi.width).arg(mi.height);
    mi.title = fi.fileName(); //FIXME this
    mi.filePath = fi.canonicalFilePath();
    mi.creation = fi.created().toString();
    mi.fileSize = fi.size();
    mi.fileType = fi.suffix();

    AVDictionaryEntry *tag = nullptr;
    while ((tag = g_mvideo_av_dict_get(av_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)) != nullptr) {
        if (tag->key && strcmp(tag->key, "creation_time") == 0) {
            auto dt = QDateTime::fromString(tag->value, Qt::ISODate);
            mi.creation = dt.toString();
            qInfo() << __func__ << dt.toString();
            break;
        }
        qInfo() << "tag:" << tag->key << tag->value;
    }

    g_mvideo_avformat_close_input(&av_ctx);
    mi.valid = true;

//    if (ok) *ok = true;
    return mi;
}

MovieService::MovieService(QObject *parent)
{
    Q_UNUSED(parent);
}

void MovieService::initThumb()
{
    QLibrary library(libPath("libffmpegthumbnailer.so"));
    m_creat_video_thumbnailer = (mvideo_thumbnailer_create) library.resolve("video_thumbnailer_create");
    m_mvideo_thumbnailer_destroy = (mvideo_thumbnailer_destroy) library.resolve("video_thumbnailer_destroy");
    m_mvideo_thumbnailer_create_image_data = (mvideo_thumbnailer_create_image_data) library.resolve("video_thumbnailer_create_image_data");
    m_mvideo_thumbnailer_destroy_image_data = (mvideo_thumbnailer_destroy_image_data) library.resolve("video_thumbnailer_destroy_image_data");
    m_mvideo_thumbnailer_generate_thumbnail_to_buffer = (mvideo_thumbnailer_generate_thumbnail_to_buffer) library.resolve("video_thumbnailer_generate_thumbnail_to_buffer");
    m_video_thumbnailer = m_creat_video_thumbnailer();

    if (m_mvideo_thumbnailer_destroy == nullptr
            || m_mvideo_thumbnailer_create_image_data == nullptr
            || m_mvideo_thumbnailer_destroy_image_data == nullptr
            || m_mvideo_thumbnailer_generate_thumbnail_to_buffer == nullptr
            || m_video_thumbnailer == nullptr) {
        return;
    }

    m_image_data = m_mvideo_thumbnailer_create_image_data();
    m_video_thumbnailer->thumbnail_size = 400 * qApp->devicePixelRatio();
    m_bInitThumb = true;
}

void MovieService::initFFmpeg()
{
    QLibrary avcodecLibrary(libPath("libavcodec.so"));
    QLibrary avformatLibrary(libPath("libavformat.so"));
    QLibrary avutilLibrary(libPath("libavutil.so"));

    g_mvideo_avformat_open_input = (mvideo_avformat_open_input) avformatLibrary.resolve("avformat_open_input");
    g_mvideo_avformat_find_stream_info = (mvideo_avformat_find_stream_info) avformatLibrary.resolve("avformat_find_stream_info");
    g_mvideo_av_find_best_stream = (mvideo_av_find_best_stream) avformatLibrary.resolve("av_find_best_stream");
    g_mvideo_avformat_close_input = (mvideo_avformat_close_input) avformatLibrary.resolve("avformat_close_input");
    g_mvideo_av_dict_get = (mvideo_av_dict_get) avutilLibrary.resolve("av_dict_get");

    m_initFFmpeg = true;
}

QString MovieService::libPath(const QString &strlib)
{
    QDir  dir;
    QString path  = QLibraryInfo::location(QLibraryInfo::LibrariesPath);
    dir.setPath(path);
    QStringList list = dir.entryList(QStringList() << (strlib + "*"), QDir::NoDotAndDotDot | QDir::Files); //filter name with strlib
    if (list.contains(strlib)) {
        return strlib;
    } else {
        list.sort();
    }

    Q_ASSERT(list.size() > 0);
    return list.last();
}
