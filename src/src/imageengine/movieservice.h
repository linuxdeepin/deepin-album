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
#include <QDateTime>
#include <deque>
#include <QRegExp>

#include <mutex>

struct MovieInfo {
    bool valid = false;
    QString filePath = "-";  //文件路径
    QString fileType = "-";  //文件类型
    QString resolution = "-";//分辨率
    QDateTime creation;      //创建时间
    qint64 fileSize = 0;     //文件大小
    QString duration = "-";  //视频长度

    //视频流信息
    QString vCodecID = "-";  //编码格式
    qint64 vCodeRate = 0;    //码率
    int fps = 0;             //帧率
    double proportion = -1;  //长宽比

    //音频流信息
    QString aCodeID = "-"; //编码格式
    qint64 aCodeRate = 0;  //码率
    QString aDigit = "-";  //数据格式
    int channels = 0;      //通道数
    int sampling = 0;      //采样率

    QString sizeStr() const
    {
        auto K = 1024;
        auto M = 1024 * K;
        auto G = 1024 * M;
        if (fileSize > G) {
            return QString("%1G").arg((double)fileSize / G, 0, 'f', 1);
        } else if (fileSize > M) {
            return QString("%1M").arg((double)fileSize / M, 0, 'f', 1);
        } else if (fileSize > K) {
            return QString("%1K").arg((double)fileSize / K, 0, 'f', 1);
        }
        return QString("%1").arg(fileSize);
    }
};

class MovieService: public QObject
{
    Q_OBJECT
public:
    static MovieService *instance(QObject *parent = nullptr);

    //获取视频信息
    MovieInfo getMovieInfo(const QUrl &url);

    //获取视频首帧图片
    QImage getMovieCover(const QUrl &url);

    //获取ffmpeg状态
    bool ffmpegIsExist()
    {
        return m_ffmpegExist;
    }

private:
    explicit MovieService(QObject *parent = nullptr);
    struct MovieInfo parseFromFile(const QFileInfo &fi);

private:
    QMutex m_queuqMutex;
    static MovieService *m_movieService;
    static std::once_flag instanceFlag;
    bool m_ffmpegExist = false;
    QMutex m_bufferMutex;
    std::deque<std::pair<QUrl, MovieInfo>> m_movieInfoBuffer;

    QRegExp resolutionExp;
    QRegExp codeRateExp;
    QRegExp fpsExp;
};

#endif // MOVIESERVICE_H
